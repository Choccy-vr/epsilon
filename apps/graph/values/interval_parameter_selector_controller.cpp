#include "interval_parameter_selector_controller.h"
#include "../../shared/interval_parameter_controller.h"
#include "../app.h"
#include <apps/i18n.h>
#include <assert.h>

using namespace Escher;

namespace Graph {

IntervalParameterSelectorController::IntervalParameterSelectorController() :
  SelectableListViewController(nullptr)
{
}

const char * IntervalParameterSelectorController::title() {
  return I18n::translate(I18n::Message::IntervalSet);
}

void IntervalParameterSelectorController::viewDidDisappear() {
  /* Deselect the table properly because it needs to be relayouted the next time
   * it appears: the number of rows might change according to the plot type. */
  m_selectableTableView.deselectTable(false);
  m_selectableTableView.setFrame(KDRectZero, false);
  resetMemoization();
}

void IntervalParameterSelectorController::didBecomeFirstResponder() {
  if (selectedRow() < 0) {
    selectCellAtLocation(0, 0);
  }
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool IntervalParameterSelectorController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE || event == Ion::Events::Right) {
    StackViewController * stack = (StackViewController *)parentResponder();
    Shared::IntervalParameterController * controller = App::app()->valuesController()->intervalParameterController();
    Shared::ContinuousFunction::SymbolType symbolType = symbolTypeAtRow(selectedRow());
    controller->setTitle(messageForType(symbolType));
    setStartEndMessages(controller, symbolType);
    controller->setInterval(App::app()->intervalForSymbolType(symbolType));
    stack->push(controller);
    return true;
  }
  return false;
}

int IntervalParameterSelectorController::numberOfRows() const {
  int rowCount = 0;
  size_t symbolTypeIndex = 0;
  Shared::ContinuousFunction::SymbolType symbolType;
  while (symbolTypeIndex < k_numberOfSymbolTypes) {
    symbolType = static_cast<Shared::ContinuousFunction::SymbolType>(symbolTypeIndex);
    bool symbolTypeIsShown = App::app()->functionStore()->numberOfActiveFunctionsOfSymbolType(symbolType) > 0;
    rowCount += symbolTypeIsShown;
    symbolTypeIndex++;
  }
  return rowCount;
}

KDCoordinate IntervalParameterSelectorController::nonMemoizedRowHeight(int j) {
  MessageTableCellWithChevron tempCell;
  return heightForCellAtIndex(&tempCell, j);
}

HighlightCell * IntervalParameterSelectorController::reusableCell(int index, int type) {
  assert(0 <= index && index < reusableCellCount(type));
  return m_intervalParameterCell + index;
}

int IntervalParameterSelectorController::reusableCellCount(int type) {
  return k_numberOfSymbolTypes;
}

void IntervalParameterSelectorController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  assert(0 <= index && index < numberOfRows());
  Shared::ContinuousFunction::SymbolType symbolType = symbolTypeAtRow(index);
  static_cast<MessageTableCellWithChevron *>(cell)->setMessage(messageForType(symbolType));
}

Shared::ContinuousFunction::SymbolType IntervalParameterSelectorController::symbolTypeAtRow(int j) const {
  int rowCount = 0;
  size_t symbolTypeIndex = 0;
  Shared::ContinuousFunction::SymbolType symbolType;
  while (symbolTypeIndex < k_numberOfSymbolTypes) {
    symbolType = static_cast<Shared::ContinuousFunction::SymbolType>(symbolTypeIndex);
    bool symbolTypeIsShown = App::app()->functionStore()->numberOfActiveFunctionsOfSymbolType(symbolType) > 0;
    if (symbolTypeIsShown && rowCount == j) {
      break;
    }
    rowCount += symbolTypeIsShown;
    symbolTypeIndex++;
  }
  assert(rowCount == j);
  return symbolType;
}

I18n::Message IntervalParameterSelectorController::messageForType(Shared::ContinuousFunction::SymbolType symbolType) {
  constexpr I18n::Message message[k_numberOfSymbolTypes] = {
    I18n::Message::IntervalX,
    I18n::Message::IntervalTheta,
    I18n::Message::IntervalT
  };
  return message[static_cast<size_t>(symbolType)];
}

void IntervalParameterSelectorController::setStartEndMessages(Shared::IntervalParameterController * controller, Shared::ContinuousFunction::SymbolType symbolType) {
  if (symbolType == Shared::ContinuousFunction::SymbolType::Theta) {
    controller->setStartEndMessages(I18n::Message::ThetaStart, I18n::Message::ThetaEnd);
  } else if (symbolType == Shared::ContinuousFunction::SymbolType::T) {
    controller->setStartEndMessages(I18n::Message::TStart, I18n::Message::TEnd);
  } else {
    controller->setStartEndMessages(I18n::Message::XStart, I18n::Message::XEnd);
  }
}

}
