#include "function_graph_controller.h"
#include "function_app.h"
#include "poincare_helpers.h"
#include "../apps_container.h"
#include <poincare/coordinate_2D.h>
#include <poincare/layout_helper.h>
#include <assert.h>
#include <cmath>
#include <float.h>
#include <algorithm>

using namespace Escher;
using namespace Poincare;

namespace Shared {

FunctionGraphController::FunctionGraphController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, ButtonRowController * header, InteractiveCurveViewRange * interactiveRange, CurveView * curveView, CurveViewCursor * cursor, int * indexFunctionSelectedByCursor, uint32_t * rangeVersion) :
  InteractiveCurveViewController(parentResponder, inputEventHandlerDelegate, header, interactiveRange, curveView, cursor, rangeVersion),
  m_indexFunctionSelectedByCursor2(0),
  m_calculusButton(this, I18n::Message::GraphCalculus, calculusButtonInvocation(), KDFont::SmallFont),
  m_indexFunctionSelectedByCursor(indexFunctionSelectedByCursor)
{
}

bool FunctionGraphController::isEmpty() const {
  if (functionStore()->numberOfActiveFunctions() == 0) {
    return true;
  }
  return false;
}

void FunctionGraphController::didBecomeFirstResponder() {
  if (curveView()->isMainViewSelected()) {
    bannerView()->abscissaValue()->setParentResponder(this);
    bannerView()->abscissaValue()->setDelegates(textFieldDelegateApp(), this);
    Container::activeApp()->setFirstResponder(bannerView()->abscissaValue());
  } else {
    InteractiveCurveViewController::didBecomeFirstResponder();
  }
}

void FunctionGraphController::viewWillAppear() {
  functionGraphView()->setBannerView(bannerView());
  functionGraphView()->setAreaHighlight(NAN,NAN);

  if (functionGraphView()->context() == nullptr) {
    functionGraphView()->setContext(textFieldDelegateApp()->localContext());
  }

  InteractiveCurveViewController::viewWillAppear();
}

bool FunctionGraphController::openMenuForCurveAtIndex(int index) {
  if (index != *m_indexFunctionSelectedByCursor) {
    *m_indexFunctionSelectedByCursor = index;
    Coordinate2D<double> xy = xyValues(index, m_cursor->t(), textFieldDelegateApp()->localContext());
    m_cursor->moveTo(m_cursor->t(), xy.x1(), xy.x2());
  }
  Ion::Storage::Record record = functionStore()->activeRecordAtIndex(indexFunctionSelectedByCursor());
  curveParameterController()->setRecord(record);
  StackViewController * stack = stackController();
  stack->push(curveParameterController());
  return true;
}

void FunctionGraphController::selectFunctionWithCursor(int functionIndex) {
  *m_indexFunctionSelectedByCursor = functionIndex;
}

KDCoordinate FunctionGraphController::FunctionSelectionController::rowHeight(int j) {
  assert(j < graphController()->functionStore()->numberOfActiveFunctions());
  ExpiringPointer<ContinuousFunction> function = graphController()->functionStore()->modelForRecord(graphController()->functionStore()->activeRecordAtIndex(j));
  return std::max(function->layout().layoutSize().height(), nameLayoutAtIndex(j).layoutSize().height()) + Metric::CellTopMargin + Metric::CellBottomMargin + Metric::CellSeparatorThickness;
}

void FunctionGraphController::FunctionSelectionController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  assert(index < graphController()->functionStore()->numberOfActiveFunctions());
  ExpiringPointer<ContinuousFunction> function = graphController()->functionStore()->modelForRecord(graphController()->functionStore()->activeRecordAtIndex(index));
  static_cast<CurveSelectionCell *>(cell)->setLayout(function->layout().clone());
}

void FunctionGraphController::reloadBannerView() {
  assert(functionStore()->numberOfActiveFunctions() > 0);
  Ion::Storage::Record record = functionStore()->activeRecordAtIndex(indexFunctionSelectedByCursor());
  reloadBannerViewForCursorOnFunction(m_cursor, record, functionStore(), AppsContainer::sharedAppsContainer()->globalContext());
}

double FunctionGraphController::defaultCursorT(Ion::Storage::Record record) {
  Poincare::Context * context = textFieldDelegateApp()->localContext();
  ExpiringPointer<ContinuousFunction> function = functionStore()->modelForRecord(record);
  float gridUnit = 2 * interactiveCurveViewRange()->xGridUnit();

  float yMin = interactiveCurveViewRange()->yMin(), yMax = interactiveCurveViewRange()->yMax();
  float middle = (interactiveCurveViewRange()->xMin()+interactiveCurveViewRange()->xMax())/2.0f;
  float resLeft = gridUnit * std::floor(middle / gridUnit);
  float yLeft = function->evaluateXYAtParameter(resLeft, context, m_indexFunctionSelectedByCursor2).x2();
  float resRight = resLeft + gridUnit;
  float yRight = function->evaluateXYAtParameter(resRight, context, m_indexFunctionSelectedByCursor2).x2();
  if ((yMin < yLeft && yLeft < yMax) || !(yMin < yRight && yRight < yMax)) {
    return resLeft;
  }
  return resRight;
}

ContinuousFunctionStore * FunctionGraphController::functionStore() const {
  return FunctionApp::app()->functionStore();
}

void FunctionGraphController::initCursorParameters() {
  Poincare::Context * context = textFieldDelegateApp()->localContext();
  const int activeFunctionsCount = functionStore()->numberOfActiveFunctions();
  int functionIndex = 0;
  Coordinate2D<double> xy;
  double t;
  do {
    Ion::Storage::Record record = functionStore()->activeRecordAtIndex(functionIndex);
    ExpiringPointer<ContinuousFunction> firstFunction = functionStore()->modelForRecord(record);
    t = defaultCursorT(record);
    xy = firstFunction->evaluateXYAtParameter(t, context, 0);
  } while ((std::isnan(xy.x2()) || std::isinf(xy.x2())) && ++functionIndex < activeFunctionsCount);
  if (functionIndex == activeFunctionsCount) {
    functionIndex = 0;
  }
  m_cursor->moveTo(t, xy.x1(), xy.x2());
  selectFunctionWithCursor(functionIndex);
}

bool FunctionGraphController::moveCursorVertically(int direction) {
  int currentActiveFunctionIndex = indexFunctionSelectedByCursor();
  Poincare::Context * context = textFieldDelegateApp()->localContext();
  int nextActiveFunctionIndex = -1;
  ExpiringPointer<ContinuousFunction> f = functionStore()->modelForRecord(functionStore()->activeRecordAtIndex(currentActiveFunctionIndex));
  if (f->hasTwoCurves() && m_indexFunctionSelectedByCursor2 - direction >= 0 && m_indexFunctionSelectedByCursor2 - direction <= 1 ) {
    m_indexFunctionSelectedByCursor2 -= direction;
  } else {
    nextActiveFunctionIndex = nextCurveIndexVertically(direction > 0, currentActiveFunctionIndex, context);
    if (nextActiveFunctionIndex < 0) {
      return false;
    }
    // Clip the current t to the domain of the next function
    f = functionStore()->modelForRecord(functionStore()->activeRecordAtIndex(nextActiveFunctionIndex));
  }
  double clippedT = m_cursor->t();
  if (!std::isnan(f->tMin())) {
    assert(!std::isnan(f->tMax()));
    clippedT = std::min<double>(f->tMax(), std::max<double>(f->tMin(), clippedT));
  }
  if (nextActiveFunctionIndex >= 0) {
    if (direction == -1 && f->hasTwoCurves()) {
      m_indexFunctionSelectedByCursor2 = 1;
    } else {
      m_indexFunctionSelectedByCursor2 = 0;
    }
  }
  Poincare::Coordinate2D<double> cursorPosition = f->evaluateXYAtParameter(clippedT, context, m_indexFunctionSelectedByCursor2);
  m_cursor->moveTo(clippedT, cursorPosition.x1(), cursorPosition.x2());
  if (nextActiveFunctionIndex >= 0) {
    selectFunctionWithCursor(nextActiveFunctionIndex);
  }
  return true;
}

bool FunctionGraphController::cursorMatchesModel() {
  Poincare::Context * context = textFieldDelegateApp()->localContext();
  if (indexFunctionSelectedByCursor() >= functionStore()->numberOfActiveFunctions()) {
    return false;
  }
  ExpiringPointer<ContinuousFunction> f = functionStore()->modelForRecord(functionStore()->activeRecordAtIndex(indexFunctionSelectedByCursor()));
  Coordinate2D<double> xy = f->evaluateXYAtParameter(m_cursor->t(), context, m_indexFunctionSelectedByCursor2);
  return PoincareHelpers::equalOrBothNan(xy.x1(), m_cursor->x()) && PoincareHelpers::equalOrBothNan(xy.x2(), m_cursor->y());
}

CurveView * FunctionGraphController::curveView() {
  return functionGraphView();
}

uint32_t FunctionGraphController::rangeVersion() {
  return interactiveCurveViewRange()->rangeChecksum();
}

bool FunctionGraphController::closestCurveIndexIsSuitable(int newIndex, int currentIndex) const {
  return newIndex != currentIndex;
}

Coordinate2D<double> FunctionGraphController::xyValues(int curveIndex, double t, Poincare::Context * context) const {
  return functionStore()->modelForRecord(functionStore()->activeRecordAtIndex(curveIndex))->evaluateXYAtParameter(t, context, (curveIndex == *m_indexFunctionSelectedByCursor ? m_indexFunctionSelectedByCursor2 : 0));
}

int FunctionGraphController::numberOfCurves() const {
  return functionStore()->numberOfActiveFunctions();
}

void FunctionGraphController::computeXRange(float xMinLimit, float xMaxLimit, float * xMin, float * xMax, float * yMinIntrinsic, float * yMaxIntrinsic) {
  DefaultComputeXRange(xMinLimit, xMaxLimit, xMin, xMax, yMinIntrinsic, yMaxIntrinsic, textFieldDelegateApp()->localContext(), functionStore());
}

void FunctionGraphController::computeYRange(float xMin, float xMax, float yMinIntrinsic, float yMaxIntrinsic, float * yMin, float * yMax, bool optimizeRange) {
  float ratio = InteractiveCurveViewRange::NormalYXRatio() * (1 - cursorTopMarginRatio() - cursorBottomMarginRatio());
  DefaultComputeYRange(xMin, xMax, yMinIntrinsic, yMaxIntrinsic, ratio, yMin, yMax, textFieldDelegateApp()->localContext(), functionStore(), optimizeRange);
}

void FunctionGraphController::improveFullRange(float * xMin, float * xMax, float * yMin, float * yMax) {
  DefaultImproveFullRange(xMin, xMax, yMin, yMax, textFieldDelegateApp()->localContext(), functionStore());
}

void FunctionGraphController::tidyModels() {
  int nbOfFunctions = functionStore()->numberOfActiveFunctions();
  for (int i = 0; i < nbOfFunctions; i++) {
    ExpiringPointer<ContinuousFunction> f = functionStore()->modelForRecord(functionStore()->activeRecordAtIndex(i));
    f->tidy();
  }
}

}
