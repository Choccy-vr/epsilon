#include "function_parameter_controller.h"

#include <apps/shared/poincare_helpers.h>
#include <assert.h>
#include <escher/metric.h>
#include <poincare/print.h>

#include <array>

#include "../app.h"

using namespace Shared;
using namespace Poincare;
using namespace Escher;

namespace Graph {

FunctionParameterController::FunctionParameterController(
    Responder *parentResponder, I18n::Message functionColorMessage,
    I18n::Message deleteFunctionMessage)
    : Shared::ListParameterController(parentResponder, functionColorMessage,
                                      deleteFunctionMessage),
      m_detailsParameterController(nullptr),
      m_domainParameterController(nullptr),
      m_useColumnTitle(false) {
  m_detailsCell.label()->setMessage(I18n::Message::Details);
  m_derivativeCell.label()->setMessage(I18n::Message::GraphDerivative);
  m_functionDomainCell.label()->setMessage(I18n::Message::FunctionDomain);
}

const char *FunctionParameterController::title() {
  return m_useColumnTitle ? m_titleBuffer
                          : I18n::translate(I18n::Message::Options);
}

HighlightCell *FunctionParameterController::cell(int row) {
  assert(0 <= row && row < numberOfRows());
  HighlightCell *const cells[] = {&m_detailsCell,    &m_colorCell,
                                  &m_derivativeCell, &m_functionDomainCell,
                                  &m_enableCell,     &m_deleteCell};
  static_assert(std::size(cells) == k_numberOfRows);
  return cells[row];
}

void FunctionParameterController::setRecord(Ion::Storage::Record record) {
  Shared::ListParameterController::setRecord(record);
  /* Set controllers' record here because we need to know which ones should be
   * displayed. */
  m_detailsParameterController.setRecord(m_record);
  m_domainParameterController.setRecord(m_record);
  bool displayDerivative = !m_record.isNull() && App::app()
                                                     ->functionStore()
                                                     ->modelForRecord(m_record)
                                                     ->canDisplayDerivative();
  m_derivativeCell.setVisible(displayDerivative);
  m_detailsCell.setVisible(displayDetails());
  m_functionDomainCell.setVisible(displayDomain());
  m_selectableListView.resetSizeAndOffsetMemoization();
}

const char *intervalBracket(double value, bool opening) {
  if (std::isinf(value)) {
    return GlobalPreferences::sharedGlobalPreferences->openIntervalChar(
        opening);
  }
  return opening ? "[" : "]";
}

int writeInterval(char *buffer, int bufferSize, double min, double max,
                  int numberOfSignificantDigits,
                  Preferences::PrintFloatMode mode) {
  return Poincare::Print::CustomPrintf(
      buffer, bufferSize, "%s%*.*ed,%*.*ed%s", intervalBracket(min, true), min,
      mode, numberOfSignificantDigits, max, mode, numberOfSignificantDigits,
      intervalBracket(max, false));
}

void FunctionParameterController::viewWillAppear() {
  updateDerivaticeCellSwitch();
  if (!m_record.isNull()) {
    App *myApp = App::app();
    Shared::ExpiringPointer<ContinuousFunction> function =
        myApp->functionStore()->modelForRecord(m_record);
    m_detailsCell.subLabel()->setMessage(function->properties().caption());
    double min = function->tMin();
    double max = function->tMax();
    constexpr int bufferSize = OneLineBufferTextView<>::MaxTextSize();
    char buffer[bufferSize];
    writeInterval(buffer, bufferSize, min, max,
                  Preferences::VeryShortNumberOfSignificantDigits,
                  Preferences::sharedPreferences->displayMode());
    // Cell's layout will adapt to fit the subLabel.
    m_functionDomainCell.subLabel()->setText(buffer);
  }
  Shared::ListParameterController::viewWillAppear();
}

bool FunctionParameterController::handleEvent(Ion::Events::Event event) {
  HighlightCell *cell = selectedCell();
  StackViewController *stack =
      static_cast<StackViewController *>(parentResponder());
  if (cell == &m_detailsCell && m_detailsCell.canBeActivatedByEvent(event)) {
    stack->push(&m_detailsParameterController);
    return true;
  }
  if (cell == &m_functionDomainCell &&
      m_functionDomainCell.canBeActivatedByEvent(event)) {
    stack->push(&m_domainParameterController);
    return true;
  }
  if (cell == &m_derivativeCell &&
      m_derivativeCell.canBeActivatedByEvent(event)) {
    function()->setDisplayDerivative(!function()->displayDerivative());
    updateDerivaticeCellSwitch();
    m_selectableListView.reloadSelectedCell();
    return true;
  }
  bool result = Shared::ListParameterController::handleEvent(event);
  // We want left to pop into graph -> calculate but not into list
  if (!result && event == Ion::Events::Left &&
      stack->depth() >
          InteractiveCurveViewController::k_graphControllerStackDepth + 1) {
    stack->pop();
    return true;
  }
  return result;
}

ExpiringPointer<ContinuousFunction> FunctionParameterController::function() {
  return App::app()->functionStore()->modelForRecord(m_record);
}

void FunctionParameterController::initializeColumnParameters() {
  setUseColumnTitle(true);
  Shared::ColumnParameters::initializeColumnParameters();
}

Shared::ColumnNameHelper *FunctionParameterController::columnNameHelper() {
  return App::app()->valuesController();
}

}  // namespace Graph
