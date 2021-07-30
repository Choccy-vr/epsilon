#ifndef REGRESSION_REGRESSION_CONTROLLER_H
#define REGRESSION_REGRESSION_CONTROLLER_H

#include <escher/message_table_cell_with_expression.h>
#include <escher/selectable_list_view_controller.h>
#include "store.h"
#include <apps/i18n.h>

namespace Regression {

class RegressionController : public Escher::SelectableListViewController {
public:
  RegressionController(Escher::Responder * parentResponder, Store * store);
  void setSeries(int series) { m_series = series; }
  // ViewController
  const char * title() override;
  TELEMETRY_ID("Regression");

  // Responder
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;

  // MemoizedListViewDataSource
  KDCoordinate nonMemoizedRowHeight(int j) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override { return k_numberOfCells; }
  int numberOfRows() const override { return k_numberOfRows; }
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
private:
  constexpr static int k_numberOfRows = 10;
  constexpr static int k_numberOfCells = ((Ion::Display::Height - Escher::Metric::TitleBarHeight - Escher::Metric::TabHeight - 2*Escher::Metric::StackTitleHeight) / Escher::TableCell::k_minimalLargeFontCellHeight) + 2; // Remaining cell can be above and below so we add +2
  Escher::MessageTableCellWithExpression m_regressionCells[k_numberOfCells];
  Store * m_store;
  int m_series;
};

}

#endif
