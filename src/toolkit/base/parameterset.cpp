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


#include "parameterset.h"
#include "boost/foreach.hpp"

namespace insight
{

ParameterSet::ParameterSet()
{
}

ParameterSet::ParameterSet(const EntryList& entries)
{
  BOOST_FOREACH( const ParameterSet::SingleEntry& i, entries )
  {
    std::string key(boost::get<0>(i));
    insert(key, boost::get<1>(i));
  }
}

ParameterSet::~ParameterSet()
{
}

ParameterSet& ParameterSet::getSubset(const std::string& name) 
{ 
  return this->get<SubsetParameter>(name)();
}

const ParameterSet& ParameterSet::getSubset(const std::string& name) const
{
  return this->get<SubsetParameter>(name)();
}

ParameterSet* ParameterSet::clone() const
{
  ParameterSet *np=new ParameterSet;
  for (ParameterSet::const_iterator i=begin(); i!=end(); i++)
  {
    std::string key(i->first);
    np->insert(key, i->second->clone());
  }
  return np;
}


SubsetParameter::SubsetParameter(const ParameterSet& defaultValue, const std::string& description)
: Parameter(description),
  value_(defaultValue.clone())
{
}

Parameter* SubsetParameter::clone() const
{
  return new SubsetParameter(*value_, description_);
}

}
