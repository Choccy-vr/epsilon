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

StoreController::ContentView::ContentView(DoublePairStore * store, Responder * parentResponder, TableViewDataSource * dataSource, SelectableTableViewDataSource * selectionDataSource, InputEventHandlerDelegate * inputEventHandlerDelegate, TextFieldDelegate * textFieldDelegate) :
  View(),
  Responder(parentResponder),
  m_dataView(store, this, dataSource, selectionDataSource),
  m_formulaInputView(this, inputEventHandlerDelegate, textFieldDelegate),
  m_displayFormulaInputView(false)
{
  m_dataView.setBackgroundColor(Palette::WallScreenDark);
  m_dataView.setVerticalCellOverlap(0);
  m_dataView.setMargins(k_margin, k_scrollBarMargin, k_scrollBarMargin, k_margin);
}

void StoreController::ContentView::displayFormulaInput(bool display) {
  if (m_displayFormulaInputView != display) {
    if (display) {
      m_formulaInputView.textField()->setText("");
    }
    m_displayFormulaInputView = display;
    layoutSubviews();
    markRectAsDirty(bounds());
  }
}

void StoreController::ContentView::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(m_displayFormulaInputView ? static_cast<Responder *>(&m_formulaInputView) : static_cast<Responder *>(&m_dataView));
}

View * StoreController::ContentView::subviewAtIndex(int index) {
  assert(index >= 0 && index < numberOfSubviews());
  View * views[] = {&m_dataView, &m_formulaInputView};
  return views[index];
}

void StoreController::ContentView::layoutSubviews(bool force) {
  KDRect dataViewFrame(0, 0, bounds().width(), bounds().height() - (m_displayFormulaInputView ? k_formulaInputHeight : 0));
  m_dataView.setFrame(dataViewFrame, force);
  m_formulaInputView.setFrame(formulaFrame(), force);
}

KDRect StoreController::ContentView::formulaFrame() const {
  return KDRect(0, bounds().height() - k_formulaInputHeight, bounds().width(), m_displayFormulaInputView ? k_formulaInputHeight : 0);
}

StoreController::StoreController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, DoublePairStore * store, ButtonRowController * header) :
  EditableCellTableViewController(parentResponder),
  ButtonRowDelegate(header, nullptr),
  m_store(store),
  m_contentView(m_store, this, this, this, inputEventHandlerDelegate, this)
{
  for (int i = 0; i < k_maxNumberOfEditableCells; i++) {
    m_editableCells[i].setParentResponder(m_contentView.dataView());
    m_editableCells[i].editableTextCell()->textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

void StoreController::setFormulaLabel() {
  char name[Shared::EditableCellTableViewController::k_maxSizeOfColumnName];
  int filledLength = fillColumnName(selectedColumn(), name);
  if (filledLength < Shared::EditableCellTableViewController::k_maxSizeOfColumnName - 1) {
    name[filledLength] = '=';
    name[filledLength+1] = 0;
  }
  static_cast<ContentView *>(view())->formulaInputView()->setBufferText(name);
}

void StoreController::displayFormulaInput() {
  setFormulaLabel();
  m_contentView.displayFormulaInput(true);
}

bool StoreController::textFieldShouldFinishEditing(TextField * textField, Ion::Events::Event event) {
  if (textField == m_contentView.formulaInputView()->textField()) {
    return event == Ion::Events::OK || event == Ion::Events::EXE;
  }
  return EditableCellTableViewController::textFieldShouldFinishEditing(textField, event);
}

bool StoreController::textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) {
  if (textField == m_contentView.formulaInputView()->textField()) {
    // Handle formula input
    Expression expression = Expression::Parse(textField->text(), storeContext());
    if (expression.isUninitialized()) {
      Container::activeApp()->displayWarning(I18n::Message::SyntaxError);
      return false;
    }
    m_contentView.displayFormulaInput(false);
    if (fillColumnWithFormula(expression)) {
      Container::activeApp()->setFirstResponder(&m_contentView);
    }
    return true;
  }
  int series = m_store->seriesAtColumn(selectedColumn());
  bool wasSeriesValid = m_store->seriesIsValid(series);
  bool result = EditableCellTableViewController::textFieldDidFinishEditing(textField, text, event);
  if (wasSeriesValid != m_store->seriesIsValid(series)) {
    // Series changed validity, series' cells have changed color.
    reloadSeriesVisibleCells(series);
  }
  return result;
}

bool StoreController::textFieldDidAbortEditing(TextField * textField) {
  if (textField == m_contentView.formulaInputView()->textField()) {
    m_contentView.displayFormulaInput(false);
    Container::activeApp()->setFirstResponder(&m_contentView);
    return true;
  }
  return EditableCellTableViewController::textFieldDidAbortEditing(textField);
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
    KDColor textColor = m_store->seriesIsValid(m_store->seriesAtColumn(i)) ? KDColorBlack : Palette::GrayDark;
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
  if (event == Ion::Events::Up) {
    selectableTableView()->deselectTable();
    assert(selectedRow() == -1);
    Container::activeApp()->setFirstResponder(tabController());
    return true;
  }
  assert(selectedColumn() >= 0 && selectedColumn() < numberOfColumns());
  int series = m_store->seriesAtColumn(selectedColumn());
  if (event == Ion::Events::Backspace) {
    if (selectedRow() == 0 || selectedRow() > numberOfElementsInColumn(selectedColumn())) {
      return false;
    }
    if (m_store->deleteValueAtIndex(series, m_store->relativeColumnIndex(selectedColumn()), selectedRow()-1)) {
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
  Container::activeApp()->setFirstResponder(&m_contentView);
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
  m_store->set(floatBody, m_store->seriesAtColumn(columnIndex), m_store->relativeColumnIndex(columnIndex), rowIndex-1);
  return true;
}

double StoreController::dataAtLocation(int columnIndex, int rowIndex) {
  return m_store->get(m_store->seriesAtColumn(columnIndex), m_store->relativeColumnIndex(columnIndex), rowIndex-1);
}

int StoreController::numberOfElementsInColumn(int columnIndex) const {
  return m_store->numberOfPairsOfSeries(m_store->seriesAtColumn(columnIndex));
}

void StoreController::reloadSeriesVisibleCells(int series, int relativeColumn) {
  // Reload visible cells of the series and, if not -1, relative column
  for (int i = 0; i < numberOfColumns(); i++) {
    if (m_store->seriesAtColumn(i) == series && (relativeColumn == -1 || relativeColumn == m_store->relativeColumnIndex(i))) {
      selectableTableView()->reloadVisibleCellsAtColumn(i);
    }
  }
}

bool StoreController::privateFillColumnWithFormula(Expression formula, ExpressionNode::isVariableTest isVariable) {
  // Fetch the series used in the formula to compute the size of the filled in series
  constexpr static int k_maxSizeOfStoreSymbols = DoublePairStore::k_lenOfColumnNames + 1;
  char variables[Expression::k_maxNumberOfVariables][k_maxSizeOfStoreSymbols];
  variables[0][0] = 0;
  AppsContainer * appsContainer = AppsContainer::sharedAppsContainer();
  int nbOfVariables = formula.getVariables(appsContainer->globalContext(), isVariable, (char *)variables, k_maxSizeOfStoreSymbols);
  (void) nbOfVariables; // Remove compilation warning of unused variable
  assert(nbOfVariables >= 0);
  int numberOfValuesToCompute = -1;
  int index = 0;
  while (variables[index][0] != 0) {
    int series;
    m_store->isColumnName(variables[index], DoublePairStore::k_lenOfColumnNames, &series);
    assert(series >= 0 && series < DoublePairStore::k_numberOfSeries);
    if (numberOfValuesToCompute == -1) {
      numberOfValuesToCompute = m_store->numberOfPairsOfSeries(series);
    } else {
      numberOfValuesToCompute = std::min(numberOfValuesToCompute, static_cast<int>(m_store->numberOfPairsOfSeries(series)));
    }
    index++;
  }
  if (numberOfValuesToCompute == -1) {
    numberOfValuesToCompute = m_store->numberOfPairsOfSeries(selectedSeries());
  }

  StoreContext * store = storeContext();

  // Make sure no value is undef, else display an error
  for (int j = 0; j < numberOfValuesToCompute; j++) {
    // Set the context
    store->setSeriesPairIndex(j);
    // Compute the new value using the formula
    double evaluation = PoincareHelpers::ApproximateToScalar<double>(formula, store);
    if (std::isnan(evaluation) || std::isinf(evaluation)) {
      Container::activeApp()->displayWarning(I18n::Message::DataNotSuitable);
      return false;
    }
  }

  // Fill in the table with the formula values
  for (int j = 0; j < numberOfValuesToCompute; j++) {
    store->setSeriesPairIndex(j);
    double evaluation = PoincareHelpers::ApproximateToScalar<double>(formula, store);
    setDataAtLocation(evaluation, selectedColumn(), j + 1);
  }
  reloadSeriesVisibleCells(selectedSeries());
  return true;
}

void StoreController::sortSelectedColumn() {
  m_store->sortColumn(selectedSeries(), m_store->relativeColumnIndex(selectedColumn()));
}

void StoreController::switchSelectedColumnHideStatus() {
  int series = selectedSeries();
  bool previousStatus = m_store->seriesIsValid(series);
  if (previousStatus) {
    // Any previously valid series can be hidden
    m_store->hideSeries(series);
  } else {
    // Series may still be invalid, in that case nothing happens
    m_store->updateSeriesValidity(series);
  }
}

}
