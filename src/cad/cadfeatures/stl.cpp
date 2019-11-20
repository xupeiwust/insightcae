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
 */

#include "stl.h"

#include "StlAPI.hxx"

#include <StlAPI_Reader.hxx>

#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <gp_Pnt.hxx>
#include <OSD_Path.hxx>
#include <RWStl.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <AIS_ColoredShape.hxx>
#include <TColStd_HPackedMapOfInteger.hxx>
#include <Geom_SphericalSurface.hxx>
#include "Graphic3d_MaterialAspect.hxx"
#include "AIS_Triangulation.hxx"
#include "Poly.hxx"
#include "XSDRAWSTLVRML_DataSource.hxx"
#include "MeshVS_MeshPrsBuilder.hxx"
#include "MeshVS_ElementalColorPrsBuilder.hxx"
#include "MeshVS_Drawer.hxx"
#include "MeshVS_DrawerAttribute.hxx"

#include "base/tools.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "transform.h"

#include "base/units.h"

#include "vtkSTLReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkTriangleFilter.h"
#include "vtkCell.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {




defineType(STL);
addToFactoryTable(Feature, STL);


size_t STL::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=fname_;
  if (trsf_) h+=*trsf_;
  if (other_trsf_) h+=*other_trsf_;
  return h.getHash();
}


STL::STL()
{}




STL::STL(const boost::filesystem::path& fname)
  : fname_(fname)
{}




STL::STL(const boost::filesystem::path& fname, const gp_Trsf& trsf)
  : fname_(fname), trsf_(new gp_Trsf(trsf))
{}




STL::STL(const boost::filesystem::path& fname, FeaturePtr other_trsf)
  : fname_(fname), other_trsf_(other_trsf)
{}




FeaturePtr STL::create
(
    const boost::filesystem::path& fname
)
{
  return FeaturePtr(new STL(fname));
}




FeaturePtr STL::create_trsf
(
    const boost::filesystem::path& fname,
    gp_Trsf trsf
    )
{
  return FeaturePtr(new STL(fname, trsf));
}




FeaturePtr STL::create_other
(
    const boost::filesystem::path& fname,
    FeaturePtr other_trsf
    )
{
  return FeaturePtr(new STL(fname, other_trsf));
}


void STL::build()
{
  ExecTimer t("STL::build() ["+featureSymbolName()+"]");

  if (!cache.contains(hash()))
  {
//    std::cout<<"STL was not in cache"<<std::endl;

    vtkSmartPointer<vtkSTLReader> stl = vtkSmartPointer<vtkSTLReader>::New();
    stl->SetFileName(fname_.c_str());

    vtkSmartPointer<vtkPolyDataNormals> split = vtkSmartPointer<vtkPolyDataNormals>::New();
    split->SetInputConnection(stl->GetOutputPort());
    split->SplittingOn();
    split->ComputeCellNormalsOn();
    split->ConsistencyOn();

    vtkSmartPointer<vtkTriangleFilter> tri = vtkSmartPointer<vtkTriangleFilter>::New();
    tri->SetInputConnection(split->GetOutputPort());

    tri->Update();

    vtkPolyData *split_mesh = tri->GetOutput();
    aSTLMesh_ =
        new Poly_Triangulation
        (
          split_mesh->GetNumberOfPoints(),
          split_mesh->GetNumberOfCells(),
          Standard_False
          );

    for (vtkIdType i = 0; i < split_mesh->GetNumberOfPoints(); i++)
    {
      double xyz[3];
      split_mesh->GetPoint(i, xyz);
      aSTLMesh_->ChangeNode (i+1) = gp_Pnt(xyz[0], xyz[1], xyz[2]);
    }

    for (vtkIdType i = 0; i < split_mesh->GetNumberOfCells(); i++)
    {
      vtkCell *c = split_mesh->GetCell(i);
      insight::assertion( c->GetNumberOfPoints()==3, "STL mesh cell needs to have exactly 3 corners");
      aSTLMesh_->ChangeTriangle (i + 1) =
          Poly_Triangle
          (
            c->GetPointId(0)+1,
            c->GetPointId(1)+1,
            c->GetPointId(2)+1
            );
    }

    // transform points, if required
    if (trsf_ || other_trsf_)
    {
      gp_Trsf tr;
      if (trsf_)
      {
        tr = *trsf_;
      }
      else if (other_trsf_)
      {
        tr = Transform::calcTrsfFromOtherTransformFeature(other_trsf_);
      }

      for (int i=1; i<=aSTLMesh_->NbNodes();i++)
      {
        aSTLMesh_->ChangeNode(i).Transform(tr);
      }
    }


    // get bounding box
    Bnd_Box bb;
    for (int i=1; i<=aSTLMesh_->NbNodes();i++)
    {
      bb.Add(aSTLMesh_->Node(i));
    }
    double r=bb.CornerMax().Distance(bb.CornerMin()) /2.;
    gp_Pnt ctr(0.5*(bb.CornerMin().XYZ()+bb.CornerMax().XYZ()));

    TopoDS_Face aFace;
    BRep_Builder aB;
    //  aB.MakeFace(aFace, aSTLMesh);
    aB.MakeFace(aFace, Handle_Geom_Surface(new Geom_SphericalSurface(gp_Sphere(gp_Ax3(ctr, gp::DZ()), r))), Precision::Confusion());
    aB.UpdateFace(aFace, aSTLMesh_);

    setShape( aFace );

    cache.insert(shared_from_this());
  }
  else
  {
//    std::cout<<"Retrieving STL from cache"<<std::endl;
    this->operator=(*cache.markAsUsed<STL>(hash()));
  }
}


Handle_AIS_InteractiveObject STL::buildVisualization() const
{
  Handle_AIS_InteractiveObject as( new AIS_Shape(shape()) );
  return as;
}



void STL::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
      (
        "STL",
        typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule(
           ( '(' >> ruleset.r_path >> ')' ) [ qi::_val = phx::bind(&STL::create, qi::_1) ]
            |
           ( '(' >> ruleset.r_path >> ',' >> ruleset.r_solidmodel_expression >> ')' ) [ qi::_val = phx::bind(&STL::create_other, qi::_1, qi::_2) ]
          ))
      );
}



FeatureCmdInfoList STL::ruleDocumentation() const
{
  return boost::assign::list_of
      (
        FeatureCmdInfo
        (
          "STL",

          "( <path:filename> [, <feature:other transform feature> ] )",

          "Import a triangulated surface for display. The result can only be used for display, no operations can be performed on it."
          "Transformations can be reused from other transform features. The name of another transformed feature can be provided optionally."
          )
        );
}



}
}
