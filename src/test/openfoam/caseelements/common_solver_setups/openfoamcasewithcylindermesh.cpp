#include "openfoamcasewithcylindermesh.h"

#include "openfoam/openfoamtools.h"
#include "openfoam/blockmesh_templates.h"
#include "openfoam/caseelements/numerics/meshingnumerics.h"
#include "openfoam/caseelements/boundaryconditions/velocityinletbc.h"
#include "openfoam/caseelements/boundaryconditions/pressureoutletbc.h"
#include "openfoam/caseelements/boundaryconditions/wallbc.h"

OpenFOAMCaseWithCylinderMesh::OpenFOAMCaseWithCylinderMesh(const string &OFEname)
    : OpenFOAMTestCase(OFEname),
      flowDir_(vec3(0,0,1)),
      dir_(false)
{}

void OpenFOAMCaseWithCylinderMesh::createInletBC(OFDictData::dict& bd)
{
    VelocityInletBC::Parameters p;
    p.velocity.fielddata=VelocityInletBC::Parameters::velocity_type::fielddata_uniformSteady_type{ 1.0*flowDir_ };
    insert(new VelocityInletBC(*this, "inlet", bd, p));
}

void OpenFOAMCaseWithCylinderMesh::createOutletBC(OFDictData::dict& bd)
{
    PressureOutletBC::Parameters p;
    arma::mat p0;
    p0 << 1e5;
    PressureOutletBC::Parameters::behaviour_uniform_type punif;
    punif.pressure.fielddata=PressureOutletBC::Parameters::behaviour_uniform_type::pressure_type::fielddata_uniformSteady_type{ p0 };
    p.behaviour=punif;
    insert(new PressureOutletBC(*this, "outlet", bd, p));
}

void OpenFOAMCaseWithCylinderMesh::createWallBC(OFDictData::dict& bd)
{
    insert(new WallBC(*this, "walls", bd));
}

void OpenFOAMCaseWithCylinderMesh::runTest()
{
  {
    OpenFOAMCase meshCase(ofe());

    meshCase.insert(new MeshingNumerics(meshCase));

    bmd::blockMeshDict_Cylinder::Parameters mp;
    mp.mesh.basePatchName="inlet";
    mp.mesh.topPatchName="outlet";
    mp.mesh.resolution = bmd::blockMeshDict_Cylinder::Parameters::mesh_type::resolution_cubical_type{9};
    meshCase.insert(new bmd::blockMeshDict_Cylinder(meshCase, mp));

    meshCase.createOnDisk(dir_);
    meshCase.executeCommand(dir_, "blockMesh");
  }

  OFDictData::dict boundaryDict;
  parseBoundaryDict(dir_, boundaryDict);

  createInletBC(boundaryDict);
  createOutletBC(boundaryDict);
  createWallBC(boundaryDict);

  createOnDisk(dir_);
  executeCommand(dir_, readSolverName(dir_));
}
