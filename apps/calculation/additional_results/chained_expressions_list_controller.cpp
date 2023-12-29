#include "chained_expressions_list_controller.h"

#include "../app.h"
#include "apps/calculation/additional_results/expressions_list_controller.h"

using namespace Poincare;
using namespace Escher;

namespace Calculation {

void ChainedExpressionsListController::viewDidDisappear() {
  if (m_tail) {
    m_tail->viewDidDisappear();
  }
  ExpressionsListController::viewDidDisappear();
}

void ChainedExpressionsListController::didBecomeFirstResponder() {
  /* Width needs to be reinit if tail of controller changed and wasn't already
   * initialized. */
  initWidth(m_listController.selectableListView());
  ExpressionsListController::didBecomeFirstResponder();
}

int ChainedExpressionsListController::reusableCellCount(int type) {
  return (type == k_expressionCellType ? k_maxNumberOfRows : 0) +
         (m_tail ? m_tail->reusableCellCount(type) : 0);
}

HighlightCell* ChainedExpressionsListController::reusableCell(int index,
                                                              int type) {
  if (type != k_expressionCellType) {
    return m_tail->reusableCell(index, type);
  }
  int numberOfOwnedCells = ExpressionsListController::reusableCellCount(type);
  if (index >= numberOfOwnedCells) {
    return m_tail->reusableCell(index - numberOfOwnedCells, type);
  }
  return &m_cells[index];
}

void ChainedExpressionsListController::initWidth(Escher::TableView* tableView) {
  ExpressionsListController::initWidth(tableView);
  if (m_tail) {
    m_tail->initWidth(tableView);
  }
}

int ChainedExpressionsListController::typeAtRow(int row) const {
  int numberOfOwnedCells = ExpressionsListController::numberOfRows();
  if (row >= numberOfOwnedCells) {
    return m_tail->typeAtRow(row - numberOfOwnedCells);
  }
  return ExpressionsListController::typeAtRow(row);
}

KDCoordinate ChainedExpressionsListController::nonMemoizedRowHeight(int row) {
  int numberOfOwnedCells = ExpressionsListController::numberOfRows();
  if (row >= numberOfOwnedCells) {
    return m_tail->nonMemoizedRowHeight(row - numberOfOwnedCells);
  }
  return ExpressionsListController::nonMemoizedRowHeight(row);
}

void ChainedExpressionsListController::fillCellForRow(HighlightCell* cell,
                                                      int row) {
  int numberOfOwnedCells = ExpressionsListController::numberOfRows();
  if (row >= numberOfOwnedCells) {
    return m_tail->fillCellForRow(cell, row - numberOfOwnedCells);
  }
  return ExpressionsListController::fillCellForRow(cell, row);
}

int ChainedExpressionsListController::numberOfRows() const {
  return ExpressionsListController::numberOfRows() +
         (m_tail ? m_tail->numberOfRows() : 0);
}

size_t ChainedExpressionsListController::textAtIndex(char* buffer,
                                                     size_t bufferSize,
                                                     HighlightCell* cell,
                                                     int index) {
  int numberOfOwnedCells = ExpressionsListController::numberOfRows();
  if (index >= numberOfOwnedCells) {
    return m_tail->textAtIndex(buffer, bufferSize, cell,
                               index - numberOfOwnedCells);
  }
  return ExpressionsListController::textAtIndex(buffer, bufferSize, cell,
                                                index);
}

}  // namespace Calculation
