#include "ggibc.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"

namespace insight {


defineType(GGIBC);
addToFactoryTable(BoundaryCondition, GGIBC);
addToStaticFunctionTable(BoundaryCondition, GGIBC, defaultParameters);

GGIBC::GGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
        const ParameterSet&ps )
: GGIBCBase(c, patchName, boundaryDict, ps),
  p_(ps)
{
}

void GGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch;
    bndDict["matchTolerance"]= 0.001;
    bndDict["lowWeightCorrection"]=0.1;
    //bndDict["transform"]= "rotational";
  }
  else
  {
    bndDict["type"]="ggi";
    bndDict["shadowPatch"]= p_.shadowPatch;
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset);
    bndDict["bridgeOverlap"]=p_.bridgeOverlap;
    bndDict["zone"]=p_.zone;
  }
}

void GGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);

  for (const FieldList::value_type& field: OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);

    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      MeshMotionBC::noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
        BC["type"]=OFDictData::data("cyclicAMI");
      else
        BC["type"]=OFDictData::data("ggi");
    }
  }
}


} // namespace insight
