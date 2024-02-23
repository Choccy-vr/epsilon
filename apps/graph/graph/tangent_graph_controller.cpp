#include "tangent_graph_controller.h"

#include <apps/apps_container_helper.h>
#include <apps/shared/poincare_helpers.h>
#include <poincare/preferences.h>
#include <poincare/print.h>

#include "../app.h"

using namespace Shared;
using namespace Poincare;
using namespace Escher;

namespace Graph {

TangentGraphController::TangentGraphController(
    Responder *parentResponder, GraphView *graphView, BannerView *bannerView,
    InteractiveCurveViewRange *curveViewRange, CurveViewCursor *cursor)
    : SimpleInteractiveCurveViewController(parentResponder, cursor),
      m_graphView(graphView),
      m_bannerView(bannerView),
      m_graphRange(curveViewRange) {}

const char *TangentGraphController::title() {
  return I18n::translate(I18n::Message::Tangent);
}

void TangentGraphController::viewWillAppear() {
  SimpleInteractiveCurveViewController::viewWillAppear();
  m_graphView->setTangentDisplay(true);
  m_graphView->setFocus(true);
  m_bannerView->setDisplayParameters(false, true, false, false, true);
  reloadBannerView();
  panToMakeCursorVisible();
  SimpleInteractiveCurveViewController::viewWillAppear();
}

void TangentGraphController::didBecomeFirstResponder() {
  if (curveView()->hasFocus()) {
    m_bannerView->abscissaValue()->setParentResponder(this);
    m_bannerView->abscissaValue()->setDelegate(this);
    App::app()->setFirstResponder(m_bannerView->abscissaValue());
  }
}

bool TangentGraphController::textFieldDidFinishEditing(
    AbstractTextField *textField, Ion::Events::Event event) {
  double floatBody = ParseInputFloatValue<double>(textField->draftText());
  if (HasUndefinedValue(floatBody)) {
    return false;
  }
  ExpiringPointer<ContinuousFunction> f = function();
  assert(f->properties().isCartesian());
  double y =
      f->evaluate2DAtParameter(floatBody, App::app()->localContext()).y();
  m_cursor->moveTo(floatBody, floatBody, y);
  panToMakeCursorVisible();
  reloadBannerView();
  curveView()->reload();
  return true;
}

void TangentGraphController::setRecord(Ion::Storage::Record record) {
  m_graphView->selectRecord(record);
  m_record = record;
}

void TangentGraphController::reloadBannerView() {
  if (m_record.isNull()) {
    return;
  }
  FunctionBannerDelegate::reloadBannerViewForCursorOnFunction(
      m_cursor, m_record, FunctionApp::app()->functionStore(),
      AppsContainerHelper::sharedAppsContainerGlobalContext());

  constexpr size_t bufferSize = FunctionBannerDelegate::k_textBufferSize;
  char buffer[bufferSize];
  int precision = numberOfSignificantDigits();

  Evaluation<double> derivative =
      reloadDerivativeInBannerViewForCursorOnFunction(m_cursor, m_record, true);
  assert(derivative.type() == EvaluationNode<double>::Type::Complex);
  double coefficientA = derivative.toScalar();

  Print::CustomPrintf(buffer, bufferSize, "a=%*.*ed", coefficientA,
                      Preferences::sharedPreferences->displayMode(), precision);
  m_bannerView->aView()->setText(buffer);

  double coefficientB = -coefficientA * m_cursor->x() + m_cursor->y();
  Print::CustomPrintf(buffer, bufferSize, "b=%*.*ed", coefficientB,
                      Preferences::sharedPreferences->displayMode(), precision);
  m_bannerView->bView()->setText(buffer);
  m_bannerView->reload();
}

bool TangentGraphController::moveCursorHorizontally(
    OMG::HorizontalDirection direction, int scrollSpeed) {
  return privateMoveCursorHorizontally(m_cursor, direction, m_graphRange,
                                       k_numberOfCursorStepsInGradUnit,
                                       m_record, curveView()->pixelWidth());
}

bool TangentGraphController::handleEnter() {
  StackViewController *stack =
      static_cast<StackViewController *>(parentResponder());
  stack->pop();
  return true;
}

ExpiringPointer<ContinuousFunction> TangentGraphController::function() const {
  return App::app()->functionStore()->modelForRecord(m_record);
}

}  // namespace Graph
