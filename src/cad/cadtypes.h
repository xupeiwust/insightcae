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

#ifndef INSIGHT_CAD_CADTYPES_H
#define INSIGHT_CAD_CADTYPES_H

#ifndef Q_MOC_RUN
#include "boost/variant.hpp"
#include "base/linearalgebra.h"
#endif

namespace insight {
namespace cad {

class SolidModel;
class Datum;
class FeatureSet;
class Filter;
class Evaluation;


typedef boost::shared_ptr<SolidModel> SolidModelPtr;
typedef boost::shared_ptr<Datum> DatumPtr;
typedef boost::shared_ptr<FeatureSet> FeatureSetPtr;
typedef boost::shared_ptr<Evaluation> EvaluationPtr;

typedef int FeatureID;
typedef boost::shared_ptr<FeatureSet> FeatureSetPtr;
typedef boost::variant<FeatureSetPtr,arma::mat,double> FeatureSetParserArg;
typedef std::vector<FeatureSetParserArg> FeatureSetParserArgList;
typedef std::vector<FeatureSetPtr> FeatureSetList;

typedef boost::shared_ptr<Filter> FilterPtr;

}
}

#endif
