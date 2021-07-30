#ifndef GRAPH_GRAPH_CONTROLLER_H
#define GRAPH_GRAPH_CONTROLLER_H

#include "graph_view.h"
#include "graph_controller_helper.h"
#include "banner_view.h"
#include "curve_parameter_controller.h"
#include "../../shared/function_graph_controller.h"
#include "../../shared/curve_view_cursor.h"
#include "../../shared/round_cursor_view.h"
#include "../../shared/interactive_curve_view_range.h"
#include "../continuous_function_store.h"

namespace Graph {

class GraphController : public Shared::FunctionGraphController, public GraphControllerHelper {
public:
  GraphController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Shared::InteractiveCurveViewRange * curveViewRange, Shared::CurveViewCursor * cursor, int * indexFunctionSelectedByCursor, uint32_t * rangeVersion, Escher::ButtonRowController * header);
  I18n::Message emptyMessage() override;
  void viewWillAppear() override;
  bool displayDerivativeInBanner() const { return m_displayDerivativeInBanner; }
  void setDisplayDerivativeInBanner(bool displayDerivative) { m_displayDerivativeInBanner = displayDerivative; }
private:
  class FunctionSelectionController : public Shared::FunctionGraphController::FunctionSelectionController {
  public:
    FunctionSelectionController(GraphController * graphController) : Shared::FunctionGraphController::FunctionSelectionController(graphController) {}
    CurveSelectionCell * reusableCell(int index, int type) override { assert(index >= 0 && index < k_maxNumberOfDisplayableFunctions); return m_cells + index; }
    int reusableCellCount(int type) override { return k_maxNumberOfDisplayableFunctions; }
  private:
    static constexpr int k_maxNumberOfDisplayableFunctions = 7;
    Poincare::Layout nameLayoutAtIndex(int j) const override;
    CurveSelectionCell m_cells[k_maxNumberOfDisplayableFunctions];
  };

  void selectFunctionWithCursor(int functionIndex) override;
  BannerView * bannerView() override { return &m_bannerView; }
  void reloadBannerView() override;
  bool moveCursorHorizontally(int direction, int scrollSpeed = 1) override;
  int nextCurveIndexVertically(bool goingUp, int currentSelectedCurve, Poincare::Context * context) const override;
  double defaultCursorT(Ion::Storage::Record record) override;
  Shared::InteractiveCurveViewRange * interactiveCurveViewRange() override { return m_graphRange; }
  GraphView * functionGraphView() override { return &m_view; }
  CurveParameterController * curveParameterController() override { return &m_curveParameterController; }
  FunctionSelectionController * curveSelectionController() const override { return const_cast<FunctionSelectionController *>(&m_functionSelectionController); }
  ContinuousFunctionStore * functionStore() const override { return static_cast<ContinuousFunctionStore *>(Shared::FunctionGraphController::functionStore()); }
  bool defaultRangeIsNormalized() const override;
  void interestingFunctionRange(Shared::ExpiringPointer<Shared::ContinuousFunction> f, float tMin, float tMax, float step, float * xm, float * xM, float * ym, float * yM) const;
  void jumpToLeftRightCurve(double t, int direction, int functionsCount, Ion::Storage::Record record) override;

  Shared::RoundCursorView m_cursorView;
  BannerView m_bannerView;
  GraphView m_view;
  Shared::InteractiveCurveViewRange * m_graphRange;
  CurveParameterController m_curveParameterController;
  FunctionSelectionController m_functionSelectionController;
  bool m_displayDerivativeInBanner;
};

}

#endif
