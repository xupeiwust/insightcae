#ifndef IQVTKCONSTRAINEDSKETCHENTITY_H
#define IQVTKCONSTRAINEDSKETCHENTITY_H


#include "constrainedsketchgeometry.h"

#include "vtkProp.h"

#include <constrainedsketchgeometry.h>

class IQVTKConstrainedSketchEntity
    : public insight::cad::ConstrainedSketchEntity
{
public:
    virtual std::vector<vtkSmartPointer<vtkProp> > createActor() const =0;
};


#endif // IQVTKCONSTRAINEDSKETCHENTITY_H
