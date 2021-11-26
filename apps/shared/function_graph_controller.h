#ifndef SHARED_FUNCTION_GRAPH_CONTROLLER_H
#define SHARED_FUNCTION_GRAPH_CONTROLLER_H

#include "function_banner_delegate.h"
#include "interactive_curve_view_controller.h"
#include "function_store.h"
#include "function_graph_view.h"
#include "function_curve_parameter_controller.h"

namespace Shared {

class FunctionGraphController : public InteractiveCurveViewController, public FunctionBannerDelegate {
public:
  FunctionGraphController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Escher::ButtonRowController * header,  InteractiveCurveViewRange * interactiveRange, CurveView * curveView, CurveViewCursor * cursor, int * indexFunctionSelectedByCursor, uint32_t * rangeVersion);
  bool isEmpty() const override;
  void didBecomeFirstResponder() override;
  void viewWillAppear() override;

  void computeXRange(float xMinLimit, float xMaxLimit, float * xMin, float * xMax, float * yMinIntrinsic, float * yMaxIntrinsic) override;
  void computeYRange(float xMin, float xMax, float yMinIntrinsic, float yMaxIntrinsic, float * yMin, float * yMax, bool optimizeRange) override;
  void improveFullRange(float * xMin, float * xMax, float * yMin, float * yMax) override;
  void tidyModels() override;

protected:
  class FunctionSelectionController : public CurveSelectionController {
  public:
    FunctionSelectionController(FunctionGraphController * graphController) : CurveSelectionController(graphController) {}
    const char * title() override { return I18n::translate(I18n::Message::GraphCalculus); }
    int numberOfRows() const override { return graphController()->functionStore()->numberOfActiveFunctions(); }
    KDCoordinate rowHeight(int j) override;
    void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  protected:
    FunctionGraphController * graphController() const { return static_cast<FunctionGraphController *>(const_cast<InteractiveCurveViewController *>(m_graphController)); }
    virtual Poincare::Layout nameLayoutAtIndex(int j) const = 0;
  };

  float cursorTopMarginRatio() override { return 0.068f; }
  float cursorBottomMarginRatio() override { return cursorBottomMarginRatioForBannerHeight(bannerView()->minimalSizeForOptimalDisplay().height()); }
  void reloadBannerView() override;
  bool openMenuForCurveAtIndex(int index) override;
  int indexFunctionSelectedByCursor() const { return *m_indexFunctionSelectedByCursor; }
  Escher::Button * calculusButton() const override { return const_cast<Escher::Button * >(&m_calculusButton); }
  virtual void selectFunctionWithCursor(int functionIndex);
  virtual double defaultCursorT(Ion::Storage::Record record);
  virtual FunctionStore * functionStore() const;

  // Closest vertical curve helper
  virtual int nextCurveIndexVertically(bool goingUp, int currentSelectedCurve, Poincare::Context * context, int currentSubCurveIndex, int * subCurveIndex) const {
    return closestCurveIndexVertically(goingUp, currentSelectedCurve, context, currentSubCurveIndex, subCurveIndex);
  }
  bool closestCurveIndexIsSuitable(int newIndex, int currentIndex) const override;
  int selectedCurveRelativePosition() const override { return *m_indexFunctionSelectedByCursor; }
  Poincare::Coordinate2D<double> xyValues(int curveIndex, double t, Poincare::Context * context, int subCurveIndex = 0) const override;
  int numberOfCurves() const override;
  bool hasTwoSubCurves(int curveIndex) const override;
  void initCursorParameters() override;
  bool cursorMatchesModel() override;
  CurveView * curveView() override;

  void yRangeForCursorFirstMove(Shared::InteractiveCurveViewRange * range) const;

private:
  virtual FunctionGraphView * functionGraphView() = 0;
  virtual FunctionCurveParameterController * curveParameterController() = 0;

  // InteractiveCurveViewController
  bool moveCursorVertically(int direction) override;
  uint32_t rangeVersion() override;

  Escher::Button m_calculusButton;
  int * m_indexFunctionSelectedByCursor;
};

}

#endif
