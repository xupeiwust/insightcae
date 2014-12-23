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

#include "geotest.h"

#include <memory>
#include "base/linearalgebra.h"
#include <base/exception.h>
#include "boost/foreach.hpp"
#include <boost/iterator/counting_iterator.hpp>
#include "boost/lexical_cast.hpp"

#include "featurefilter.h"
#include "solidmodel.h"

#include "gp_Cylinder.hxx"

#define BOOST_SPIRIT_USE_PHOENIX_V3
// #define BOOST_SPIRIT_DEBUG

#include "boost/filesystem.hpp"
#include <boost/fusion/include/std_pair.hpp>
#include "boost/tuple/tuple.hpp"
#include "boost/fusion/tuple.hpp"
#include <boost/fusion/adapted/boost_tuple.hpp>
#include "boost/variant.hpp"
#include "boost/ptr_container/ptr_map.hpp"
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/spirit/include/qi.hpp"
#include "boost/variant/recursive_variant.hpp"
#include "boost/spirit/repository/include/qi_confix.hpp"
#include <boost/spirit/include/qi_eol.hpp>
#include <boost/spirit/include/phoenix.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/function/adapt_callable.hpp>
#include <boost/spirit/include/qi_no_case.hpp>
#include <boost/spirit/home/classic/utility/distinct.hpp>
#include <boost/fusion/adapted.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/karma.hpp>

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


std::ostream& operator<<(std::ostream& os, const FeatureSet& fs)
{
  os<<fs.size()<<" {";
  BOOST_FOREACH(int fi, fs)
  {
    os<<" "<<fi;
  }
  os<<" }";
  return os;
}

Filter::Filter()
: model_(NULL)
{
}

Filter::~Filter()
{
}

void Filter::initialize(const SolidModel& m)
{
  model_=&m;
}

void Filter::firstPass(FeatureID feature)
{
}


AND::AND(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}

void AND::initialize(const SolidModel& m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

bool AND::checkMatch(FeatureID feature) const
{
  return f1_->checkMatch(feature) && f2_->checkMatch(feature);
}

FilterPtr AND::clone() const
{
  return FilterPtr(new AND(*f1_, *f2_));
}



OR::OR(const Filter& f1, const Filter& f2)
: Filter(),
  f1_(f1.clone()), f2_(f2.clone())
{
}

void OR::initialize(const SolidModel& m)
{
  f1_->initialize(m);
  f2_->initialize(m);
}

bool OR::checkMatch(FeatureID feature) const
{
  return f1_->checkMatch(feature) || f2_->checkMatch(feature);
}

FilterPtr OR::clone() const
{
  return FilterPtr(new OR(*f1_, *f2_));
}



NOT::NOT(const Filter& f1)
: Filter(),
  f1_(f1.clone())
{
}
void NOT::initialize(const SolidModel& m)
{
  f1_->initialize(m);
}

bool NOT::checkMatch(FeatureID feature) const
{
  return !f1_->checkMatch(feature);
}

FilterPtr NOT::clone() const
{
  return FilterPtr(new NOT(*f1_));
}

FilterPtr Filter::operator&&(const Filter& f2)
{
  return FilterPtr(new AND(*this, f2));
}

FilterPtr Filter::operator!()
{
  return FilterPtr(new NOT(*this));
}

// ANDFilter operator&&(const Filter& f1, const Filter& f2)
// {
//   return ANDFilter(f1, f2);
// }
// 
// NOTFilter operator!(const Filter& f1)
// {
//   return NOTFilter(f1);
// }


edgeTopology::edgeTopology(GeomAbs_CurveType ct)
: ct_(ct)
{
}

bool edgeTopology::checkMatch(FeatureID feature) const
{
  return model_->edgeType(feature) == ct_;
}

FilterPtr edgeTopology::clone() const
{
  return FilterPtr(new edgeTopology(ct_));
}

faceTopology::faceTopology(GeomAbs_SurfaceType ct)
: ct_(ct)
{
}

bool faceTopology::checkMatch(FeatureID feature) const
{
  return model_->faceType(feature) == ct_;
}

FilterPtr faceTopology::clone() const
{
  return FilterPtr(new faceTopology(ct_));
}

cylFaceOrientation::cylFaceOrientation(bool io)
: io_(io)
{
}

bool cylFaceOrientation::checkMatch(FeatureID feature) const
{
  if (model_->faceType(feature)==GeomAbs_Cylinder)
  {
      GeomAdaptor_Surface adapt(BRep_Tool::Surface(model_->face(feature)));
      gp_Cylinder icyl=adapt.Cylinder();
      gp_Ax1 iax=icyl.Axis();
      BRepGProp_Face prop(model_->face(feature));
      double u1,u2,v1,v2;
      prop.Bounds(u1, u2, v1, v2);
      double u = (u1+u2)/2;
      double v = (v1+v2)/2;
      gp_Vec vec;
      gp_Pnt pnt;
      prop.Normal(u,v,pnt,vec);
      vec.Normalize();
      gp_XYZ dp=pnt.XYZ()-icyl.Location().XYZ();
      gp_XYZ ax=iax.Direction().XYZ();
      ax.Normalize();
      gp_XYZ dr=dp-ax.Multiplied(dp.Dot(ax));
      dr.Normalize();
      
      if (io_)
      {
	if (! (fabs(vec.XYZ().Dot(dr) + 1.) < 1e-6) ) return true;
      }
      else
      {
	if (! (fabs(vec.XYZ().Dot(dr) - 1.) < 1e-6) ) return true;
      }
  }
  else
    return false;
}

FilterPtr cylFaceOrientation::clone() const
{
  return FilterPtr(new cylFaceOrientation(io_));
}

everything::everything()
{}

bool everything::checkMatch(FeatureID feature) const
{
  return true;
}
  
FilterPtr everything::clone() const
{
  return FilterPtr(new everything());
}


edgeCoG::edgeCoG() 
{}

edgeCoG::~edgeCoG()
{}
  
arma::mat edgeCoG::evaluate(FeatureID ei)
{
  return model_->edgeCoG(ei);
}
  
QuantityComputer<arma::mat>::Ptr edgeCoG::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new edgeCoG());
}

faceCoG::faceCoG() 
{}

faceCoG::~faceCoG()
{}
  
arma::mat faceCoG::evaluate(FeatureID ei)
{
  return model_->faceCoG(ei);
}
  
QuantityComputer<arma::mat>::Ptr faceCoG::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new faceCoG());
}


faceNormalVector::faceNormalVector() 
{}

faceNormalVector::~faceNormalVector()
{}
  
arma::mat faceNormalVector::evaluate(FeatureID fi)
{
  return model_->faceNormal(fi);
}
  
QuantityComputer<arma::mat>::Ptr faceNormalVector::clone() const 
{
  return QuantityComputer<arma::mat>::Ptr(new faceNormalVector());
}
  
  
cylRadius::cylRadius() 
{}

cylRadius::~cylRadius()
{}

bool cylRadius::isValidForFeature(FeatureID f) const
{
  return model_->faceType(f) == GeomAbs_Cylinder;
}
  
double cylRadius::evaluate(FeatureID fi)
{
  if (model_->faceType(fi)==GeomAbs_Cylinder)
  {
      GeomAdaptor_Surface adapt(BRep_Tool::Surface(model_->face(fi)));
      gp_Cylinder icyl=adapt.Cylinder();
      return icyl.Radius();
  }
  else return -1.0;
}
  
QuantityComputer<double>::Ptr cylRadius::clone() const 
{
  return QuantityComputer<double>::Ptr(new cylRadius());
}


template<> coincident<Edge>::coincident(const SolidModel& m)
: f_(m.allEdges())
{
}

template<>
bool coincident<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_)
  {
    TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
    TopoDS_Edge e2=TopoDS::Edge(f_.model().edge(f));
    match |= isPartOf(e2, e1);
  }
  
  return match;
}

template<> coincident<Face>::coincident(const SolidModel& m)
: f_(m.allFaces())
{
}

template<>
bool coincident<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_)
  {
    TopoDS_Face e1=TopoDS::Face(model_->face(feature));
    TopoDS_Face e2=TopoDS::Face(f_.model().face(f));
    match |= isPartOf(e2, e1);
  }
  
  return match;
}



template<>
bool secant<Edge>::checkMatch(FeatureID feature) const
{
  TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));

  TopoDS_Vertex v0=TopExp::FirstVertex(e1);
  TopoDS_Vertex v1=TopExp::LastVertex(e1);
  arma::mat v = Vector( BRep_Tool::Pnt(v0).XYZ() - BRep_Tool::Pnt(v1).XYZ() );
  
  return (1.0 - fabs(arma::dot( arma::normalise(v), arma::normalise(dir_) ))) < 1e-10;
}



maximal::maximal(const scalarQuantityComputer& qtc, int rank)
: qtc_(qtc.clone()),
  rank_(rank)
{
}

void maximal::initialize(const SolidModel& m)
{
  Filter::initialize(m);
  qtc_->initialize(m);
}

void maximal::firstPass(FeatureID feature)
{
  if (qtc_->isValidForFeature(feature))
    ranking_[-qtc_->evaluate(feature)].insert(feature);
}

bool maximal::checkMatch(FeatureID feature) const
{
  int j=0;
  for (std::map<double, std::set<FeatureID> >::const_iterator i=ranking_.begin(); i!=ranking_.end(); i++)
  {
    if ((i->second.find(feature)!=i->second.end()) && (j==rank_)) return true;
    j++;
  }
  return false;
}

FilterPtr maximal::clone() const
{
  return FilterPtr(new maximal(*qtc_, rank_));
}

minimal::minimal(const scalarQuantityComputer& qtc, int rank)
: maximal(qtc, rank)
{}

void minimal::firstPass(FeatureID feature)
{
  if (qtc_->isValidForFeature(feature))
    ranking_[qtc_->evaluate(feature)].insert(feature);
}

FilterPtr minimal::clone() const
{
  return FilterPtr(new minimal(*qtc_, rank_));
}


namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace qi;
using namespace phx;
using namespace insight::cad;

typedef std::vector<FeatureSet> FeatureSetList;

FeatureSetPtr lookupFeatureSet(const FeatureSetList& fl, size_t id)
{
  if (id>=fl.size())
    throw insight::Exception
    (
      "Feature set #"+lexical_cast<std::string>(id)
     +" is not present in list of size "+lexical_cast<std::string>(fl.size())
    );
  
  return FeatureSetPtr(new FeatureSet(fl.at(id)));
}
BOOST_PHOENIX_ADAPT_FUNCTION(FeatureSetPtr, lookupFeatureSet_, lookupFeatureSet, 2);

BOOST_PHOENIX_ADAPT_FUNCTION(arma::mat, vec3_, vec3, 3);

template <typename Iterator, typename Skipper = qi::space_type >
struct FeatureFilterExprParser
: qi::grammar<Iterator, FilterPtr(), Skipper>
{

public:
    qi::rule<Iterator, std::string(), Skipper> r_identifier;
    qi::rule<Iterator, FeatureSetPtr(), Skipper> r_featureset;
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter_primary, r_filter_and, r_filter_or;
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter;
    qi::rule<Iterator, FilterPtr(), Skipper> r_qty_comparison;
    qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_expression, r_scalar_primary, r_scalar_term;
    qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_expression, r_mat_primary, r_mat_term;
    
    qi::rule<Iterator, FilterPtr(), Skipper> r_filter_functions;
    qi::rule<Iterator, scalarQuantityComputer::Ptr(), Skipper> r_scalar_qty_functions;
    qi::rule<Iterator, matQuantityComputer::Ptr(), Skipper> r_mat_qty_functions;
    
    const FeatureSetList& externalFeatureSets_;

    FeatureFilterExprParser
    (
      const FeatureSetList& extsets
    )
    : FeatureFilterExprParser::base_type(r_filter),
      externalFeatureSets_(extsets)
    {
	r_identifier = lexeme[ alpha >> *(alnum | char_('_')) >> !(alnum | '_') ];
	r_featureset = lexeme[ '%' >> int_ ] [ _val = phx::construct<FeatureSetPtr>(lookupFeatureSet_(externalFeatureSets_, _1)) ];

        r_filter =  r_filter_or.alias();

	r_filter_or = 
	  ( r_filter_and >> lit("||") > r_filter_and ) [ _val = phx::construct<FilterPtr>(new_<OR>(*_1, *_2)) ] 
	  | r_filter_and [ _val = _1 ]
	  ;
	
	r_filter_and =
	  ( r_filter_primary >> lit("&&") > r_filter_primary ) [ _val = phx::construct<FilterPtr>(new_<AND>(*_1, *_2)) ]
	  | r_filter_primary [ _val = _1 ]
	  ;
	  
	r_filter_primary =
	  ( r_filter_functions ) [ _val = _1 ]
	  |
	  ( lit("maximal") >> '(' >> r_scalar_qty_expression >> ( ( ',' >> int_ ) | attr(0) ) >> ')' ) 
	    [ _val = phx::construct<FilterPtr>(new_<maximal>(*_1, _2)) ]
	  |
	  ( lit("minimal") >> '(' >> r_scalar_qty_expression >> ( ( ',' >> int_ ) | attr(0) ) >> ')' ) 
	    [ _val = phx::construct<FilterPtr>(new_<minimal>(*_1, _2)) ]
	  |
	  ( r_qty_comparison ) [ _val = _1 ]
	  |
	  ( '(' > r_filter > ')' ) [ _val = _1 ]
	  | 
	  ( '!' > r_filter_primary ) [ _val = phx::construct<FilterPtr>(new_<NOT>(*_1)) ]
	  ;
	  
	r_qty_comparison = 
	  ( r_scalar_qty_expression >> lit("==") >> r_scalar_qty_expression ) [ _val = phx::construct<FilterPtr>(new_<equal<double, double> >(*_1, *_2)) ]
	  |
	  ( r_scalar_qty_expression >> '~' >> r_scalar_qty_expression >> ( ( '{' >> double_ >> '}' ) | attr(1e-2) ) ) 
	    [ _val = phx::construct<FilterPtr>(new_<approximatelyEqual<double> >(*_1, *_2, _3)) ]
	  |
	  ( r_scalar_qty_expression >> '>' >> r_scalar_qty_expression ) [ _val = phx::construct<FilterPtr>(new_<greater<double, double> >(*_1, *_2)) ]
	  |
	  ( r_scalar_qty_expression >> lit(">=") >> r_scalar_qty_expression ) [ _val = phx::construct<FilterPtr>(new_<greaterequal<double, double> >(*_1, *_2)) ]
	  |
	  ( r_scalar_qty_expression >> '<' >> r_scalar_qty_expression ) [ _val = phx::construct<FilterPtr>(new_<less<double, double> >(*_1, *_2)) ]
	  |
	  ( r_scalar_qty_expression >> lit("<=") >> r_scalar_qty_expression ) [ _val = phx::construct<FilterPtr>(new_<lessequal<double, double> >(*_1, *_2)) ]
	  ;
	  
	r_scalar_qty_expression =
	  r_scalar_term [ _val = _1 ]
	  |
	  ( r_scalar_term >> '+' >> r_scalar_term ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<added<double,double> >(*_1, *_2)) ]
	  | 
	  ( r_scalar_term >> '-' >> r_scalar_term ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<subtracted<double,double> >(*_1, *_2)) ]
	  ;
	
	r_scalar_term =
	  r_scalar_primary [ _val = _1 ]
	  |
	  ( r_scalar_primary >> '*' >> r_scalar_primary ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<multiplied<double,double> >(*_1, *_2)) ]
	  | 
	  ( r_scalar_primary >> '/' >> r_scalar_primary ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<divided<double,double> >(*_1, *_2)) ]
	  |
	  ( r_mat_primary >> '&' >> r_mat_primary ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<dotted<arma::mat,arma::mat> >(*_1, *_2)) ]
	  ;
	  
	r_scalar_primary =
	  /*lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  |*/ double_ [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<constantQuantity<double> >(_1)) ]
	  | r_scalar_qty_functions [ _val = _1 ]
	  | ( lit("mag") >> '(' >> r_scalar_qty_expression >> ')' ) 
	   [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<mag<double> >(*_1)) ]
	  | ( lit("angle") >> '(' >> r_mat_qty_expression >> ',' >> r_mat_qty_expression >> ')' ) 
	   [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<angle<arma::mat,arma::mat> >(*_1, *_2)) ]
	  | ( lit("angleMag") >> '(' >> r_mat_qty_expression >> ',' >> r_mat_qty_expression >> ')' ) 
	   [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<angleMag<arma::mat,arma::mat> >(*_1, *_2)) ]
	  | ( '(' >> r_scalar_qty_expression >> ')' ) [ _val = _1 ]
// 	  | ('-' >> r_scalar_primary) [ _val = -_1 ]
	  | ( r_mat_primary >> '.' >> 'x' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compX<arma::mat> >(*_1)) ]
	  | ( r_mat_primary >> '.' >> 'y' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compY<arma::mat> >(*_1)) ]
	  | ( r_mat_primary >> '.' >> 'z' ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<compZ<arma::mat> >(*_1)) ]
	  ;
	  
	r_mat_qty_expression =
	  r_mat_term [ _val = _1 ]
	  |
	  ( r_mat_term >> '+' >> r_mat_term ) 
	    [ _val = phx::construct<matQuantityComputer::Ptr>(new_<added<arma::mat,arma::mat> >(*_1, *_2)) ]
	  | 
	  ( r_mat_term >> '-' >> r_mat_term ) 
	    [ _val = phx::construct<matQuantityComputer::Ptr>(new_<subtracted<arma::mat,arma::mat> >(*_1, *_2)) ]
	  ;
	
	r_mat_term =
	(
	  r_mat_primary [ _val = _1 ]
	  |
	  ( r_mat_primary >> '*' >> r_mat_primary ) 
	    [ _val = phx::construct<matQuantityComputer::Ptr>(new_<multiplied<arma::mat,arma::mat> >(*_1, *_2)) ]
	  | 
	  ( r_mat_primary >> '/' >> r_mat_primary ) 
	    [ _val = phx::construct<matQuantityComputer::Ptr>(new_<divided<arma::mat,arma::mat> >(*_1, *_2)) ]
	) /*| (
	  r_vector_primary >> '&' >> r_vector_primary
	) [_val = dot_(_1, _2) ]*/
	  ;
	  
	r_mat_primary =
	  /*lexeme[ model_->scalarSymbols >> !(alnum | '_') ] [ _val = _1 ]
	  |*/ 
	  ( '[' >> double_ >> ',' >> double_ >> ',' >> double_ >> ']' ) 
	    [ _val = phx::construct<matQuantityComputer::Ptr>(new_<constantQuantity<arma::mat> >(vec3_(_1,_2,_3))) ]
	  | 
	  r_mat_qty_functions [ _val = _1 ]
	  | 
	  ( '(' >> r_mat_qty_expression >> ')' ) [ _val = _1 ]
// 	  | ('-' >> r_scalar_primary) [ _val = -_1 ]
	  ;

//       BOOST_SPIRIT_DEBUG_NODE(r_filter);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_or);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_and);
//       BOOST_SPIRIT_DEBUG_NODE(r_filter_primary);
//       
//       BOOST_SPIRIT_DEBUG_NODE(r_qty_comparison);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_qty_expression);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_term);
//       BOOST_SPIRIT_DEBUG_NODE(r_scalar_primary);

      on_error<fail>(r_filter,
                       phx::ref(std::cout)
                       << "Error! Expecting "
                       << qi::_4
                       << " here: '"
                       << phx::construct<std::string>(qi::_3, qi::_2)
                       << "'\n"
                      );
    }  
    
    
};

template <typename Iterator/*, typename Skipper = skip_grammar<Iterator>*/ >
struct EdgeFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  EdgeFeatureFilterExprParser(const FeatureSetList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
    FeatureFilterExprParser<Iterator>::r_filter_functions =
	( lit("isLine") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Line)) ]
	|
	( lit("isCircle") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Circle)) ]
	|
	( lit("isEllipse") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Ellipse)) ]
	|
	( lit("isHyperbola") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Hyperbola)) ]
	|
	( lit("isParabola") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_Parabola)) ]
	|
	( lit("isBezierCurve") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_BezierCurve)) ]
	|
	( lit("isBSplineCurve") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_BSplineCurve)) ]
	|
	( lit("isOtherCurve") ) [ _val = phx::construct<FilterPtr>(new_<edgeTopology>(GeomAbs_OtherCurve)) ]
	|
	( lit("isCoincident") >> FeatureFilterExprParser<Iterator>::r_featureset ) 
	  [ _val = phx::construct<FilterPtr>(new_<coincidentEdge>(*_1)) ]
	  ;
  }
};


template <typename Iterator/*, typename Skipper = skip_grammar<Iterator>*/ >
struct FaceFeatureFilterExprParser
: public FeatureFilterExprParser<Iterator>
{

  FaceFeatureFilterExprParser(const FeatureSetList& extsets)
  : FeatureFilterExprParser<Iterator>(extsets)
  {
    FeatureFilterExprParser<Iterator>::r_filter_functions = 
	( lit("isPlane") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Plane)) ]
	|
	( lit("isCylinder") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cylinder)) ]
	|
	( lit("isCone") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Cone)) ]
	|
	( lit("isSphere") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Sphere)) ]
	|
	( lit("isTorus") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_Torus)) ]
	|
	( lit("isBezierSurface") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BezierSurface)) ]
	|
	( lit("isBSplineSurface") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_BSplineSurface)) ]
	|
	( lit("isSurfaceOfRevolution") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfRevolution)) ]
	|
	( lit("isSurfaceOfExtrusion") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_SurfaceOfExtrusion)) ]
	|
	( lit("isOffsetSurface") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OffsetSurface)) ]
	|
	( lit("isOtherSurface") ) [ _val = phx::construct<FilterPtr>(new_<faceTopology>(GeomAbs_OtherSurface)) ]
      ;

      FeatureFilterExprParser<Iterator>::r_scalar_qty_functions =
	( lit("cylRadius") ) [ _val = phx::construct<scalarQuantityComputer::Ptr>(new_<cylRadius>()) ]
      ;
      
      FeatureFilterExprParser<Iterator>::r_mat_qty_functions = 
        ( lit("normal") ) [ _val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::faceNormalVector>()) ]
        |
        ( lit("faceCoG") ) [ _val = phx::construct<matQuantityComputer::Ptr>(new_<insight::cad::faceCoG>()) ]
      ;
    
  }
};

template<class Parser>
FilterPtr parseFilterExpr(std::istream& in, const std::vector<FeatureSet>& refs)
{
try {
  Parser parser(refs);
//   skip_grammar<Iterator> skip;
  
  std::string contents_raw;
  in.seekg(0, std::ios::end);
  contents_raw.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents_raw[0], contents_raw.size());
  
  std::string::iterator first=contents_raw.begin();
  std::string::iterator last=contents_raw.end();
  
  FilterPtr result;
  bool r = qi::phrase_parse
  (
      first, 
      last,
      parser,
      qi::space,
      result
  );
  
//   ModelStepsWriter writer;
//   parser.model_.modelstepSymbols.for_each(writer);

  if (first != last) // fail if we did not get a full match
  {
    throw insight::Exception("parsing of filtering expression failed!");
    return FilterPtr();
  }
  
//   model = parser.model_;
  
  return result;
}
catch (insight::Exception e)
{
  std::cerr<<"Exception occurred: "<<e<<std::endl;
  throw e;
}

}

FilterPtr parseEdgeFilterExpr(std::istream& in, const std::vector<FeatureSet>& refs)
{
  return parseFilterExpr<EdgeFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

FilterPtr parseFaceFilterExpr(std::istream& in, const std::vector<FeatureSet>& refs)
{
  return parseFilterExpr<FaceFeatureFilterExprParser<std::string::iterator> >(in, refs);
}

}

}