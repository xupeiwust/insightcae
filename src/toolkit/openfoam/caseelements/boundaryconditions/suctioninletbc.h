#ifndef INSIGHT_SUCTIONINLETBC_H
#define INSIGHT_SUCTIONINLETBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

namespace insight {

namespace OFDictData { class dict; }

class SuctionInletBC
    : public BoundaryCondition
{
public:
#include "suctioninletbc__SuctionInletBC__Parameters.h"
/*
PARAMETERSET>>> SuctionInletBC Parameters

pressure = double 0.0 "Total pressure at boundary"
rho = double 1025.0 "Density at boundary"
T = double 300.0 "Total temperature at boundary"
gamma = double 1.0 "Ratio of specific heats at boundary"
phiName = string "phi" "Name of flux field"
psiName = string "none" "Name of compressibility field"
rhoName = string "none" "Name of density field"
UName = string "U" "Name of velocity field"

turb_I = double 0.05 "[-] turbulence intensity at inflow"
turb_L = double 0.1 "[m] turbulent length scale at inflow"

phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType ( "SuctionInletBC" );
    SuctionInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet&ps = Parameters::makeDefault()
    );

    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
};



} // namespace insight

#endif // INSIGHT_SUCTIONINLETBC_H
