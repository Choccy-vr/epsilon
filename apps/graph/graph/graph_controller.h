#ifndef GRAPH_GRAPH_CONTROLLER_H
#define GRAPH_GRAPH_CONTROLLER_H

#include <apps/shared/continuous_function_store.h>
#include <apps/shared/curve_view_cursor.h>
#include <apps/shared/function_graph_controller.h>
#include <apps/shared/interactive_curve_view_range.h>
#include <apps/shared/round_cursor_view.h>
#include <apps/shared/with_record.h>

#include "banner_view.h"
#include "curve_parameter_controller.h"
#include "graph_controller_helper.h"
#include "graph_view.h"
#include "points_of_interest_cache.h"

namespace Graph {

class GraphController : public Shared::FunctionGraphController,
                        public GraphControllerHelper {
 public:
  GraphController(
      Escher::Responder *parentResponder, Escher::ButtonRowController *header,
      Shared::InteractiveCurveViewRange *interactiveRange,
      Shared::CurveViewCursor *cursor, int *selectedCurveIndex,
      FunctionParameterController *functionParameterController,
      DerivativeColumnParameterController *derivativeColumnParameterController);

  // Responder
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;

  // ViewController
  void viewWillAppear() override;

  // AlternateEmptyViewDelegate
  I18n::Message emptyMessage() override;

  // InteractiveCurveViewRangeDelegate
  Poincare::Range2D<float> optimalRange(
      bool computeX, bool computeY,
      Poincare::Range2D<float> originalRange) const override;

  // TextFieldDelegate
  bool textFieldIsEditable(Escher::AbstractTextField *) override {
    return !functionStore()
                ->modelForRecord(recordAtSelectedCurveIndex())
                ->properties()
                .isScatterPlot();
  }

  PointsOfInterestCache *pointsOfInterestForRecord(Ion::Storage::Record record);
  PointsOfInterestCache *pointsOfInterestForSelectedRecord() {
    return pointsOfInterestForRecord(recordAtSelectedCurveIndex());
  }

 private:
  class FunctionSelectionController
      : public Shared::FunctionGraphController::FunctionSelectionController {
   public:
    FunctionSelectionController(GraphController *graphController)
        : Shared::FunctionGraphController::FunctionSelectionController(
              graphController) {}
    Shared::CurveSelectionCellWithChevron *reusableCell(int index,
                                                        int type) override {
      assert(index >= 0 && index < k_maxNumberOfDisplayableFunctions);
      return m_cells + index;
    }
    int reusableCellCount(int type) const override {
      return k_maxNumberOfDisplayableFunctions;
    }

   private:
    constexpr static int k_maxNumberOfDisplayableFunctions = 7;
    Poincare::Layout nameLayoutAtIndex(int j) const override;
    Shared::CurveSelectionCellWithChevron
        m_cells[k_maxNumberOfDisplayableFunctions];
  };

  // ZoomCurveViewController
  Shared::InteractiveCurveViewRange *interactiveCurveViewRange() override {
    return m_graphRange;
  }

  // GraphControllerHelper
  BannerView *bannerView() override { return &m_bannerView; }
  GraphView *graphView() override { return &m_view; }
  Shared::FunctionBannerDelegate *functionBannerDelegate() override {
    return this;
  }
  void jumpToLeftRightCurve(double t, OMG::HorizontalDirection direction,
                            int functionsCount,
                            Ion::Storage::Record record) override;

  // SimpleInteractiveCurveViewController
  void reloadBannerView() override;
  bool moveCursorHorizontally(OMG::HorizontalDirection direction,
                              int scrollSpeed = 1) override;

  // FunctionGraphController
  void selectCurveAtIndex(int curveIndex, bool willBeVisible) override;
  int nextCurveIndexVertically(OMG::VerticalDirection direction,
                               int currentCurveIndex,
                               Poincare::Context *context,
                               int currentSubCurveIndex,
                               int *nextSubCurveIndex) const override;
  double defaultCursorT(Ion::Storage::Record record,
                        bool ignoreMargins) override;
  Shared::ContinuousFunctionStore *functionStore() const override {
    return static_cast<Shared::ContinuousFunctionStore *>(
        Shared::FunctionGraphController::functionStore());
  }
  GraphView *functionGraphView() override { return &m_view; }
  void openMenuForSelectedCurve() override;
  bool moveCursorVertically(OMG::VerticalDirection direction) override;

  // InteractiveCurveViewController
  FunctionSelectionController *curveSelectionController() const override {
    return const_cast<FunctionSelectionController *>(
        &m_functionSelectionController);
  }

  // FunctionBannerDelegate
  void reloadBannerViewForCursorOnFunction(
      Shared::CurveViewCursor *cursor, Ion::Storage::Record record,
      Shared::FunctionStore *functionStore, Poincare::Context *context,
      bool cappedNumberOfSignificantDigits = false) override;

  bool defaultRangeIsNormalized() const {
    return functionStore()->displaysFunctionsToNormalize() ||
           m_graphRange->gridType() ==
               Shared::InteractiveCurveViewRange::GridType::Polar;
  }
  void interestingFunctionRange(
      Shared::ExpiringPointer<Shared::ContinuousFunction> f, float tMin,
      float tMax, float step, float *xm, float *xM, float *ym, float *yM) const;

  Shared::ToggleableRingRoundCursorView m_cursorView;
  BannerView m_bannerView;
  GraphView m_view;
  Shared::InteractiveCurveViewRange *m_graphRange;
  CurveParameterController m_curveParameterController;
  FunctionSelectionController m_functionSelectionController;
  constexpr static int k_numberOfCaches = 5;
  Ion::RingBuffer<PointsOfInterestCache, k_numberOfCaches> m_pointsOfInterest;
};

}  // namespace Graph

#endif
