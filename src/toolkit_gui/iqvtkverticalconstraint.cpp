#include "iqvtkverticalconstraint.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

IQVTKVerticalConstraint::IQVTKVerticalConstraint(
    std::shared_ptr<insight::cad::Line const> line )
    : line_(line)
{}

std::vector<vtkSmartPointer<vtkProp> >
IQVTKVerticalConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("V");
    caption->SetAttachmentPoint(
        arma::mat(0.5*
                  (line_->getDatumPoint("p0")+line_->getDatumPoint("p1")))
            .memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};

}
int IQVTKVerticalConstraint::nConstraints() const
{
    return 1;
}

double IQVTKVerticalConstraint::getConstraintError(unsigned int iConstraint) const
{
    arma::mat ex = insight::normalized(
        line_->getDatumVectors().at("ex") );
    return 1.-fabs(arma::dot(insight::vec3(0,1,0), ex));
}

void IQVTKVerticalConstraint::scaleSketch(double scaleFactor)
{}

void IQVTKVerticalConstraint::generateScriptCommand(insight::cad::ConstrainedSketchScriptBuffer &script, const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{

}
