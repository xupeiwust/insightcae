#include "iqvtkverticalconstraint.h"

#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

defineType(IQVTKVerticalConstraint);

IQVTKVerticalConstraint::IQVTKVerticalConstraint(
    std::shared_ptr<insight::cad::Line> line )
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

void IQVTKVerticalConstraint::generateScriptCommand(
    insight::cad::ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels ) const
{
    int myLabel=entityLabels.at(this);

    line_->generateScriptCommand(script, entityLabels);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + boost::lexical_cast<std::string>(entityLabels.at(line_.get()))
            + parameterString()
            + ")"
        );
}

namespace insight { namespace cad {
addToStaticFunctionTable(ConstrainedSketchEntity, IQVTKVerticalConstraint, addParserRule);
}}

void IQVTKVerticalConstraint::addParserRule(
    insight::cad::ConstrainedSketchGrammar &ruleset,
    insight::cad::MakeDefaultGeometryParametersFunction )
{
    using namespace insight::cad;
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::int_
             > ruleset.r_parameters >
             ')' )
                [ qi::_val = phx::bind(
                     &IQVTKVerticalConstraint::create<std::shared_ptr<insight::cad::Line> >,
                     phx::bind(&insight::cad::ConstrainedSketchGrammar::lookupEntity<insight::cad::Line>, phx::ref(ruleset), qi::_2) ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_3, "."),
                 phx::insert(
                     phx::ref(ruleset.labeledEntities),
                     phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val)) ]
            );
}

std::set<std::comparable_weak_ptr<insight::cad::ConstrainedSketchEntity> > IQVTKVerticalConstraint::dependencies() const
{
    return { line_ };
}

void IQVTKVerticalConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity)
{
    if (auto l = std::dynamic_pointer_cast<insight::cad::Line>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line_) == entity)
        {
            line_ = l;
        }
    }
}
