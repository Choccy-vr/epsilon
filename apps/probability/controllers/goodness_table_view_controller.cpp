#include "goodness_table_view_controller.h"
#include "probability/app.h"

#include <algorithm>

namespace Probability {

GoodnessTableViewController::GoodnessTableViewController(
    Escher::Responder * parent,
    GoodnessStatistic * statistic,
    DynamicSizeTableViewDataSourceDelegate * delegate,
    Escher::SelectableTableViewDelegate * scrollDelegate) :
      TableViewController(parent),
      m_statistic(statistic),
      m_inputTableView(this, statistic, this, this, delegate, scrollDelegate) {
}

bool GoodnessTableViewController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Backspace && m_inputTableView.typeAtLocation(m_inputTableView.selectedColumn(), m_inputTableView.selectedRow()) == InputGoodnessTableView::k_typeOfInnerCells) {
    deleteSelectedValue();
    return true;
  }
  return TableViewController::handleEvent(event);
}

bool GoodnessTableViewController::textFieldDidFinishEditing(Escher::TextField * textField,
                                                            const char * text,
                                                            Ion::Events::Event event) {
  // TODO: factorize with InputCategoricalData & HomogeneityTableViewController
  double p;
  if (textFieldDelegateApp()->hasUndefinedValue(text, &p, false, false)) {
    return false;
  }
  int selectedColumn = m_inputTableView.selectedColumn();
  int selectedRow = m_inputTableView.selectedRow();
  if (!m_statistic->isValidParameterAtPosition(selectedRow - 1, selectedColumn, p)) {
    App::app()->displayWarning(I18n::Message::ForbiddenValue);
    return false;
  }
  m_statistic->setParameterAtPosition(selectedRow - 1, selectedColumn, p);
  if (m_inputTableView.selectedRow() == m_inputTableView.numberOfRows() - 1 &&
      m_inputTableView.numberOfRows() <= m_statistic->maxNumberOfRows()) {
    m_inputTableView.recomputeNumberOfRows();
  }

  m_inputTableView.reloadCellAtLocation(selectedColumn, selectedRow);
  // Select correct cell
  moveSelectionForEvent(event, &selectedRow, &selectedColumn);
  m_inputTableView.selectCellAtClippedLocation(selectedColumn, selectedRow);
  return true;
}

/* TODO : factorize with HomogeneityTableViewController:
 * - Override the statistics parameter position
 * - In DynamicCellsDataSourceDelegate, factorize pointers into virtual methods:
 *   - m_inputTableView / m_innerTableData (as InputHomogeneityDataSource)
 *   - m_statistic (as Chi2Statistic)
 * - The assert still differ, as we don't count number of rows/columns the same
 */
void GoodnessTableViewController::deleteSelectedValue() {
  int row = tableView()->selectedRow(), col = tableView()->selectedColumn();
  assert(row > 0);
  // Remove value
  bool shouldDeleteRow = m_statistic->deleteParameterAtPosition(row - 1, col);
  if (!shouldDeleteRow) {
    // Only one cell needs to reload.
    assert(row < m_inputTableView.numberOfRows() && col < m_inputTableView.numberOfColumns());
    tableView()->reloadCellAtLocation(col, row);
  } else {
    // A row is empty, we should recompute data
    m_statistic->recomputeData();
    // At most one row has been deleted, reselecting the table isn't necessary
    tableView()->deselectTable();
    if (!m_inputTableView.recomputeNumberOfRows()) {
      /* A row has been deleted, but size didn't change, meaning the number of
       * non-empty rows was k_maxNumberOfRows. However, recomputeData may have
       * moved up multiple cells, m_inputTableView should be reloaded. */
      tableView()->reloadData(false);
    }
    tableView()->selectCellAtClippedLocation(col, row, false);
  }
}

void GoodnessTableViewController::initCell(EvenOddEditableTextCell, void * cell, int index) {
  Escher::EvenOddEditableTextCell * c = static_cast<Escher::EvenOddEditableTextCell *>(cell);
  c->setParentResponder(&m_inputTableView);
  c->editableTextCell()->textField()->setDelegates(Probability::App::app(), this);
  c->setEven((index / 2) % 2 == 1);
  c->setFont(KDFont::SmallFont);
}

}  // namespace Probability
