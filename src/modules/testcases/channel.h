/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef INSIGHT_CHANNEL_H
#define INSIGHT_CHANNEL_H

#include "base/linearalgebra.h"
#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamcaseelements.h"

namespace insight {

class ChannelBase 
: public OpenFOAMAnalysis
{
public:
  /**
   * convert friction velocity Reynolds number into bulk velocity Re
   */
  static double Re(double Retau);

  /**
   * convert friction velocity Reynolds number into bulk velocity Re
   */
  static double Retau(double Re);

  /**
   * compute factor Umax/Ubulk
   */
  static double UmaxByUbulk(double Retau);
  
  
  /**
   * number of profiles for homogeneous averages
   */
  const int n_hom_avg=10;

protected:
  std::string cycl_in_, cycl_out_;
  
  // Derived input data
  /**
   * Bulk velocity reynolds number
   */
  double Re_;
  
  /**
   * friction velocity
   */
  double utau_;
  
  /**
   * bulk velocity
   */
  double Ubulk_;
  
  /** 
   * flow-through time
   */
  double T_;
  
  /**
   * viscosity
   */
  double nu_;
  
  /**
   * height of cell layer nearest to wall
   */
  double ywall_;
  
  double gradl_;
  
  /**
   * number of cells along flow direction
   */
  int nax_;
  
  /**
   * number of cells along span
   */
  int nb_;
  
  /**
   * number of cells along half height
   */
  int nh_;
  
  /**
   * number of cells along half height
   */
  int nhbuf_;
  
  /**
   * grading towards wall
   */
  double gradh_;
  
  /**
   * height of buffer layer
   */
  double hbuf_;
  
public:
  declareType("Channel Flow Test Case");
  
  ChannelBase(const NoParameters& p = NoParameters());
  ~ChannelBase();
  
  virtual ParameterSet defaultParameters() const;
  
  std::string cyclPrefix() const;
  virtual void calcDerivedInputData();

  virtual void createMesh
  (
    OpenFOAMCase& cm
  );
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
  
  virtual void evaluateAtSection(
    OpenFOAMCase& cm,
    ResultSetPtr results, double x, int i
  );
    
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cmp);
  
};




class ChannelCyclic
: public ChannelBase
{
public:
  declareType("Channel Flow Test Case (Axial Cyclic)");
  
  ChannelCyclic(const NoParameters& p = NoParameters() );
  
  virtual ParameterSet defaultParameters() const;

  virtual void createMesh
  (
    OpenFOAMCase& cm
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
  
};

class ChannelInflow
: public ChannelBase
{
  
#ifndef SWIG
  /**
   * locations (x/H) of section evaluations 
   */
  std::vector<double> sec_locs_ = (boost::assign::list_of<double>(0)(0.1)(0.25)(0.5)(0.75)(1)(1.5)(2)(3)(4)(6)(8));

  const static int ntpc_ = 4;
  const static char* tpc_names_[ntpc_]; 
  const static double tpc_xlocs_[ntpc_];
#endif
  
public:
  declareType("Channel Flow Test Case (Inflow Generator)");
  
  ChannelInflow(const NoParameters& p = NoParameters() );
  
  virtual ParameterSet defaultParameters() const;

  virtual void createMesh
  (
    OpenFOAMCase& cm
  );  
  
  virtual void createCase
  (
    OpenFOAMCase& cm
  );

  ResultSetPtr evaluateResults(OpenFOAMCase& cm);

  virtual void applyCustomOptions(OpenFOAMCase& cm, boost::shared_ptr<OFdicts>& dicts);
  virtual void applyCustomPreprocessing(OpenFOAMCase& cm);
  
};

}

#endif // INSIGHT_CHANNEL_H
