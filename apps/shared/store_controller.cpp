#include "store_controller.h"
#include <apps/apps_container.h>
#include <apps/shared/poincare_helpers.h>
#include <apps/constant.h>
#include <escher/metric.h>
#include <assert.h>
#include <algorithm>

using namespace Poincare;
using namespace Escher;

namespace Shared {

StoreController::StoreController(Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, DoublePairStore * store, ButtonRowController * header, Context * parentContext) :
  EditableCellTableViewController(parentResponder),
  ButtonRowDelegate(header, nullptr),
  StoreColumnHelper(this, parentContext, this),
  m_store(store),
  m_prefacedView(0, this, &m_dataView, this),
  m_dataView(m_store, this, this, this, &m_prefacedView)
{
  m_prefacedView.setBackgroundColor(Palette::WallScreenDark);
  m_prefacedView.setCellOverlap(0, 0);
  m_prefacedView.setMargins(k_margin, k_scrollBarMargin, k_scrollBarMargin, k_margin);
  for (int i = 0; i < k_maxNumberOfEditableCells; i++) {
    m_editableCells[i].setParentResponder(&m_dataView);
    m_editableCells[i].editableTextCell()->textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

bool StoreController::textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) {
  int series = m_store->seriesAtColumn(selectedColumn());
  bool wasSeriesValid = m_store->seriesIsValid(series);
  bool result = EditableCellTableViewController::textFieldDidFinishEditing(textField, text, event);
  if (wasSeriesValid != m_store->seriesIsValid(series)) {
    // Series changed validity, series' cells have changed color.
    reloadSeriesVisibleCells(series);
  }
  return result;
}

int StoreController::numberOfColumns() const {
  return DoublePairStore::k_numberOfColumnsPerSeries * DoublePairStore::k_numberOfSeries;
}

KDCoordinate StoreController::columnWidth(int i) {
  return k_cellWidth;
}

KDCoordinate StoreController::cumulatedWidthFromIndex(int i) {
  return i*k_cellWidth;
}

int StoreController::indexFromCumulatedWidth(KDCoordinate offsetX) {
  return (offsetX-1) / k_cellWidth;
}

HighlightCell * StoreController::reusableCell(int index, int type) {
  assert(index >= 0);
  switch (type) {
    case k_titleCellType:
      assert(index < k_numberOfTitleCells);
      return &m_titleCells[index];
    case k_editableCellType:
      assert(index < k_maxNumberOfEditableCells);
      return &m_editableCells[index];
    default:
      assert(false);
      return nullptr;
  }
}

int StoreController::reusableCellCount(int type) {
  return type == k_titleCellType ? k_numberOfTitleCells : k_maxNumberOfEditableCells;
}

int StoreController::typeAtLocation(int i, int j) {
  return j == 0 ? k_titleCellType : k_editableCellType;
}

void StoreController::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  // Handle hidden cells
  const int numberOfElementsInCol = numberOfElementsInColumn(i);
  if (j > numberOfElementsInCol + 1) {
    StoreCell * myCell = static_cast<StoreCell *>(cell);
    myCell->editableTextCell()->textField()->setText("");
    myCell->setHide(true);
    return;
  }
  if (typeAtLocation(i, j) == k_editableCellType) {
    Shared::StoreCell * myCell = static_cast<StoreCell *>(cell);
    myCell->setHide(false);
    myCell->setSeparatorLeft(i > 0 && (m_store->relativeColumnIndex(i) == 0));
    KDColor textColor = (m_store->seriesIsValid(m_store->seriesAtColumn(i)) || m_store->numberOfPairsOfSeries(m_store->seriesAtColumn(i)) == 0) ? KDColorBlack : Palette::GrayDark;
    myCell->editableTextCell()->textField()->setTextColor(textColor);
  }
  willDisplayCellAtLocationWithDisplayMode(cell, i, j, Preferences::sharedPreferences()->displayMode());
}

void StoreController::setTitleCellText(HighlightCell * cell, int columnIndex) {
  // Default : put column name in titleCell
  StoreTitleCell * myTitleCell = static_cast<StoreTitleCell *>(cell);
  fillColumnName(columnIndex, const_cast<char *>(myTitleCell->text()));
}

void StoreController::setTitleCellStyle(HighlightCell * cell, int columnIndex) {
  int seriesIndex = m_store->seriesAtColumn(columnIndex);
  int realColumnIndex = m_store->relativeColumnIndex(columnIndex);
  Shared::StoreTitleCell * myCell = static_cast<Shared::StoreTitleCell *>(cell);
  myCell->setColor(!m_store->seriesIsValid(seriesIndex) ? Palette::GrayDark : DoublePairStore::colorOfSeriesAtIndex(seriesIndex)); // TODO Share GrayDark with graph/list_controller
  myCell->setSeparatorLeft(columnIndex > 0 && ( realColumnIndex == 0));
}

const char * StoreController::title() {
  return I18n::translate(I18n::Message::DataTab);
}

bool StoreController::handleEvent(Ion::Events::Event event) {
  if (EditableCellTableViewController::handleEvent(event)) {
    return true;
  }
  int i = selectedColumn();
  int j = selectedRow();
  if (event == Ion::Events::Up) {
    selectableTableView()->deselectTable();
    assert(j == -1);
    Container::activeApp()->setFirstResponder(tabController());
    return true;
  }
  assert(i >= 0 && i < numberOfColumns());
  int series = m_store->seriesAtColumn(i);
  if (event == Ion::Events::Backspace) {
    if (j == 0 || j > numberOfElementsInColumn(i)) {
      return false;
    }
    if (deleteCellValue(series, i, j)) {
      // A row has been deleted
      selectableTableView()->reloadData();
    } else {
      reloadSeriesVisibleCells(series);
    }
    return true;
  }
  return false;
}

void StoreController::didBecomeFirstResponder() {
  if (selectedRow() < 0 || selectedColumn() < 0) {
    selectCellAtLocation(0, 0);
  }
  EditableCellTableViewController::didBecomeFirstResponder();
}

bool StoreController::deleteCellValue(int series, int i, int j) {
  return m_store->deleteValueAtIndex(series, m_store->relativeColumnIndex(i), j - 1);
}

StackViewController * StoreController::stackController() const {
  return static_cast<StackViewController *>(parentResponder()->parentResponder());
}

Responder * StoreController::tabController() const {
  return (parentResponder()->parentResponder()->parentResponder());
}

bool StoreController::cellAtLocationIsEditable(int columnIndex, int rowIndex) {
  return typeAtLocation(columnIndex, rowIndex) == k_editableCellType;
}

bool StoreController::setDataAtLocation(double floatBody, int columnIndex, int rowIndex) {
  m_store->set(floatBody, m_store->seriesAtColumn(columnIndex), m_store->relativeColumnIndex(columnIndex), rowIndex-1, false, true);
  return true;
}

double StoreController::dataAtLocation(int columnIndex, int rowIndex) {
  return m_store->get(m_store->seriesAtColumn(columnIndex), m_store->relativeColumnIndex(columnIndex), rowIndex-1);
}

int StoreController::numberOfElementsInColumn(int columnIndex) const {
  return m_store->numberOfPairsOfSeries(m_store->seriesAtColumn(columnIndex));
}

}
