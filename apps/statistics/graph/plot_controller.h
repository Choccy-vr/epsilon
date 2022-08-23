#ifndef STATISTICS_PLOT_CONTROLLER_H
#define STATISTICS_PLOT_CONTROLLER_H

#include <apps/i18n.h>
#include <apps/shared/curve_view.h>
#include <apps/shared/curve_view_cursor.h>
#include <escher/button_row_controller.h>
#include <escher/responder.h>
#include <escher/stack_view_controller.h>
#include <escher/view_controller.h>
#include "../store.h"
#include "data_view_controller.h"
#include "plot_curve_view.h"
#include "plot_banner_view.h"
#include "plot_range.h"
#include "plot_view.h"
#include <algorithm>

namespace Statistics {

class PlotController : public DataViewController {
public:
  PlotController(
    Escher::Responder * parentResponder,
    Escher::ButtonRowController * header,
    Escher::Responder * tabController,
    Escher::StackViewController * stackViewController,
    Escher::ViewController * typeViewController,
    Store * store
  );
  bool seriesIsValid(int series) const { return validSerieMethod()(m_store, series); }
  virtual int totalValues(int series) const = 0;
  virtual double valueAtIndex(int series, int i) const = 0;
  virtual double resultAtIndex(int series, int i) const = 0;
  virtual bool connectPoints() const { return false; }
  virtual bool drawSeriesZScoreLine(int series, float * x, float * y, float * u, float * v) const { return false; }
  virtual void appendLabelSuffix(Shared::CurveView::Axis axis, char * labelBuffer, int maxSize, int glyphLength, int maxGlyphLength) const {}
  virtual float labelStepMultiplicator(Shared::CurveView::Axis axis) const { return 2.0f; }

  // DataViewController
  DataView * dataView() override { return &m_view; }
  bool moveSelectionHorizontally(int deltaIndex) override;

protected:
  constexpr static KDCoordinate k_smallMargin = 10;
  constexpr static KDCoordinate k_mediumMargin = 20;
  constexpr static KDCoordinate k_largeMargin = 30;
  static int SanitizeIndex(int index, int numberOfPairs) {
    return std::max(0, std::min(index, numberOfPairs - 1));
  }

  // DataViewController
  void viewWillAppearBeforeReload() override;
  bool reloadBannerView() override;

  Shared::CurveViewCursor m_cursor;
  PlotRange m_graphRange;
  PlotBannerView m_bannerView;
  Shared::CursorView m_cursorView;
  PlotView m_view;
  PlotCurveView m_curveView;

private:
  void computeXBounds(float * xMin, float *xMax) const;
  virtual void computeYBounds(float * yMin, float *yMax) const = 0;
  virtual bool handleNullFrequencies() const = 0;
  virtual KDCoordinate horizontalMargin() const = 0;
  virtual KDCoordinate bottomMargin() const = 0;
  virtual KDCoordinate topMargin() const = 0;
  virtual const char * resultMessageTemplate() const = 0;
  virtual I18n::Message resultMessage() const = 0;
};

}  // namespace Statistics

#endif /* STATISTICS_PLOT_CONTROLLER_H */
