#ifndef SHARED_INTERACTIVE_CURVE_VIEW_CONTROLLER_H
#define SHARED_INTERACTIVE_CURVE_VIEW_CONTROLLER_H

#include <escher/alternate_empty_view_delegate.h>
#include <escher/button_row_controller.h>
#include <escher/button_state.h>
#include <escher/toggleable_dot_view.h>
#include <escher/unequal_view.h>
#include <poincare/coordinate_2D.h>
#include "cursor_view.h"
#include "curve_selection_controller.h"
#include "function_zoom_and_pan_curve_view_controller.h"
#include "range_parameter_controller.h"
#include "simple_interactive_curve_view_controller.h"

namespace Shared {

class InteractiveCurveViewController : public SimpleInteractiveCurveViewController, public InteractiveCurveViewRangeDelegate, public Escher::ButtonRowDelegate, public Escher::AlternateEmptyViewDefaultDelegate {
public:
  static constexpr int k_graphControllerStackDepth = 1;

  InteractiveCurveViewController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Escher::ButtonRowController * header, InteractiveCurveViewRange * interactiveRange, CurveView * curveView, CurveViewCursor * cursor, uint32_t * rangeVersion);

  const char * title() override;
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
  TELEMETRY_ID("Graph");

  RangeParameterController * rangeParameterController();
  ViewController * zoomParameterController();

  int numberOfButtons(Escher::ButtonRowController::Position position) const override;
  Escher::Button * buttonAtIndex(int index, Escher::ButtonRowController::Position position) const override;

  Responder * defaultController() override;

  void viewWillAppear() override;
  void viewDidDisappear() override;
  void willExitResponderChain(Responder * nextFirstResponder) override;
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;
  bool textFieldDidReceiveEvent(Escher::TextField * textField, Ion::Events::Event event) override;

  virtual bool openMenuForCurveAtIndex(int index) = 0;

protected:
  Responder * tabController() const;
  virtual Escher::StackViewController * stackController() const;
  virtual void initCursorParameters() = 0;
  virtual bool moveCursorVertically(int direction) = 0;
  virtual uint32_t rangeVersion() = 0;
  bool isCursorVisible();
  // The cursor does not match if selected model has been edited or deleted
  virtual bool cursorMatchesModel() = 0;
  bool openMenu() { return openMenuForCurveAtIndex(selectedCurveRelativePosition()); };
  virtual CurveSelectionController * curveSelectionController() const = 0;
  virtual Escher::Button * calculusButton() const = 0;
  Escher::Invocation calculusButtonInvocation();

  // Closest vertical curve helper
  int closestCurveIndexVertically(bool goingUp, int currentSelectedCurve, Poincare::Context * context) const;
  virtual bool closestCurveIndexIsSuitable(int newIndex, int currentIndex) const = 0;
  virtual int selectedCurveRelativePosition() const = 0;
  virtual Poincare::Coordinate2D<double> xyValues(int curveIndex, double t, Poincare::Context * context) const = 0;
  virtual bool suitableYValue(double y) const { return true; }
  virtual int numberOfCurves() const = 0;

  // SimpleInteractiveCurveViewController
  bool handleEnter() override { return openMenu(); }

  InteractiveCurveViewRange * interactiveRange() { return m_interactiveRange; }

private:
  /* The value 21 is the actual height of the ButtonRow, that is
   * ButtonRowController::ContentView::k_plainStyleHeight + 1.
   * That value is not public though. */
  constexpr static float k_viewHeight = Ion::Display::Height - Escher::Metric::TitleBarHeight - Escher::Metric::TabHeight - 21;
  void refreshCursor();

  // InteractiveCurveViewRangeDelegate
  float addMargin(float x, float range, bool isVertical, bool isMin) override;
  void updateZoomButtons() override;
  void updateBottomMargin() override { refreshCursor(); }

  void setCurveViewAsMainView();

  void navigationButtonAction();
  /* This method returns the new status for the button,
   * ie m_interactiveRange->zoomAuto(). */
  bool autoButtonAction();

  uint32_t * m_rangeVersion;
  RangeParameterController m_rangeParameterController;
  FunctionZoomAndPanCurveViewController m_zoomParameterController;
  InteractiveCurveViewRange * m_interactiveRange;
  Escher::ButtonState m_autoButton;
  Escher::ToggleableDotView m_autoDotView;
  Escher::Button m_navigationButton;
  Escher::ButtonState m_rangeButton;
  Escher::UnequalView m_rangeUnequalView;
};

}

#endif
