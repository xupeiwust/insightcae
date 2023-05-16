#ifndef INSIGHT_CADSKETCHPARAMETER_H
#define INSIGHT_CADSKETCHPARAMETER_H

#include "cadtypes.h"
#include "base/parameter.h"
#include "base/parameterset.h"

#include "constrainedsketchgeometry.h"

namespace insight {

namespace cad {
class ConstrainedSketch;
}

class CADSketchParameter
: public Parameter
{


protected:
    //    insight::cad::ModelPtr cadmodel_;
    std::string script_;

    cad::MakeDefaultGeometryParametersFunction makeDefaultGeometryParameters;

    mutable std::shared_ptr<insight::cad::ConstrainedSketch> CADGeometry_;

    void resetCADGeometry() const;

public:
    declareType ( "cadsketch" );

    CADSketchParameter (
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0  );

    CADSketchParameter (
        const std::string& script,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );

    CADSketchParameter (
        const std::string& script,
        cad::MakeDefaultGeometryParametersFunction defaultGeometryParameters,
        const std::string& description,
        bool isHidden=false,
        bool isExpert=false,
        bool isNecessary=false,
        int order=0 );


    insight::ParameterSet defaultGeometryParameters() const;

    const std::string& script() const;
    void setScript(const std::string& script);

    //    void setCADModel(insight::cad::ModelPtr cadmodel);
    std::shared_ptr<insight::cad::ConstrainedSketch> featureGeometry() const;

    std::string latexRepresentation() const override;
    std::string plainTextRepresentation(int indent=0) const override;

    rapidxml::xml_node<>* appendToNode
        (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath
            ) const override;

    void readFromNode
        (
            const std::string& name,
            rapidxml::xml_document<>& doc,
            rapidxml::xml_node<>& node,
            boost::filesystem::path inputfilepath
            ) override;

    CADSketchParameter* cloneCADSketchParameter() const;
    Parameter* clone() const override;

    void operator=(const CADSketchParameter& op);

    bool isDifferent(const Parameter &) const override;
};

} // namespace insight

#endif // INSIGHT_CADSKETCHPARAMETER_H
