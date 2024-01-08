#ifndef STATISTICS_PLOT_CURVE_VIEW_H
#define STATISTICS_PLOT_CURVE_VIEW_H

#include <apps/shared/plot_view_policies.h>

#include "../store.h"

namespace Statistics {

class PlotController;

class LabeledAxisWithOptionalPercent
    : public Shared::PlotPolicy::VerticalLabeledAxis {
 public:
  void setPlotController(PlotController* controller) {
    m_plotController = controller;
  }

 protected:
  float tickStep(const Shared::AbstractPlotView* plotView,
                 Shared::AbstractPlotView::Axis axis) const;
  int computeLabel(int i, const Shared::AbstractPlotView* plotView,
                   Shared::AbstractPlotView::Axis axis);

  PlotController* m_plotController;
};

typedef Shared::PlotPolicy::Axes<Shared::PlotPolicy::WithCartesianGrid,
                                 Shared::PlotPolicy::HorizontalLabeledAxis,
                                 LabeledAxisWithOptionalPercent>
    PlotViewAxes;

class PlotViewPolicy {
 protected:
  void drawPlot(const Shared::AbstractPlotView* plotView, KDContext* ctx,
                KDRect rect) const;
  void drawSeriesCurve(const Shared::AbstractPlotView* plotView, KDContext* ctx,
                       KDRect rect, int series) const;

  PlotController* m_plotController;
};

class PlotCurveView : public Shared::PlotView<PlotViewAxes, PlotViewPolicy,
                                              Shared::PlotPolicy::NoBanner,
                                              Shared::PlotPolicy::WithCursor> {
 public:
  PlotCurveView(Shared::CurveViewRange* range, Shared::CurveViewCursor* cursor,
                Shared::CursorView* cursorView, PlotController* plotController);
};

}  // namespace Statistics

#endif
