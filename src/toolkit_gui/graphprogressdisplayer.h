/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */


#ifndef GRAPHPROGRESSDISPLAYER_H
#define GRAPHPROGRESSDISPLAYER_H

#ifndef Q_MOC_RUN
#include <base/analysis.h>
#endif

#include <map>
#include <vector>

#include <QWidget>
#include <QLabel>
#include <QMutex>
#include <QTabWidget>

#include <qwt.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>

class GraphProgressChart
    : public QwtPlot
{
  Q_OBJECT

protected:
  size_t maxCnt_;
  typedef std::map<std::string, std::vector<double> > ArrayList;
  ArrayList progressX_;
  ArrayList progressY_;
  std::map<std::string, QwtPlotCurve*> curve_;
  bool needsRedraw_;
  QMutex mutex_;
  bool logscale_;

public:
  GraphProgressChart(bool logscale, QWidget* parent=nullptr);
  ~GraphProgressChart();

  virtual void update(double t, const std::string& name, double y_value);
  virtual void reset();

public slots:
  void checkForUpdate();
};




class GraphProgressDisplayer 
: public QTabWidget,
  public insight::ProgressDisplayer
{
  Q_OBJECT
  
protected:
  std::map<std::string, GraphProgressChart*> charts_;

public:
    GraphProgressDisplayer(QWidget* parent=nullptr);
    virtual ~GraphProgressDisplayer();

    GraphProgressChart* addChartIfNeeded(const std::string& name);

    virtual void reset();

Q_SIGNALS:
    void createChart(bool log, const std::string name);

public Q_SLOTS:
    virtual void update(const insight::ProgressState& pi);

private Q_SLOTS:
    void onCreateChart(bool log, const std::string name);
};

#endif // GRAPHPROGRESSDISPLAYER_H
