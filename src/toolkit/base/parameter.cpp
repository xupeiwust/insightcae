/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "parameter.h"
#include "base/latextools.h"
#include "base/tools.h"
#include <ostream>
#include <streambuf>
#include <iterator>

#include "boost/archive/iterators/base64_from_binary.hpp"
#include "boost/archive/iterators/binary_from_base64.hpp"
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

#include <iostream>




namespace boost 
{ 
namespace filesystem
{
  
    
    
    
template < >
// path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end, const codecvt_type& cvt)
path& path::append< typename path::iterator >( typename path::iterator begin, typename path::iterator end)
{ 
  for( ; begin != end ; ++begin )
	  *this /= *begin;
  return *this;
}



// Return path when appended to a_From will resolve to same as a_To
boost::filesystem::path make_relative( boost::filesystem::path a_From, boost::filesystem::path a_To )
{
  a_From = boost::filesystem::absolute( a_From ); a_To = boost::filesystem::absolute( a_To );
  boost::filesystem::path ret;
  boost::filesystem::path::const_iterator itrFrom( a_From.begin() ), itrTo( a_To.begin() );
  // Find common base
  for( boost::filesystem::path::const_iterator toEnd( a_To.end() ), fromEnd( a_From.end() ) ; itrFrom != fromEnd && itrTo != toEnd && *itrFrom == *itrTo; ++itrFrom, ++itrTo );
  // Navigate backwards in directory to reach previously found base
  for( boost::filesystem::path::const_iterator fromEnd( a_From.end() ); itrFrom != fromEnd; ++itrFrom )
  {
	  if( (*itrFrom) != "." )
		  ret /= "..";
  }
  // Now navigate down the directory branch
  ret.append( itrTo, a_To.end() );
  return ret;
}

  
} 
}




//namespace boost { namespace filesystem { using filesystem3::make_relative; } }




using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace rapidxml;

namespace insight 
{
  
    
rapidxml::xml_node<> *findNode(rapidxml::xml_node<>& father, const std::string& name, const std::string& typeName)
{
  for (xml_node<> *child = father.first_node(typeName.c_str()); child; child = child->next_sibling(typeName.c_str()))
  {
    if (child->first_attribute("name")->value() == name)
    {
      return child;
    }
  }
  return nullptr;
}

    
const std::string base64_padding[] = {"", "==","="};




std::string base64_encode(const std::string& s) 
{
  namespace bai = boost::archive::iterators;

  std::stringstream os;

  // convert binary values to base64 characters
  typedef bai::base64_from_binary
  // retrieve 6 bit integers from a sequence of 8 bit bytes
  <bai::transform_width<const char *, 6, 8> > base64_enc; // compose all the above operations in to a new iterator

  std::copy(base64_enc(s.c_str()), base64_enc(s.c_str() + s.size()),
            std::ostream_iterator<char>(os));

  os << base64_padding[s.size() % 3];
  return os.str();
}

string base64_encode(const path& f)
{
  std::ifstream in(f.c_str());
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  
  return base64_encode(contents_raw);
}


std::string base64_decode(const std::string& s) 
{
  namespace bai = boost::archive::iterators;

  std::stringstream os;

  typedef bai::transform_width<bai::binary_from_base64<const char *>, 8, 6> base64_dec;

  unsigned int size = s.size();

  // Remove the padding characters, cf. https://svn.boost.org/trac/boost/ticket/5629
  if (size && s[size - 1] == '=') {
    --size;
    if (size && s[size - 1] == '=') --size;
  }
  if (size == 0) return std::string();

  std::copy(base64_dec(s.data()), base64_dec(s.data() + size),
            std::ostream_iterator<char>(os));

  return os.str();
}



void writeMatToXMLNode(const arma::mat& matrix, xml_document< char >& doc, xml_node< char >& node)
{
  std::ostringstream voss;
  matrix.save(voss, arma::raw_ascii);
  
  // set stringified table values as node value
  node.value(doc.allocate_string(voss.str().c_str()));
}



defineType(Parameter);
defineFactoryTable(Parameter, LIST(const std::string& desc), LIST(desc) );

Parameter::Parameter()
{
}

Parameter::Parameter(const std::string& description, bool isHidden, bool isExpert, bool isNecessary, int order)
: description_(description),
  isHidden_(isHidden), isExpert_(isExpert), isNecessary_(isNecessary), order_(order)
{
}

Parameter::~Parameter()
{
}


bool Parameter::isHidden() const { return isHidden_; }
bool Parameter::isExpert() const {return isExpert_; }
bool Parameter::isNecessary() const { return isNecessary_; }
int Parameter::order() const { return order_; }


rapidxml::xml_node<>* Parameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path) const
{
    using namespace rapidxml;
    xml_node<>* child = doc.allocate_node(node_element, doc.allocate_string(this->type().c_str()));
    node.append_node(child);
    child->append_attribute(doc.allocate_attribute
    (
      "name", 
      doc.allocate_string(name.c_str()))
    );

    return child;
}

bool Parameter::isPacked() const
{
  return false;
}

void Parameter::pack()
{
    // do nothing by default
}

void Parameter::unpack(const boost::filesystem::path&)
{
    // do nothing by default
}

void Parameter::clearPackedData()
{
  // do nothing by default
}

void Parameter::reset(const Parameter& op)
{
  description_ = op.description_;
  isHidden_ = op.isHidden_;
  isExpert_ = op.isExpert_;
  isNecessary_ = op.isNecessary_;
  order_ = op.order_;
}


std::string valueToString(const arma::mat& value)
{
  std::string s;
  for (arma::uword i=0; i<value.n_elem; i++)
  {
    if (i>0) s+=" ";
    s += boost::str(boost::format("%g") % value(i)); //boost::lexical_cast<string>(value(i));
  }
  return s;
}


void stringToValue(const std::string& s, arma::mat& v)
{
  CurrentExceptionContext ex("converting string \""+s+"\" into vector");

  std::vector<std::string> cmpts;
  boost::split(cmpts, s, boost::is_any_of(" \t,;"), token_compress_on);
  std::vector<double> vals;
  for (size_t i=0; i<cmpts.size(); i++)
  {
    vals.push_back( to_number<double>(cmpts[i]) );
  }

  v=arma::mat(vals.data(), vals.size(), 1);
}
 
 
 
 
char DoubleName[] = "double";
char IntName[] = "int";
char BoolName[] = "bool";
char VectorName[] = "vector";
char StringName[] = "string";
char PathName[] = "pathbase";





  
template<> defineType(DoubleParameter);
template<> defineType(IntParameter);
template<> defineType(BoolParameter);
template<> defineType(VectorParameter);
template<> defineType(StringParameter);
typedef SimpleParameter<boost::filesystem::path, PathName> PathParameterBase;
template<> defineType(PathParameterBase);

addToFactoryTable(Parameter, DoubleParameter);
addToFactoryTable(Parameter, IntParameter);
addToFactoryTable(Parameter, BoolParameter);
addToFactoryTable(Parameter, VectorParameter);
addToFactoryTable(Parameter, StringParameter);
// addToFactoryTable(Parameter, PathParameter, std::string);







defineType(PathParameter);
addToFactoryTable(Parameter, PathParameter);

PathParameter::PathParameter(
    const string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order
    )
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

PathParameter::PathParameter(
    const path& value, const string& description,
    bool isHidden, bool isExpert, bool isNecessary, int order,
    std::shared_ptr<std::string> base64_content
    )
: Parameter(description, isHidden, isExpert, isNecessary, order),
  value_(value),
  file_content_(base64_content)
{
}


std::string PathParameter::latexRepresentation() const
{
    return SimpleLatex( valueToString ( value_ ) ).toLaTeX();
}

std::string PathParameter::plainTextRepresentation(int /*indent*/) const
{
  return SimpleLatex( valueToString ( value_ ) ).toPlainText();
}

bool PathParameter::isValid() const
{
  return !value_.empty();
}


boost::filesystem::path PathParameter::filePath(const boost::filesystem::path& baseDir) const
{
  if (isPacked())
    const_cast<PathParameter*>(this)->unpack(baseDir); // does nothing, if already unpacked

  return value_;
}

const boost::filesystem::path &PathParameter::originalFilePath() const
{
  return value_;
}

boost::filesystem::path PathParameter::fileName() const
{
  return value_.filename();
}


void PathParameter::setOriginalFilePath(const boost::filesystem::path &value)
{
  value_=value;
}

std::istream& PathParameter::stream() const
{
  if (isPacked())
  {
    file_content_stream_.reset(new std::istringstream(*file_content_));
  }
  else
  {
    file_content_stream_.reset(new std::ifstream(value_.c_str()));
    if (! (*file_content_stream_) )
    {
      throw insight::Exception("Could not open file "+value_.string()+" for reading!");
    }
  }

  return *file_content_stream_;
}


bool PathParameter::isPacked() const
{
  return !file_content_;
}

void PathParameter::pack()
{
  // read and store file

  if (exists(value_))
  {
    // read raw file into buffer
    std::cout<<"reading content of file "<<value_<<std::endl;
    std::ifstream in(value_.c_str());
    istreambuf_iterator<char> inputBegin(in), inputEnd;
    file_content_.reset(new std::string);
    std::back_insert_iterator<string> stringInsert(*file_content_);
    copy(inputBegin, inputEnd, stringInsert);

    // compute hash
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, file_content_->data(), file_content_->size());
    MD5_Final(file_content_hash_, &ctx);
  }
}

void PathParameter::unpack(const boost::filesystem::path& basePath)
{

  if (!value_.empty())
  {
    bool needUnpack = false;
    if (!exists(value_)) // unpack only, if it is not yet there (e.g. already unpacked)
    {
      auto dirHash = std::hash<std::string>()(value_.parent_path().string());
      boost::filesystem::path unpackPath =
          basePath / boost::lexical_cast<std::string>(dirHash);
      // extract file, create parent path.
      if (!exists(value_.parent_path()) )
      {
        boost::filesystem::create_directories( value_.parent_path() );
      }

//      // typedefs
//      using namespace boost::archive::iterators;
//      typedef
//        transform_width<
//            binary_from_base64<
//            remove_whitespace
//             <std::string::const_iterator> >, 8, 6>
//          base64_text; // compose all the above operations in to a new iterator

//      long paddChars = count(file_content_.begin(), file_content_.end(), '=');
//      std::replace(file_content_.begin(), file_content_.end(), '=', 'A');

//      std::string output(
//            base64_text(file_content_.begin()),
//            base64_text(file_content_.end())
//            );
//      output.erase(output.end() - paddChars, output.end());

      std::ofstream file( value_.c_str(), ios::out | ios::binary);
      if (file.good())
      {
          file.write(file_content_->c_str(), long(file_content_->size()) );
          file.close();
      }

    }
  }
}


void PathParameter::clearPackedData()
{
  file_content_.reset();
}

//template <typename char_type>
//struct ostreambuf
//    : public std::basic_streambuf<char_type, std::char_traits<char_type> >
//{
//    ostreambuf(char_type* buffer, std::streamsize bufferLength)
//    {
//        // set the "put" pointer the start of the buffer and record it's length.
//        this->setp(buffer, buffer + bufferLength);
//    }
//};

rapidxml::xml_node<>* PathParameter::appendToNode
(
  const std::string& name, 
  rapidxml::xml_document<>& doc, 
  rapidxml::xml_node<>& node, 
  boost::filesystem::path inputfilepath
) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    std::string relpath="";
    if (!value_.empty())
    {
      relpath=make_relative(inputfilepath, value_).string();
    }
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(relpath.c_str())
    ));

    if (isPacked())
    {

      // ===========================================================================================
      // 1.) do base64 encode
      // typedefs
      using namespace boost::archive::iterators;
      typedef
//          insert_linebreaks<         // insert line breaks every 72 characters
              base64_from_binary<    // convert binary values to base64 characters
                  transform_width<   // retrieve 6 bit integers from a sequence of 8 bit bytes
                      const char*, 6, 8
                  >
              >
//              ,72 >
          base64_enc; // compose all the above operations in to a new iterator


//      // read raw file into buffer
//      std::ifstream in(value_.c_str());
//      std::string raw_data ( static_cast<std::stringstream const&>(std::stringstream() << in.rdbuf()).str() );


      // base64-encode
//      unsigned int writePaddChars = (3-raw_data.length()%3)%3;

      char tail[3] = {0,0,0};
      size_t len=file_content_->size();
      uint one_third_len = len/3;
      uint len_rounded_down = one_third_len*3;
      uint j = len_rounded_down + one_third_len;
      unsigned int base64length = ((4 * file_content_->size() / 3) + 3) & ~3;

      auto *xml_content = doc.allocate_string(0, base64length+1);
      std::copy(base64_enc(file_content_->c_str()), base64_enc(file_content_->c_str()+len_rounded_down), xml_content);

      if (len_rounded_down != len)
      {
          uint i=0;
          for(; i < len - len_rounded_down; ++i)
          {
              tail[i] = (*file_content_)[len_rounded_down+i];
          }

          std::copy(base64_enc(tail), base64_enc(tail + 3), xml_content + j);

          for(i=len + one_third_len + 1; i < j+4; ++i)
          {
              xml_content[i] = '=';
          }
      }

//      ostreambuf<char> ostreamBuffer(xml_content, base64length);
//      std::ostream os(&ostreamBuffer);
//      std::copy(
//            base64_text(file_content_.begin()),
//            base64_text(file_content_.end()),
//            std::ostream_iterator<char>(os)
//         );
//      os<<std::string(writePaddChars, '=');

      // ===========================================================================================
      // 2.) append to node
      child->append_attribute(doc.allocate_attribute
      (
        "content",
        xml_content
      ));

    }
    return child;
  
}

void PathParameter::readFromNode
(
  const std::string& name, 
  rapidxml::xml_document<>&,
  rapidxml::xml_node<>& node, 
  boost::filesystem::path inputfilepath
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    boost::filesystem::path abspath(child->first_attribute("value")->value());
    if (!abspath.empty())
    {
      if (abspath.is_relative())
      {
        abspath = boost::filesystem::absolute(inputfilepath / abspath);
      }
  #if BOOST_VERSION < 104800
  #warning Conversion into canonical paths disabled!
  #else
      if (boost::filesystem::exists(abspath)) // avoid throwing errors during read of parameterset
	  abspath=boost::filesystem::canonical(abspath);
  #endif
    }
    value_=abspath;

    if (auto* a = child->first_attribute("content"))
    {
      file_content_.reset(new std::string);

        const char *src = a->value();

        unsigned long size = strlen(src);
        if (size && src[size - 1] == '=') {
          --size;
          if (size && src[size - 1] == '=') --size;
        }
        if (size == 0)
        {
          file_content_->clear();
        }
        else
        {
          file_content_->resize(3*size/4);
          using namespace boost::archive::iterators;
          typedef

            transform_width<
                binary_from_base64<
                  const char*>, 8, 6>

                base64_dec;

            base64_dec src_it(src);
            for(size_t i=0; i < file_content_->size(); ++i)
            {
                (*file_content_)[i] = *src_it;
//                std::cout<<" "<<i<<":"<<file_content_[i];
                ++src_it;
            }
//            std::copy(base64_dec(src), base64_dec(src + size),
//                        file_content_.begin());
        }
    }
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % value_.string()
           )
        );
  }
}

PathParameter *PathParameter::clonePathParameter() const
{
  return new PathParameter(value_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_, file_content_);
}

Parameter* PathParameter::clone() const
{
  return clonePathParameter();
}

void PathParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const PathParameter*>(&p))
  {
    Parameter::reset(p);
    file_content_=op->file_content_;
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}

void PathParameter::operator=(const PathParameter &op)
{
  description_ = op.description_;
  isHidden_ = op.isHidden_;
  isExpert_ = op.isExpert_;
  isNecessary_ = op.isNecessary_;
  order_ = op.order_;

  value_ = op.value_;
  file_content_ = op.file_content_;
  memcpy(file_content_hash_, op.file_content_hash_, sizeof(op.file_content_hash_));
}




defineType(DirectoryParameter);
addToFactoryTable(Parameter, DirectoryParameter);

DirectoryParameter::DirectoryParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(".", description, isHidden, isExpert, isNecessary, order)
{}

DirectoryParameter::DirectoryParameter(const boost::filesystem::path& value, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: PathParameter(value, description, isHidden, isExpert, isNecessary, order)
{}

std::string DirectoryParameter::latexRepresentation() const
{
    return std::string() 
      + "{\\ttfamily "
      + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(value_)) ).toLaTeX()
      + "}";
}

//std::string DirectoryParameter::plainTextRepresentation(int indent) const
//{
//    return std::string(indent, ' ')
//      + "\""
//      + SimpleLatex( boost::lexical_cast<std::string>(boost::filesystem::absolute(value_)) ).toPlainText()
//      + "\"\n";
//}


rapidxml::xml_node<>* DirectoryParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      doc.allocate_string(value_.c_str())
    ));
    return child;
}

void DirectoryParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_document<>&,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    value_=boost::filesystem::path(child->first_attribute("value")->value());
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % value_.string()
           )
        );
  }}



Parameter* DirectoryParameter::clone() const
{
  return new DirectoryParameter(value_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}


void DirectoryParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const DirectoryParameter*>(&p))
  {
    PathParameter::reset(p);
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}



defineType(SelectionParameter);
addToFactoryTable(Parameter, SelectionParameter);

SelectionParameter::SelectionParameter( const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: SimpleParameter< int , IntName>(-1, description, isHidden, isExpert, isNecessary, order)
{
}

SelectionParameter::SelectionParameter(const int& value, const SelectionParameter::ItemList& items, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: SimpleParameter< int , IntName>(value, description, isHidden, isExpert, isNecessary, order),
  items_(items)
{
}

SelectionParameter::SelectionParameter(const std::string& key, const SelectionParameter::ItemList& items, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: SimpleParameter< int , IntName>(0, description, isHidden, isExpert, isNecessary, order),
  items_(items)
{
  ItemList::const_iterator i=std::find(items_.begin(), items_.end(), key);
  if (i!=items_.end()) 
  {
    value_ = int( i - items_.begin() );
  }
  else
    value_ = 0;
}

SelectionParameter::~SelectionParameter()
{
}

const SelectionParameter::ItemList& SelectionParameter::items() const
{ 
  return items_;
}

std::string SelectionParameter::latexRepresentation() const
{
  return SimpleLatex(items_[size_t(value_)]).toLaTeX();
}

std::string SelectionParameter::plainTextRepresentation(int) const
{
  return SimpleLatex(items_[size_t(value_)]).toPlainText();
}


rapidxml::xml_node<>* SelectionParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    child->append_attribute(doc.allocate_attribute
    (
      "value", 
      //doc.allocate_string( boost::lexical_cast<std::string>(value_).c_str() )
      doc.allocate_string( items_[size_t(value_)].c_str() )
    ));
    return child;
}

void SelectionParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_document<>&,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    //value_=boost::lexical_cast<int>(child->first_attribute("value")->value());
    string key=child->first_attribute("value")->value();
    ItemList::const_iterator i=std::find(items_.begin(), items_.end(), key);
    if (i != items_.end())
    {
      value_ = int( i - items_.begin() );
    }
    else
    {
      try
      {
	int v=boost::lexical_cast<int>(key);
	value_=v;
      }
      catch(...)
      {
        throw insight::Exception("Invalid selection value ("+key+") in parameter "+name);
      }
    }
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation(0)
           )
        );
  }
}



Parameter* SelectionParameter::clone() const
{
  return new SelectionParameter(value_, items_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}


void SelectionParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const SelectionParameter*>(&p))
  {
    IntParameter::reset(p);
    items_ = op->items_;
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}




defineType(DoubleRangeParameter);
addToFactoryTable(Parameter, DoubleRangeParameter);

DoubleRangeParameter::DoubleRangeParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

DoubleRangeParameter::DoubleRangeParameter(const RangeList& value, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  values_(value)
{
}


DoubleRangeParameter::DoubleRangeParameter(double defaultFrom, double defaultTo, int defaultNum, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
  if (defaultNum==1)
    insertValue(defaultFrom);
  else
  {
    for(int i=0; i<defaultNum; i++)
    {
      insertValue( defaultFrom + (defaultTo-defaultFrom)*double(i)/double(defaultNum-1) );
    }
  }
}

DoubleRangeParameter::~DoubleRangeParameter()
{}

std::string DoubleRangeParameter::latexRepresentation() const
{
  std::ostringstream oss;
  oss << *values_.begin();
  for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
  {
    oss<<"; "<<*i;
  }
  return oss.str();
}

std::string DoubleRangeParameter::plainTextRepresentation(int indent) const
{
  std::ostringstream oss;
  oss << *values_.begin();
  for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
  {
    oss<<"; "<<*i;
  }
  return std::string(indent, ' ') + oss.str() + '\n';
}

DoubleParameter* DoubleRangeParameter::toDoubleParameter(RangeList::const_iterator i) const
{
  return new DoubleParameter(*i, "realized from range iterator");
}


rapidxml::xml_node<>* DoubleRangeParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
    using namespace rapidxml;
    xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
    
    std::ostringstream oss;
    oss << *values_.begin();
    for ( RangeList::const_iterator i=(++values_.begin()); i!=values_.end(); i++ )
    {
      oss<<" "<<*i;
    }
    child->append_attribute(doc.allocate_attribute
    (
      "values", 
      doc.allocate_string(oss.str().c_str())
    ));
    return child;
}

void DoubleRangeParameter::readFromNode
(
    const std::string& name,
    rapidxml::xml_document<>&,
    rapidxml::xml_node<>& node,
    boost::filesystem::path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    values_.clear();
    std::istringstream iss(child->first_attribute("values")->value());
    while (!iss.eof())
    {
      double v;
      iss >> v;
      if (iss.fail()) break;
      values_.insert(v);
    }
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation()
           )
        );
  }
}


Parameter* DoubleRangeParameter::clone() const
{
  return new DoubleRangeParameter(values_, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
}



void DoubleRangeParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const DoubleRangeParameter*>(&p))
  {
    Parameter::reset(p);
    values_ = op->values_;
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}


defineType(ArrayParameter);
addToFactoryTable(Parameter, ArrayParameter);

ArrayParameter::ArrayParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  defaultSize_(0)
{
}

ArrayParameter::ArrayParameter(const Parameter& defaultValue, int size, const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  defaultValue_(defaultValue.clone()),
  defaultSize_(size)
{
  for (int i=0; i<size; i++) appendEmpty();
}
  
  
std::string ArrayParameter::latexRepresentation() const
{
  return std::string();
}

std::string ArrayParameter::plainTextRepresentation(int) const
{
  return std::string();
}

bool ArrayParameter::isPacked() const
{
  bool is_packed=false;
  for (const auto& p: value_)
  {
    is_packed |= p->isPacked();
  }
  return is_packed;
}

void ArrayParameter::pack()
{
  for (auto& p: value_)
  {
    p->pack();
  }
}

void ArrayParameter::unpack(const boost::filesystem::path& basePath)
{
  for (auto& p: value_)
  {
    p->unpack(basePath);
  }
}

void ArrayParameter::clearPackedData()
{
  for (auto& p: value_)
  {
    p->clearPackedData();
  }
}


rapidxml::xml_node<>* ArrayParameter::appendToNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath) const
{
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);
  defaultValue_->appendToNode("default", doc, *child, inputfilepath);
  for (int i=0; i<size(); i++)
  {
    value_[i]->appendToNode(boost::lexical_cast<std::string>(i), doc, *child, inputfilepath);
  }
  return child;
}

void ArrayParameter::readFromNode(const std::string& name, rapidxml::xml_document<>& doc, rapidxml::xml_node<>& node, 
    boost::filesystem::path inputfilepath)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());

  std::vector<std::pair<double, ParameterPtr> > readvalues;

  if (child)
  {
    value_.clear();
    for (xml_node<> *e = child->first_node(); e; e = e->next_sibling())
    {
      std::string name(e->first_attribute("name")->value());
      if (name=="default")
      {
        defaultValue_->readFromNode( name, doc, *child, inputfilepath );
      }
      else
      {
        int i=boost::lexical_cast<int>(name);
        ParameterPtr p(defaultValue_->clone());
        p->readFromNode( name, doc, *child, inputfilepath );

        readvalues.push_back( decltype(readvalues)::value_type(i, p) );
      }
    }

    sort(readvalues.begin(), readvalues.end(),
         [](const decltype(readvalues)::value_type& v1, const decltype(readvalues)::value_type& v2)
            {
                return v1.first < v2.first;
            }
    );

    for (const auto& v: readvalues)
    {
        value_.push_back(v.second);
    }
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation()
           )
        );
  }
}



Parameter* ArrayParameter::clone () const
{
  ArrayParameter* np=new ArrayParameter(*defaultValue_, 0, description_.simpleLatex(), isHidden_, isExpert_, isNecessary_, order_);
  for (int i=0; i<size(); i++)
  {
    np->appendValue( *(value_[i]) );
  }
  return np;
}


void ArrayParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const ArrayParameter*>(&p))
  {
    Parameter::reset(p);
    defaultValue_.reset( op->defaultValue_->clone() );
    defaultSize_ = op->defaultSize_;
    value_.clear();
    for (const auto& v: op->value_)
      value_.push_back( ParameterPtr(v->clone()) );
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}

  
  
  
defineType(MatrixParameter);
addToFactoryTable(Parameter, MatrixParameter);

MatrixParameter::MatrixParameter(const std::string& description,  bool isHidden, bool isExpert, bool isNecessary, int order)
: Parameter(description, isHidden, isExpert, isNecessary, order)
{
}

MatrixParameter::MatrixParameter
(
  const arma::mat& defaultValue, 
  const string& description,  bool isHidden, bool isExpert, bool isNecessary, int order
)
: Parameter(description, isHidden, isExpert, isNecessary, order),
  value_(defaultValue)
{}


arma::mat& MatrixParameter::operator()()
{
  return value_;
}

const arma::mat& MatrixParameter::operator()() const
{
  return value_;
}

string MatrixParameter::latexRepresentation() const
{
  std::ostringstream oss;
  
  oss<<"\\begin{tabular}{l";
  for (arma::uword j=0; j<value_.n_cols; j++) oss<<'c'<<endl;
  oss<<"}\n";
  
  for (arma::uword i=0;i<value_.n_rows; i++)
  {
    oss<<i<<"&";
    for (arma::uword j=0;j<value_.n_cols; j++)
    {
      oss<<value_(i,j);
      if (j<value_.n_cols-1) oss<<"&";
    }
    oss<<"\\\\"<<endl;
  }
  oss<<"\\end{tabular}"<<endl;
  
  return oss.str();
}


string MatrixParameter::plainTextRepresentation(int indent) const
{
  std::ostringstream oss;

  for (arma::uword i=0;i<value_.n_rows; i++)
  {
    oss<<string(size_t(indent), ' ')<<i<<": ";
    for (arma::uword j=0;j<value_.n_cols; j++)
    {
      oss<<value_(i,j);
      if (j<value_.n_cols-1) oss<<", ";
    }
    oss<<"\n"<<endl;
  }

  return oss.str();
}

xml_node< char >* MatrixParameter::appendToNode(const string& name, xml_document< char >& doc, xml_node< char >& node, path inputfilepath) const
{
  using namespace rapidxml;
  xml_node<>* child = Parameter::appendToNode(name, doc, node, inputfilepath);

//   std::ostringstream voss;
//   value_.save(voss, arma::raw_ascii);
//   
//   // set stringified table values as node value
//   child->value(doc.allocate_string(voss.str().c_str()));
  writeMatToXMLNode(value_, doc, *child);
    
  return child;
}

void MatrixParameter::readFromNode
(
    const string& name,
    xml_document< char >&,
    xml_node< char >& node,
    path
)
{
  using namespace rapidxml;
  xml_node<>* child = findNode(node, name, type());
  if (child)
  {
    string value_str=child->value();
    std::istringstream iss(value_str);
    value_.load(iss, arma::raw_ascii);
  }
  else
  {
    insight::Warning(
          boost::str(
            boost::format(
             "No xml node found with type '%s' and name '%s', default value '%s' is used."
             ) % type() % name % plainTextRepresentation()
           )
        );
  }
}

Parameter* MatrixParameter::clone() const
{
  return new MatrixParameter(value_, description_.simpleLatex());
}



void MatrixParameter::reset(const Parameter& p)
{
  if (const auto* op = dynamic_cast<const MatrixParameter*>(&p))
  {
    Parameter::reset(p);
    value_ = op->value_;
  }
  else
    throw insight::Exception("Tried to set a "+type()+" from a different type ("+p.type()+")!");
}


}
