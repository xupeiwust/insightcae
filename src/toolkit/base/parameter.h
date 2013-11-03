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


#ifndef INSIGHT_PARAMETER_H
#define INSIGHT_PARAMETER_H

#include <string>
#include <vector>
#include <string>
#include <typeinfo>
#include "boost/filesystem.hpp"
#include "boost/variant.hpp"
#include <boost/noncopyable.hpp>
#include <boost/concept_check.hpp>

namespace insight {
  
class Parameter
: boost::noncopyable
{
protected:
  std::string description_;
  
public:
  Parameter(const std::string& description);
  virtual ~Parameter();
  
  inline const std::string& description() const
  { return description_; }
  
  virtual Parameter* clone() const =0;
};


template<class T>
class SimpleParameter
: public Parameter
{
  
protected:
  T value_;
  
public:
  SimpleParameter(T defaultValue, const std::string& description)
  : Parameter(description),
    value_(defaultValue)
  {}
  
  virtual ~SimpleParameter()
  {}
  
  inline T& operator()() { return value_; }
  inline const T& operator()() const { return value_; }
  
  virtual Parameter* clone() const
  {
    return new SimpleParameter<T>(value_, description_);
  }
};


typedef SimpleParameter<double> DoubleParameter;
typedef SimpleParameter<int> IntParameter;
typedef SimpleParameter<bool> BoolParameter;
typedef SimpleParameter<std::string> StringParameter;
typedef SimpleParameter<boost::filesystem::path> PathParameter;

class DirectoryParameter
: public PathParameter
{
public:
  DirectoryParameter(boost::filesystem::path defaultValue, const std::string& description);
  virtual Parameter* clone() const;
};


class SelectionParameter
: public IntParameter
{
public:
  typedef std::vector<std::string> ItemList;
  
protected:
  ItemList items_;
  
public:
  SelectionParameter(int defaultValue, const ItemList& items, const std::string& description);
  virtual ~SelectionParameter();
  
  virtual const ItemList& items() const;
  
  virtual Parameter* clone() const;
};

}

namespace boost
{
  
inline insight::Parameter* new_clone(const insight::Parameter& p)
{
  return p.clone();
}

}

#endif // INSIGHT_PARAMETER_H
