#include "remoteexecution.h"

#include <cstdlib>
#include "base/exception.h"
#include "base/tools.h"
#include "openfoam/openfoamcase.h"
#include "pstreams/pstream.h"
#include <boost/asio.hpp>
#include <boost/process/async.hpp>

#include <regex>

using namespace std;
using namespace boost;

namespace insight
{



boost::filesystem::path RemoteExecutionConfig::socket() const
{
  return remoteDir()/"tsp.socket";
}

void RemoteExecutionConfig::execRemoteCmd(const std::string& command)
{
    std::ostringstream cmd;

    cmd << "ssh " << server_ << " \"";
     cmd << "export TS_SOCKET="<<socket()<<";";

     try
     {
         const OFEnvironment& cofe = OFEs::getCurrent();
         cmd << "source " << cofe.bashrc().filename() << ";";
     }
     catch (const std::exception& /*e*/) {
         // ignore, don't load OF config remotely
     }

     cmd << "cd "<<remoteDir_<<" && (";
     cmd << command;
    cmd << ")\"";

    std::cout<<cmd.str()<<std::endl;
    if (! ( std::system(cmd.str().c_str()) == 0 ))
    {
        throw insight::Exception("Could not execute command on server "+server_+": \""+cmd.str()+"\"");
    }
}

bool RemoteExecutionConfig::isValid() const
{
    return (!server_.empty())&&(!remoteDir_.empty());
}

bool RemoteExecutionConfig::remoteDirExists() const
{
  if (isValid())
  {
    int ret = boost::process::system(
                boost::process::search_path("ssh"),
                boost::process::args({server(), "cd", remoteDir().string()})
          );
    if (ret==0) return true;
  }
  return false;
}

RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig, const bfs_path& meta_file)
  : localDir_(location)
{
  if (meta_file.empty()) {
      meta_file_ = location/"meta.foam";
  } else {
      meta_file_ = meta_file;
  }

  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+location.string()+" from file "+meta_file_.string());

  if (!boost::filesystem::exists(meta_file_))
  {
      if (needConfig)
          throw insight::Exception("There is no remote execution configuration file present!");
  }
  else {
    std::ifstream f(meta_file_.c_str());
    std::string line;
    if (!getline(f, line))
      throw insight::Exception("Could not read first line from file "+meta_file_.string());

    std::vector<std::string> pair;
    boost::split(pair, line, boost::is_any_of(":"));
    if (pair.size()!=2)
      throw insight::Exception("Error reading "+meta_file_.string()+": expected <server>:<remote directory>, got "+line);

    server_=pair[0];
    remoteDir_=pair[1];

//    std::cout<<"configured "<<server_<<":"<<remoteDir_<<std::endl;
  }
}

const std::string& RemoteExecutionConfig::server() const
{
  return server_;
}

const boost::filesystem::path& RemoteExecutionConfig::localDir() const
{
  return localDir_;
}

const boost::filesystem::path& RemoteExecutionConfig::remoteDir() const
{
  return remoteDir_;
}

const boost::filesystem::path& RemoteExecutionConfig::metaFile() const
{
  return meta_file_;
}


std::vector<bfs_path> RemoteExecutionConfig::remoteLS() const
{
  std::vector<bfs_path> res;

  redi::ipstream p_in;

  p_in.open("ssh", { "ssh", server(), "ls", remoteDir().string() } );

  if (!p_in.is_open())
  {
    throw insight::Exception("RemoteExecutionConfig::remoteLS: Failed to launch directory listing subprocess!");
  }

  std::string line;
  while (std::getline(p_in.out(), line))
  {
    cout<<line<<endl;
    res.push_back(line);
  }
  while (std::getline(p_in.err(), line))
  {
    cerr<<"ERR: "<<line<<endl;
  }
  p_in.close();

  if (p_in.rdbuf()->status()!=0)
  {
    throw insight::Exception("RemoteExecutionConfig::remoteLS: command failed with nonzero return code.");
  }

  return res;
}

std::vector<bfs_path> RemoteExecutionConfig::remoteSubdirs() const
{
  std::vector<bfs_path> res;
  boost::process::ipstream is;
  std::shared_ptr<boost::process::child> c;

  c.reset(new boost::process::child(
            boost::process::search_path("ssh"),
            boost::process::args({server(),
                                  "find", remoteDir().string()+"/", // add slash for symbolic links
                                  "-maxdepth", "1", "-type", "d", "-printf", "%P\\\\n"}),
            boost::process::std_out > is
            ));

  if (!c->running())
    throw insight::Exception("Could not execute remote dir list process!");

  std::string line;
  while (std::getline(is, line))
  {
    res.push_back(line);
  }

  c->wait();

  return res;
}


void RemoteExecutionConfig::runRsync
(
    const std::vector<std::string>& args,
    std::function<void(int,const std::string&)> pf
)
{
  namespace bp=boost::process;
  bp::ipstream is;
  bp::child c
      (
       bp::search_path("rsync"),
       bp::args( args ),
       bp::std_out > is
      );

  std::string line;
  boost::regex pattern(".* ([^ ]*)% *([^ ]*) *([^ ]*) \\(xfr#([0-9]+), to-chk=([0-9]+)/([0-9]+)\\)");
  while (c.running() && std::getline(is, line) && !line.empty())
  {
    boost::smatch match;
    if (boost::regex_search( line, match, pattern, boost::match_default ))
    {
      std::string percent=match[1];
      std::string rate=match[2];
      std::string eta=match[3];
//        int i_file=to_number<int>(match[4]);
      int i_to_chk=to_number<int>(match[5]);
      int total_to_chk=to_number<int>(match[6]);

      double progress = total_to_chk==0? 1.0 : double(total_to_chk-i_to_chk) / double(total_to_chk);

      if (pf) pf(int(100.*progress), rate+", "+eta+" (current file: "+percent+")");
    }
  }

  c.wait();
}

void RemoteExecutionConfig::putFile
(
    const boost::filesystem::path& localFile,
    const boost::filesystem::path& remoteFileName,
    std::function<void(int,const std::string&)> pf
    )
{
  boost::filesystem::path lf=localFile, rf=remoteFileName;
  if (lf.is_relative()) lf=localDir_/lf;
  if (rf.is_relative()) rf=remoteDir_/rf;
  std::vector<std::string> args=
      {
       lf.string(),
       server_+":"+rf.string()
      };

  runRsync(args, pf);
}


void RemoteExecutionConfig::syncToRemote
(
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
    std::vector<std::string> args=
        {
         "-az",
         "--delete",
         "--info=progress",

         "--exclude", "processor*",
         "--exclude", "*.foam",
         "--exclude", "postProcessing",
         "--exclude", "*.socket",
         "--exclude", "backup",
         "--exclude", "archive",
         "--exclude", "mnt_remote"
        };

    for (const auto& ex: exclude_pattern)
    {
      args.push_back("--exclude");
      args.push_back(ex);
    }

    args.push_back(localDir_.string());
    args.push_back(server_+":"+remoteDir_.string());

    runRsync(args, pf);
}

void RemoteExecutionConfig::syncToLocal
(
    bool skipTimeSteps,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
    std::vector<std::string> args;

    args =
    {
      "-az",
      "--info=progress",
      "--exclude", "processor*",
      "--exclude", "*.foam",
      "--exclude", "*.socket",
      "--exclude", "backup",
      "--exclude", "archive",
      "--exclude", "mnt_remote"
    };


    if (skipTimeSteps)
      {
        auto files = remoteLS();

        // remove non-numbers
        files.erase(remove_if(files.begin(), files.end(),
                [&](const bfs_path& f)
                {
                  try { to_number<double>(f.c_str()); return false; }
                  catch (...) { return true; }
                }), files.end());

        for (const auto& f: files)
          {
            args.push_back("--exclude");
            args.push_back(f.c_str());
          }
      }

    for (const auto& ex: exclude_pattern)
    {
      args.push_back("--exclude");
      args.push_back(ex);
    }

    args.push_back(server_+":"+remoteDir_.string()+"/*");
    args.push_back(localDir_.string());

    runRsync(args, pf);
}

void RemoteExecutionConfig::queueRemoteCommand(const std::string& command, bool waitForPreviousFinished)
{
  if (waitForPreviousFinished)
      execRemoteCmd("tsp -d " + command);
  else
      execRemoteCmd("tsp " + command);
}


void RemoteExecutionConfig::waitRemoteQueueFinished()
{
    execRemoteCmd("while tsp -c; do tsp -C; done");
}

void RemoteExecutionConfig::waitLastCommandFinished()
{
    execRemoteCmd("tsp -t");
}

void RemoteExecutionConfig::cancelRemoteCommands()
{
    execRemoteCmd("tsp -C; tsp -k; tsp -K");
}

void RemoteExecutionConfig::removeRemoteDir()
{
    execRemoteCmd("tsp -C; tsp -k; tsp -K");
    execRemoteCmd("rm -r *; cd ..; rmdir "+remoteDir_.string());
}

}
