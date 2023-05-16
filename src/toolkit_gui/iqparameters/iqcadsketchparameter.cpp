#include "iqcadsketchparameter.h"

#include "iqparametersetmodel.h"
#include "iqcadmodel3dviewer.h"
#include "iqcaditemmodel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QInputDialog>

#include "base/tools.h"
#include "base/parameters/selectablesubsetparameter.h"

defineType(IQCADSketchParameter);
addToFactoryTable(IQParameter, IQCADSketchParameter);




IQCADSketchParameter::IQCADSketchParameter
    (
        QObject* parent,
        const QString& name,
        insight::Parameter& parameter,
        const insight::ParameterSet& defaultParameterSet
        )
    : IQParameter(parent, name, parameter, defaultParameterSet)
{
}




QString IQCADSketchParameter::valueText() const
{
    return "sketch";
}




QVBoxLayout* IQCADSketchParameter::populateEditControls(
    IQParameterSetModel* model,
    const QModelIndex &index,
    QWidget* editControlsContainer,
    IQCADModel3DViewer *viewer)
{
    const auto&p = dynamic_cast<const insight::CADSketchParameter&>(parameter());

    auto* layout = IQParameter::populateEditControls(model, index, editControlsContainer, viewer);


    auto *teScript = new QTextEdit(editControlsContainer);
    teScript->document()->setPlainText(QString::fromStdString(p.script()));
    layout->addWidget(teScript);


    QPushButton* apply=new QPushButton("&Apply", editControlsContainer);
    layout->addWidget(apply);

    QPushButton* edit=new QPushButton("&Edit sketch", editControlsContainer);
    layout->addWidget(edit);

    auto applyFunction = [=]()
    {
        auto&p = dynamic_cast<insight::CADSketchParameter&>(model->parameterRef(index));
        p.setScript( teScript->document()->toPlainText().toStdString() );
        model->notifyParameterChange(index);
    };
    connect(apply, &QPushButton::pressed, applyFunction);

    auto editFunction = [=]()
    {
        auto&p = dynamic_cast<insight::CADSketchParameter&>(model->parameterRef(index));

        auto plane = viewer->cadmodel()->datums().find("XY")->second;

        std::shared_ptr<insight::cad::ConstrainedSketch> sk;

        if (!(sk = p.featureGeometry()))
        {
            if (p.script().empty())
            {
                sk =  insight::cad::ConstrainedSketch::create(plane);
            }
            else
            {
                std::istringstream is(p.script());
                sk = insight::cad::ConstrainedSketch::createFromStream(
                    plane, is, p.defaultGeometryParameters() );
            }
        }

        IQParameterSetModel::ParameterEditor pc(*model, path().toStdString());
        auto& tp = dynamic_cast<insight::CADSketchParameter&>(pc.parameter);

        viewer->editSketch(
            sk,
            p.defaultGeometryParameters(),
#warning replace!!
            [](
                const insight::ParameterSet& seps,
                vtkProperty* actprops)
            {
                if (seps.size()>0)
                {
                    auto &selp = seps.get<insight::SelectableSubsetParameter>("type");
                    if (selp.selection()=="wall")
                    {
                        actprops->SetColor(0,0,0);
                        actprops->SetLineWidth(3);
                    }
                    else if (selp.selection()=="window")
                    {
                        actprops->SetColor(1,1,0);
                        actprops->SetLineWidth(4);
                    }
                    else if (selp.selection()=="door")
                    {
                        actprops->SetColor(0,1,0);
                        actprops->SetLineWidth(4);
                    }
                }
                else
                {
                    actprops->SetColor(1,0,0);
                    actprops->SetLineWidth(2);
                }
            },
            [&tp,sk]() {
                std::ostringstream os;
                sk->generateScript(os);
                tp.setScript(os.str());
            }
            );
    };
    connect(edit, &QPushButton::pressed, editFunction);

    return layout;
}
