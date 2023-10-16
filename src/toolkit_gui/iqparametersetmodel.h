#ifndef IQPARAMETERSETMODEL_H
#define IQPARAMETERSETMODEL_H

#include "toolkit_gui_export.h"


#include <QAbstractItemModel>
#include <QScopedPointer>

#include "base/parameterset.h"
#include "iqparameter.h"

class IQArrayParameter;
class IQSelectableSubsetParameter;
class IQCADModel3DViewer;
template<class IQBaseParameter, const char* N> class IQArrayElementParameter;

namespace insight {
class CADParameterSetVisualizer;
}

class TOOLKIT_GUI_EXPORT IQParameterSetModel
    : public QAbstractItemModel
{
  Q_OBJECT

  friend class IQArrayParameter;
  friend class IQSelectableSubsetParameter;

  template<class IQBaseParameter, const char* N>
  friend class IQArrayElementParameter;

  insight::ParameterSet parameterSet_, defaultParameterSet_;
  QList<IQParameter*> rootParameters_;
  std::string analysisName_;

  mutable std::map<QString, insight::cad::FeaturePtr> transformedGeometry_;

  /**
   * @brief vectorBasePoints_
   * if a vector parameter represents a direction, this map contains the base point.
   * If there is no base point for a vector parameter, it treated as a point (location vector)
   */
  mutable std::map<QString, arma::mat> vectorBasePoints_;

  std::pair<QString, const insight::Parameter*> getParameterAndName(const QModelIndex& index) const;

  QList<IQParameter*> decorateSubdictContent(QObject*, const insight::ParameterSet&);
  IQParameter* decorateArrayElement(QObject* parent, int i, insight::Parameter& cp);
  QList<IQParameter*> decorateArrayContent(QObject*, insight::ArrayParameterBase& ap);
  void decorateChildren(QObject* parent, insight::Parameter* p);

public:
  IQParameterSetModel(const insight::ParameterSet& ps, const insight::ParameterSet& defaultps, QObject* parent=nullptr);

  int	columnCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant	headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

  int	rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex	index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
  QModelIndex	parent(const QModelIndex &index) const override;

  Qt::ItemFlags flags(const QModelIndex &index) const override;
  Qt::DropActions supportedDropActions() const override;
  QStringList mimeTypes() const override;
  QMimeData * mimeData(const QModelIndexList & indexes) const override;
  bool dropMimeData(const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex & parent) override;

  QVariant	data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

  void copy(const QModelIndexList & indexes) const;
  void paste(const QModelIndexList & indexes);
  void contextMenu(QWidget* pw, const QModelIndex& index, const QPoint& p);

  class ParameterEditor
  {
      IQParameterSetModel& model_;
      const QModelIndex index_;
  public:
      insight::Parameter& parameter;
      ParameterEditor(IQParameterSetModel& psm, const std::string& parameterPath);
      ~ParameterEditor();
  };
  friend class ParameterContext;

  // access functions
  QList<int> pathFromIndex(const QModelIndex& i) const;
  QModelIndex indexFromParameterPath(const std::string& pp) const;
  QModelIndex indexFromPath(const QList<int>& p) const;

  const insight::ParameterSet& getParameterSet() const;

  // edit functions
  bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;

  insight::Parameter& parameterRef(const QModelIndex &index);

  /**
   * @brief notifyParameterChange
   * update parameter and redecorate all children, if necessary
   * @param index
   */
  void notifyParameterChange(const QModelIndex &index, bool redecorateChildren=false);

  void appendArrayElement(const QModelIndex &index, const insight::Parameter& elem);
  /**
   * @brief insertArrayElement
   * @param index
   * index of array element before which shall be inserted or parent array (appended in this case)
   * @param elem
   */
  void insertArrayElement(const QModelIndex &index, const insight::Parameter& elem);
  void removeArrayElement(const QModelIndex &index);


  void addGeometryToSpatialTransformationParameter(
          const QString& parameterPath, insight::cad::FeaturePtr geom );
  void addVectorBasePoint(
          const QString& parameterPath, const arma::mat& pBase );

  insight::cad::FeaturePtr
  getGeometryToSpatialTransformationParameter(
          const QString& parameterPath );
  const arma::mat* const getVectorBasePoint(
          const QString& parameterPath );

  void setAnalysisName(const std::string& analysisName);
  const std::string& getAnalysisName() const;

public Q_SLOTS:
  void clearParameters();
  void resetParameters(const insight::ParameterSet& ps, const insight::ParameterSet& defaultps);
  void handleClick(const QModelIndex &index, QWidget* editControlsContainer,
                   IQCADModel3DViewer *vri=nullptr,
                   insight::CADParameterSetVisualizer *viz = nullptr);

 Q_SIGNALS:
  void parameterSetChanged();
};

#endif // IQPARAMETERSETMODEL_H
