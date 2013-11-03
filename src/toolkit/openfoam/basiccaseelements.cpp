/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "basiccaseelements.h"

using namespace std;

namespace insight
{
  
FVNumerics::FVNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "FVNumerics")
{
}

void FVNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["writeControl"]="timeStep";
  controlDict["writeInterval"]=100;
  controlDict["purgeWrite"]=10;
  controlDict["writeFormat"]="binary";
  controlDict["writePrecision"]=6;
  controlDict["writeCompression"]="compressed";
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.addListIfNonexistent("libs");
  controlDict.addSubDictIfNonexistent("functions");
  
  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  fvSolution.addSubDictIfNonexistent("solvers");
  fvSolution.addSubDictIfNonexistent("relaxationFactors");
  
  OFDictData::dict& fvSchemes=dictionaries.addDictionaryIfNonexistent("system/fvSchemes");
  fvSchemes.addSubDictIfNonexistent("ddtSchemes");
  fvSchemes.addSubDictIfNonexistent("gradSchemes");
  fvSchemes.addSubDictIfNonexistent("divSchemes");
  fvSchemes.addSubDictIfNonexistent("laplacianSchemes");
  fvSchemes.addSubDictIfNonexistent("interpolationSchemes");
  fvSchemes.addSubDictIfNonexistent("snGradSchemes");
  fvSchemes.addSubDictIfNonexistent("fluxRequired");
}

OFDictData::dict GAMGSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="GAMG";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["smoother"]="DICGaussSeidel";
  d["nPreSweeps"]=0;
  d["nPostSweeps"]=2;
  d["cacheAgglomeration"]="on";
  d["agglomerator"]="faceAreaPair";
  d["nCellsInCoarsestLevel"]=10;
  d["mergeLevels"]=1;
  return d;
}

OFDictData::dict smoothSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="smoothSolver";
  d["smoother"]="GaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["nSweeps"]=1;
  return d;
}


simpleFoamNumerics::simpleFoamNumerics(OpenFOAMCase& c)
: FVNumerics(c)
{
}
 
void simpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="simpleFoam";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1.0;
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGSolverSetup(1e-7, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-8, 0.1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["p"]=0.3;
    relax["U"]=0.7;
    relax["k"]=0.7;
    relax["omega"]=0.7;
    relax["epsilon"]=0.7;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    fieldRelax["p"]=0.3;
    eqnRelax["U"]=0.7;
    eqnRelax["k"]=0.7;
    eqnRelax["omega"]=0.7;
    eqnRelax["epsilon"]=0.7;
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";
  if (OFversion()<=160) suf=" grad(U)"; else suf=" cellLimited leastSquares 1";
  div["default"]=pref+"Gauss upwind";
  div["div(phi,U)"]=pref+"Gauss linearUpwindV"+suf;
  if (OFversion()>=210)
  {
    div["div((nuEff*dev(T(grad(U)))))"]=pref+"Gauss linear";
  }
  else 
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}


transportModel::transportModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "transportModel")
{
}

singlePhaseTransportProperties::singlePhaseTransportProperties(OpenFOAMCase& c)
: transportModel(c)
{
}
 
void singlePhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["transportModel"]="Newtonian";
  transportProperties["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimensionSet(0, 2, -1), OFDictData::data(1e-6));
}

turbulenceModel::turbulenceModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "turbulenceModel")
{
}

kOmegaSST_RASModel::kOmegaSST_RASModel(OpenFOAMCase& c)
: turbulenceModel(c)
{
}
  
void kOmegaSST_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSSTCoeffs");
}

BoundaryCondition::BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"BC"),
  patchName_(patchName),
  type_(boundaryDict.subDict(patchName).getString("type")),
  nFaces_(boundaryDict.subDict(patchName).getInt("nFaces")),
  startFace_(boundaryDict.subDict(patchName).getInt("startFace"))  
{
}

void BoundaryCondition::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["type"]=type_;
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
}

void BoundaryCondition::addIntoFieldDictionaries(OFdicts& dictionaries, const std::string& fieldName) const
{
  OFDictData::dict& fieldDict=dictionaries.addDictionaryIfNonexistent("0/"+fieldName);
  
}

void BoundaryCondition::addIntoDictionaries(OFdicts& dictionaries) const
{
  //addIntoFieldDictionaries(dictionaries);
  OFDictData::dict bndsubd;
  addOptionsToBoundaryDict(bndsubd);
  
  OFDictData::dict& boundaryDict=dictionaries.addDictionaryIfNonexistent("constant/polyMesh/boundary");
  if (boundaryDict.size()==0)
    boundaryDict.addListIfNonexistent("");
  
  OFDictData::list& bl=
    *boost::get<OFDictData::list>( &boundaryDict.begin()->second );
  
  // search, if patchname is already present; replace, if yes
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (std::string *name = boost::get<std::string>(&(*i)))
    {
      cout<<"found "<<*name<<endl;
      if ( *name == patchName_ )
      {
	i++;
	*i=bndsubd;
	return;
      }
    }
  }
  
  // not found, append
  bl.push_back( OFDictData::data(patchName_) );
  bl.push_back( bndsubd );
}

WallBC::WallBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: BoundaryCondition(c, patchName, boundaryDict)
{
}

void WallBC::addIntoFieldDictionaries(OFdicts& dictionaries, const std::string& fieldName) const
{
}

void WallBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  BoundaryCondition::addOptionsToBoundaryDict(bndDict);
}

}

