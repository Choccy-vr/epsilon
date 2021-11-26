#include "function_list_controller.h"
#include "function_app.h"

using namespace Escher;

namespace Shared {

FunctionListController::FunctionListController(Responder * parentResponder, ButtonRowController * header, ButtonRowController * footer, I18n::Message text) :
  ExpressionModelListController(parentResponder, text),
  ButtonRowDelegate(header, footer),
  m_selectableTableView(this, this, this, this),
  m_plotButton(this, I18n::Message::Plot, Invocation([](void * context, void * sender) {
      FunctionListController * list = (FunctionListController *)context;
      TabViewController * tabController = list->tabController();
      tabController->setActiveTab(1);
      return true;
    }, this), KDFont::SmallFont, Palette::PurpleBright),
  m_valuesButton(this, I18n::Message::DisplayValues, Invocation([](void * context, void * sender) {
      FunctionListController * list = (FunctionListController *)context;
      TabViewController * tabController = list->tabController();
      tabController->setActiveTab(2);
      return true;
    }, this), KDFont::SmallFont, Palette::PurpleBright)
{
  m_selectableTableView.setMargins(0);
  m_selectableTableView.setVerticalCellOverlap(0);
}

/* TableViewDataSource */

KDCoordinate FunctionListController::cellWidth() {
  return selectableTableView()->bounds().width();
}

int FunctionListController::typeAtIndex(int index) {
  if (isAddEmptyRow(index)) {
    return 1;
  }
  return 0;
}

HighlightCell * FunctionListController::reusableCell(int index, int type) {
  assert(index >= 0 && index < maxNumberOfDisplayableRows());
  if (type == 0) {
    return functionCells(index);
  }
  assert(type == 1);
  return &(m_addNewModel);
}

int FunctionListController::reusableCellCount(int type) {
  if (type == 3) {
    return 2;
  }
  if (type > 3) {
    return 1;
  }
  return maxNumberOfDisplayableRows();
}

void FunctionListController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  assert(isAddEmptyRow(index));
  EvenOddCell * myCell = static_cast<EvenOddCell *>(cell);
  myCell->setEven(index%2 == 0);
  myCell->setHighlighted(index == selectedRow());
  myCell->reloadCell();
}

/* ButtonRowDelegate */

int FunctionListController::numberOfButtons(ButtonRowController::Position position) const {
  if (position == ButtonRowController::Position::Bottom) {
    return 2;
  }
  return 0;
}

Button * FunctionListController::buttonAtIndex(int index, ButtonRowController::Position position) const {
  if (position == ButtonRowController::Position::Top) {
    return nullptr;
  }
  const Button * buttons[2] = {&m_plotButton, &m_valuesButton};
  return (Button *)buttons[index];
}

/* Responder */

void FunctionListController::didBecomeFirstResponder() {
  if (selectedRow() == -1) {
    selectCellAtLocation(0, 0);
  } else {
    selectCellAtLocation(selectedColumn(), selectedRow());
  }
  if (selectedRow() >= numberOfRows()) {
    selectCellAtLocation(selectedColumn(), numberOfRows()-1);
  }
  footer()->setSelectedButton(-1);
  Container::activeApp()->setFirstResponder(selectableTableView());
}

bool FunctionListController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Up) {
    if (selectedRow() == -1) {
      footer()->setSelectedButton(-1);
      selectableTableView()->selectCellAtLocation(0, numberOfRows()-1);
      Container::activeApp()->setFirstResponder(selectableTableView());
      return true;
    }
    selectableTableView()->deselectTable();
    assert(selectedRow() == -1);
    Container::activeApp()->setFirstResponder(tabController());
    return true;
  }
  if (event == Ion::Events::Down) {
    if (selectedRow() == -1) {
      return false;
    }
    selectableTableView()->deselectTable();
    footer()->setSelectedButton(0);
    return true;
  }
  if (selectedRow() < 0) {
    return false;
  }
  // TODO Hugo : Handle button state and call accordingly
  if (selectedColumn() == 0) {
    return handleEventOnExpression(event);
  }
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    configureFunction(modelStore()->recordAtIndex(modelIndexForRow(selectedRow())));
    return true;
  }
  if (event == Ion::Events::Backspace) {
    Ion::Storage::Record record = modelStore()->recordAtIndex(modelIndexForRow(selectedRow()));
    if (removeModelRow(record)) {
      int newSelectedRow = selectedRow() >= numberOfRows() ? numberOfRows()-1 : selectedRow();
      selectCellAtLocation(selectedColumn(), newSelectedRow);
      selectableTableView()->reloadData();
    }
    return true;
  }
  return false;
}

void FunctionListController::didEnterResponderChain(Responder * previousFirstResponder) {
  selectableTableView()->reloadData();
}

void FunctionListController::willExitResponderChain(Responder * nextFirstResponder) {
  if (nextFirstResponder == tabController()) {
    assert(tabController() != nullptr);
    selectableTableView()->deselectTable();
    footer()->setSelectedButton(-1);
  }
}

/* ExpressionModelListController */

StackViewController * FunctionListController::stackController() const {
  return static_cast<StackViewController *>(parentResponder()->parentResponder()->parentResponder());
}

void FunctionListController::configureFunction(Ion::Storage::Record record) {
  StackViewController * stack = stackController();
  parameterController()->setRecord(record);
  stack->push(parameterController());
}

TabViewController * FunctionListController::tabController() const {
  return static_cast<TabViewController *>(parentResponder()->parentResponder()->parentResponder()->parentResponder());
}

InputViewController * FunctionListController::inputController() {
  return FunctionApp::app()->inputViewController();
}

}
