#ifndef REGRESSION_CALCULATION_CONTROLLER_H
#define REGRESSION_CALCULATION_CONTROLLER_H

#include <escher/even_odd_expression_cell.h>
#include <escher/even_odd_message_text_cell.h>
#include <poincare/preferences.h>
#include "store.h"
#include "column_title_cell.h"
#include "even_odd_double_buffer_text_cell_with_separator.h"
#include <apps/shared/hideable_even_odd_cell.h>
#include <apps/shared/separator_even_odd_buffer_text_cell.h>
#include <apps/shared/store_cell.h>
#include <apps/shared/double_pair_table_controller.h>

namespace Regression {

constexpr static KDCoordinate maxCoordinate(KDCoordinate a, KDCoordinate b) { return a > b ? a : b; }

class CalculationController : public Shared::DoublePairTableController, public Escher::SelectableTableViewDelegate {

public:
  CalculationController(Escher::Responder * parentResponder, Escher::ButtonRowController * header, Store * store);

  // View Controller
  TELEMETRY_ID("Calculation");

  // SelectableTableViewDelegate
  void tableViewDidChangeSelection(Escher::SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) override;

  // TableViewDataSource
  int numberOfRows() const override;
  int numberOfColumns() const override;
  void willDisplayCellAtLocation(Escher::HighlightCell * cell, int i, int j) override;
  KDCoordinate columnWidth(int i) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override;
private:
  constexpr static int k_totalNumberOfDoubleBufferRows = 5;
  constexpr static int k_numberOfDoubleCalculationCells = Store::k_numberOfSeries * k_totalNumberOfDoubleBufferRows;
  constexpr static int k_numberOfCalculationCells = Store::k_numberOfSeries * k_maxNumberOfDisplayableRows;
  constexpr static int k_standardCalculationTitleCellType = 0;
  constexpr static int k_columnTitleCellType = 1;
  constexpr static int k_doubleBufferCalculationCellType = 2;
  constexpr static int k_standardCalculationCellType = 3;
  static constexpr int k_hideableCellType = 4;
  static constexpr int k_regressionCellIndex = 9;

  static constexpr KDCoordinate k_titleCalculationCellWidth = Ion::Display::Width/2 - Escher::Metric::CommonRightMargin/2 - Escher::Metric::CommonLeftMargin/2;
  /* Separator and margins from EvenOddCell::layoutSubviews (and derived classes
   * implementations) must be accounted for here.
   * TODO: change 7 for KDFont::SmallFont->glyphSize().width()
   * Calculation width should at least be able to hold to numbers with
   * VeryLargeNumberOfSignificantDigits and contains two even odd cells. */
  static constexpr KDCoordinate k_minCalculationCellWidth = 2*(7*Poincare::PrintFloat::glyphLengthForFloatWithPrecision(Poincare::Preferences::VeryLargeNumberOfSignificantDigits)+2*Escher::EvenOddCell::k_horizontalMargin)+Escher::EvenOddCell::k_separatorWidth;
  // To hold _y=a·x^3+b·x^2+c·x+d_
  static constexpr KDCoordinate k_cubicCalculationCellWidth = maxCoordinate(7*21+2*Escher::EvenOddCell::k_horizontalMargin+Escher::EvenOddCell::k_separatorWidth, k_minCalculationCellWidth);
  // To hold _y=a·x^4+b·x^3+c·x^2+d·x+e_
  static constexpr KDCoordinate k_quarticCalculationCellWidth = maxCoordinate(7*27+2*Escher::EvenOddCell::k_horizontalMargin+Escher::EvenOddCell::k_separatorWidth, k_minCalculationCellWidth);

  Shared::DoublePairStore * store() const override { return m_store; }
  bool hasLinearRegression() const;
  int maxNumberOfCoefficients() const;

  Escher::EvenOddMessageTextCell m_titleCells[k_maxNumberOfDisplayableRows];
  ColumnTitleCell m_columnTitleCells[Store::k_numberOfSeries];
  EvenOddDoubleBufferTextCellWithSeparator m_doubleCalculationCells[k_numberOfDoubleCalculationCells];
  Shared::SeparatorEvenOddBufferTextCell m_calculationCells[k_numberOfCalculationCells];
  Shared::HideableEvenOddCell m_hideableCell;
  Store * m_store;
};

}

#endif
