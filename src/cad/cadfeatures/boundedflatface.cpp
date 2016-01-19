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

#include "boundedflatface.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(BoundedFlatFace);
addToFactoryTable(Feature, BoundedFlatFace, NoParameters);


BoundedFlatFace::BoundedFlatFace(const NoParameters&)
{}

BoundedFlatFace::BoundedFlatFace(const std::vector<FeaturePtr>& edges)
: edges_(edges)
{}

BoundedFlatFace::BoundedFlatFace(const std::vector<FeatureSetPtr>& edges)
: edges_(edges)
{}

void BoundedFlatFace::build()
{
  if 
  (
    const std::vector<FeaturePtr>* edgesPtr 
     = boost::get<std::vector<FeaturePtr> >(&edges_) 
  )
  {
    const std::vector<FeaturePtr>& edges=*edgesPtr;
    
    TopTools_ListOfShape edgs;
    
    int n_ok=0, n_nok=0;
    BOOST_FOREACH(const FeaturePtr& m, edges)
    {
      if (m->isSingleEdge())
      {
	TopoDS_Edge e=m->asSingleEdge();
	edgs.Append(e);
	n_ok++;
      }
      else if (m->isSingleWire())
      {
	TopoDS_Wire wire=m->asSingleWire();
	for (TopExp_Explorer ex(wire, TopAbs_EDGE); ex.More(); ex.Next())
	{
	  TopoDS_Edge e=TopoDS::Edge(ex.Current());
	  edgs.Append(e);
	}
	n_ok++;
      }
      else n_nok++;
    }

    if (n_ok==0)
      throw insight::Exception("No valid edge given!");
    if (n_nok>0)
      insight::Warning(str(format("Only %d out of %d given edges were valid!") % n_ok % (n_ok+n_nok)));

    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);
    
    BRepBuilderAPI_MakeFace fb(w.Wire(), true);
    if (!fb.IsDone())
      throw insight::Exception("Failed to generate planar face!");
    
    ShapeFix_Face FixShape;
    FixShape.Init(fb.Face());
    FixShape.Perform();
    
    setShape(FixShape.Face());
  }
  else if 
  (
    const std::vector<FeatureSetPtr>* edgesPtr 
     = boost::get<std::vector<FeatureSetPtr> >(&edges_) 
  )
  {
    const std::vector<FeatureSetPtr>& edges=*edgesPtr;

    TopTools_ListOfShape edgs;
    BOOST_FOREACH(const FeatureSetPtr& m, edges)
    {
      BOOST_FOREACH(const FeatureID& fi, m->data())
      {
	edgs.Append(m->model()->edge(fi));
      }
    }
    
    BRepBuilderAPI_MakeWire w;
    w.Add(edgs);

    BRepBuilderAPI_MakeFace fb(w.Wire(), true);
    if (!fb.IsDone())
      throw insight::Exception("Failed to generate planar face!");

    ShapeFix_Face FixShape;
    FixShape.Init(fb.Face());
    FixShape.Perform();
    
    setShape(FixShape.Face());
  }
}

BoundedFlatFace::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}


void BoundedFlatFace::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "BoundedFlatFace",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ( ruleset.r_solidmodel_expression % ',' ) >> ')' )
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<BoundedFlatFace>(qi::_1)) ]
    |
    ( '(' >> ( ruleset.r_edgeFeaturesExpression % ',' ) >> ')' )
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<BoundedFlatFace>(qi::_1)) ]
      
    ))
  );
}

}
}