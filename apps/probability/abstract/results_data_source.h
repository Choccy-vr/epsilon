#ifndef PROBABILITY_MODELS_RESULTS_DATA_SOURCE_H
#define PROBABILITY_MODELS_RESULTS_DATA_SOURCE_H

#include <apps/shared/button_with_separator.h>
#include <escher/memoized_list_view_data_source.h>

#include <escher/invocation.h>
#include "probability/abstract/dynamic_cells_data_source.h"
#include "probability/gui/expression_cell_with_buffer_with_message.h"
#include "probability/models/statistic/statistic.h"

namespace Probability {

/* A ResultsDataSource is a TableViewDataSource which is meant to represent
 * data provided (and computed) by a Statistic.
 */
class ResultsDataSource : public Escher::MemoizedListViewDataSource, public DynamicCellsDataSource<ExpressionCellWithBufferWithMessage, k_maxNumberOfExpressionCellsWithBufferWithMessage> {
public:
  ResultsDataSource(Escher::Responder * parent, Statistic * statistic, Escher::Invocation invocation, DynamicCellsDataSourceDelegate<ExpressionCellWithBufferWithMessage> * dynamicCellsDataSourceDelegate);
  int numberOfRows() const override;
  KDCoordinate cellWidth() override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int i) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtIndex(int index) override;

  constexpr static int k_numberOfReusableCells = 5;
private:
  constexpr static int k_resultCellType = 0;
  constexpr static int k_buttonCellType = 1;
  Statistic * m_statistic;
  Shared::ButtonWithSeparator m_next;
};

}  // namespace Probability

#endif /* PROBABILITY_MODELS_RESULTS_DATA_SOURCE_H */
