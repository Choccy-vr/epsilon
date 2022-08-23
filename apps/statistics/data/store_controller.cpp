#include "store_controller.h"
#include <apps/constant.h>
#include <apps/shared/range_1D.h>
#include <apps/shared/poincare_helpers.h>
#include <assert.h>
#include <float.h>
#include <cmath>
#include <poincare/print.h>

using namespace Poincare;
using namespace Shared;
using namespace Escher;

namespace Statistics {

StoreController::StoreController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, Store * store, ButtonRowController * header, Context * parentContext) :
  Shared::StoreController(parentResponder, inputEventHandlerDelegate, store, header),
  m_store(store),
  m_statisticsContext(m_store, parentContext),
  m_storeParameterController(this, this),
  m_displayCumulatedFrequencies{false, false, false}
{}

bool StoreController::fillColumnWithFormula(Expression formula) {
  return privateFillColumnWithFormula(formula, Symbol::isSeriesSymbol);
}

int StoreController::relativeColumnIndex(int i) const {
  computeRelativeColumnAndSeries(&i);
  return i;
}

void StoreController::sortSelectedColumn() {
  int relativeIndex = relativeColumnIndex(selectedColumn());
  // Also sort the values if the cumulated frequency is selected
  m_store->sortColumn(selectedSeries(), relativeIndex < 2 ? relativeIndex : 0);
}

int StoreController::fillColumnName(int columnIndex, char * buffer) {
  if (isCumulatedFrequencyColumn(columnIndex)) {
    // FC column options doesn't specify the column name.
    buffer[0] = 0;
    return 0;
  }
  int series = seriesAtColumn(columnIndex);
  int isValueColumn = relativeColumnIndex(columnIndex) == 0;
  buffer[0] = isValueColumn ? 'V' : 'N';
  buffer[1] = static_cast<char>('1' + series);
  buffer[2] = 0;
  return 2;
}

int StoreController::numberOfColumns() const {
  int result = Shared::StoreController::numberOfColumns();
  for (int i = 0; i < Store::k_numberOfSeries; i++) {
    result += m_displayCumulatedFrequencies[i];
  }
  return result;
}

void StoreController::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  if (!isCumulatedFrequencyCell(i, j)) {
    return Shared::StoreController::willDisplayCellAtLocation(cell, i, j);
  }
  // Handle hidden cells
  const int numberOfElementsInCol = numberOfElementsInColumn(i);
  Shared::HideableEvenOddBufferTextCell * myCell = static_cast<Shared::HideableEvenOddBufferTextCell *>(cell);
  if (j > numberOfElementsInCol + 1) {
    myCell->setText("");
    myCell->setHide(true);
    return;
  }
  myCell->setHide(false);
  myCell->setEven(j%2 == 0);

  double value = (j == numberOfElementsInCol + 1) ? NAN : dataAtLocation(i, j);
  if (std::isnan(value)) {
    // Special case : last row and NaN
    myCell->setText("");
  } else {
    const int bufferSize = PrintFloat::charSizeForFloatsWithPrecision(Preferences::VeryLargeNumberOfSignificantDigits);
    char buffer[bufferSize];
    Shared::PoincareHelpers::ConvertFloatToTextWithDisplayMode<double>(value, buffer, bufferSize, Preferences::VeryLargeNumberOfSignificantDigits, Preferences::sharedPreferences()->displayMode());
    myCell->setText(buffer);
  }
}

int StoreController::computeRelativeColumnAndSeries(int * i) const {
  int seriesIndex = 0;
  while (*i >= DoublePairStore::k_numberOfColumnsPerSeries + m_displayCumulatedFrequencies[seriesIndex]) {
    *i -= DoublePairStore::k_numberOfColumnsPerSeries + m_displayCumulatedFrequencies[seriesIndex];
    seriesIndex++;
    assert(seriesIndex < Store::k_numberOfSeries);
  }
  return seriesIndex;
}

bool StoreController::setDataAtLocation(double floatBody, int columnIndex, int rowIndex) {
  if (floatBody < -Shared::Range1D::k_lowerMaxFloat || floatBody > Shared::Range1D::k_upperMaxFloat) {
    return false;
  }
  if (relativeColumnIndex(columnIndex) == 1) {
    if (floatBody < 0) {
      return false;
    }
  }
  if (!Shared::StoreController::setDataAtLocation(floatBody, columnIndex, rowIndex)) {
    return false;
  }
  if (m_displayCumulatedFrequencies[seriesAtColumn(columnIndex)]) {
    // Data must be reloaded so that each cumulated frequencies is re-computed
    selectableTableView()->reloadData();
  }
  return true;
}

double StoreController::dataAtLocation(int columnIndex, int rowIndex) {
  if (isCumulatedFrequencyColumn(columnIndex)) {
    int series = seriesAtColumn(columnIndex);
    double value = m_store->get(series, 0, rowIndex - 1);
    return m_store->sumOfValuesBetween(series, -DBL_MAX, value, false);
  }
  return Shared::StoreController::dataAtLocation(columnIndex, rowIndex);
}

void StoreController::setTitleCellText(HighlightCell * cell, int columnIndex) {
  assert(typeAtLocation(columnIndex, 0) == k_titleCellType);
  StoreTitleCell * myTitleCell = static_cast<StoreTitleCell *>(cell);
  if (isCumulatedFrequencyColumn(columnIndex)) {
    myTitleCell->setText(I18n::translate(I18n::Message::CumulatedFrequencyColumnName));
  } else {
    char columnName[Shared::ColumnParameterController::k_maxSizeOfColumnName];
    fillColumnName(columnIndex, columnName);
    char columnTitle[k_columnTitleSize]; // 50 is an ad-hoc value. A title cell can contain max 15 glyphs but the glyph can take more space than 1 byte in memory.
    I18n::Message titleType = relativeColumnIndex(columnIndex) % 2 == 1 ? I18n::Message::Frequencies : I18n::Message::Values;
    Poincare::Print::customPrintf(columnTitle, k_columnTitleSize, I18n::translate(titleType), columnName);
    myTitleCell->setText(columnTitle);
  }
}

void StoreController::clearSelectedColumn() {
  int series = seriesAtColumn(selectedColumn());
  int column = relativeColumnIndex(selectedColumn());
  if (column == 0) {
    m_store->deleteAllPairsOfSeries(series);
  } else {
    m_store->resetColumn(series, column);
  }
}

void StoreController::setClearPopUpContent() {
  int column = relativeColumnIndex(selectedColumn());
  assert(column == 0 || column == 1);
  int series = seriesAtColumn(selectedColumn());
  if (column == 0) {
    char tableName[k_tableNameSize];
    FillSeriesName(series, tableName, true);
    m_confirmPopUpController.setMessageWithPlaceholder(I18n::Message::ClearTableConfirmation, tableName);
  } else {
    char columnNameBuffer[Shared::ColumnParameterController::k_titleBufferSize];
    fillColumnName(selectedColumn(), columnNameBuffer);
    m_confirmPopUpController.setMessageWithPlaceholder(I18n::Message::ResetFreqConfirmation, columnNameBuffer);
  }
}

void StoreController::FillSeriesName(int series, char * buffer, bool withFinalSpace) {
   /* We have to add a space in some cases because we use this table name in the message for
    * deleting the table in Graph and Sequence, but the table name is empty in Sequence.
    */
  char tableIndex = static_cast<char>('1' + series);
  Poincare::Print::customPrintf(buffer, k_tableNameSize, k_tableName, tableIndex, tableIndex);
  if (!withFinalSpace) {
    buffer[5] = 0;
  }
}

}
