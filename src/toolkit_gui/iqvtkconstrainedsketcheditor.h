#ifndef IQVTKCONSTRAINEDSKETCHEDITOR_H
#define IQVTKCONSTRAINEDSKETCHEDITOR_H

#include "toolkit_gui_export.h"


#include "vtkVersionMacros.h"
#include "vtkSmartPointer.h"
#include "vtkActor.h"

#include <QObject>
#include <QToolBar>
#include <QToolBox>
#include <QDockWidget>

#include "sketch.h"
#include "selectionlogic.h"
//#include "iqvtkviewerstate.h"
#include "viewwidgetaction.h"
#include "iqvtkcadmodel3dviewer.h"

class ParameterEditorWidget;


class IQVTKConstrainedSketchEntity
        : public insight::cad::ConstrainedSketchEntity
{
public:
    virtual std::vector<vtkSmartPointer<vtkProp> > createActor() const =0;
};




class IQVTKFixedPoint
        : public IQVTKConstrainedSketchEntity
{

    insight::cad::SketchPointPtr p_;

public:
    IQVTKFixedPoint(
            insight::cad::SketchPointPtr p  );

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;

    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};


class IQVTKHorizontalConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::Line const> line_;

public:
    IQVTKHorizontalConstraint(std::shared_ptr<insight::cad::Line const> line);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};


class IQVTKVerticalConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::Line const> line_;
public:
    IQVTKVerticalConstraint(std::shared_ptr<insight::cad::Line const> line);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};


class IQVTKPointOnCurveConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::SketchPoint const> p_;
    std::shared_ptr<insight::cad::Feature const> curve_;
public:
    IQVTKPointOnCurveConstraint(
        std::shared_ptr<insight::cad::SketchPoint const> p_,
        std::shared_ptr<insight::cad::Feature const> curve_ );

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};


//struct SketchEntitySelectionViewPropsToRestore
//{
//    double oldColor[3];
//};

class IQVTKConstrainedSketchEditor;


//class SketchEntitySelection
//    : public QObject,
//      public std::map<
//          std::weak_ptr<insight::cad::ConstrainedSketchEntity>,
//          std::map<vtkProp*, SketchEntitySelectionViewPropsToRestore>,
//          std::owner_less<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > >
//{
//    Q_OBJECT

//    insight::ParameterSet commonParameters_, defaultCommonParameters_;
//    IQVTKConstrainedSketchEditor& editor_;
//    ParameterEditorWidget* pe_;
//    int tbi_;

//    void highlight(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
//    void unhighlight(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);

//public:
//    SketchEntitySelection(IQVTKConstrainedSketchEditor& editor);
//    ~SketchEntitySelection();

//    void addToSelection(insight::cad::ConstrainedSketchEntityPtr entity);
//    bool isInSelection(const insight::cad::ConstrainedSketchEntityPtr& entity);
//};



struct SelectedSketchEntityWrapper
{
    std::weak_ptr<insight::cad::ConstrainedSketchEntity> sketchEntity_;
    std::weak_ptr<IQVTKViewerState> highlight_;
};



class SketchEntityMultiSelection
    : public QObject,
      public std::map<
          std::weak_ptr<insight::cad::ConstrainedSketchEntity>,
          std::set<std::weak_ptr<IQVTKViewerState>,
                   std::owner_less<std::weak_ptr<IQVTKViewerState> > >,
          std::owner_less<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > >
{
    Q_OBJECT

    insight::ParameterSet commonParameters_, defaultCommonParameters_;
    IQVTKConstrainedSketchEditor& editor_;
    ParameterEditorWidget* pe_;
    int tbi_;

//    void highlight(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
//    void unhighlight(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);

public:
    SketchEntityMultiSelection(IQVTKConstrainedSketchEditor& editor);
    ~SketchEntityMultiSelection();

    void addToSelection(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
    bool isInSelection(const insight::cad::ConstrainedSketchEntity* entity);
};


typedef
    SelectionLogic<
        ViewWidgetAction<IQVTKCADModel3DViewer>,
        IQVTKCADModel3DViewer,
        insight::cad::ConstrainedSketchEntity,
        SelectedSketchEntityWrapper,
        SketchEntityMultiSelection>
    IQVTKConstrainedSketchEditorSelectionLogic;




class TOOLKIT_GUI_EXPORT IQVTKConstrainedSketchEditor
      : public QWidget, //QObject,
        public IQVTKConstrainedSketchEditorSelectionLogic, // ViewWidgetAction<IQVTKCADModel3DViewer> //IQVTKViewerState,
        public insight::cad::ConstrainedSketchPtr
{
    Q_OBJECT

public:
    typedef
        std::set<vtkSmartPointer<vtkProp> >
        ActorSet;


private:
    typedef
        std::map<
            insight::cad::ConstrainedSketchEntityPtr,
            ActorSet >
        SketchGeometryActorMap;

    IQCADModel3DViewer::SetSketchEntityAppearanceCallback setActorAppearance_;

    SketchGeometryActorMap sketchGeometryActors_;

    void add(insight::cad::ConstrainedSketchEntityPtr);
    void remove(insight::cad::ConstrainedSketchEntityPtr);

    insight::ParameterSet defaultGeometryParameters_;

    QToolBar *toolBar_;
    QDockWidget *toolBoxWidget_;
    QToolBox *toolBox_;

    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr currentAction_;

    friend class SketchEntityMultiSelection;

    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
    findEntitiesUnderCursor(const QPoint& point) const override;

    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
        std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity
        ) const override;
    void unhighlightEntity(
        IQVTKCADModel3DViewer::HighlightingHandleSet highlighters
        ) const override;

//    std::shared_ptr<SketchEntitySelection> currentSelection_;

private Q_SLOTS:
    void drawLine();
    void solve();

public:
    IQVTKConstrainedSketchEditor(
            IQVTKCADModel3DViewer& viewer,
            insight::cad::ConstrainedSketchPtr sketch,
            const insight::ParameterSet& defaultGeometryParameters,
            IQCADModel3DViewer::SetSketchEntityAppearanceCallback setActorAppearance
            );
    ~IQVTKConstrainedSketchEditor();

    insight::cad::ConstrainedSketchEntityPtr
        findSketchElementOfActor(vtkProp *actor) const;


    bool onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

    bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override;
    bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key ) override;

    void onMouseMove
      (
       Qt::MouseButtons buttons,
       const QPoint point,
       Qt::KeyboardModifiers curFlags
       ) override;

    void onMouseWheel
      (
        double angleDeltaX,
        double angleDeltaY
       ) override;


public Q_SLOTS:
    void updateActors();

Q_SIGNALS:
    void finished();
};


#endif // IQVTKCONSTRAINEDSKETCHEDITOR_H
