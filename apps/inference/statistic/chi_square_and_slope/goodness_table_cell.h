#ifndef PROBABILITY_GUI_GOODNESS_TABLE_CELL_H
#define PROBABILITY_GUI_GOODNESS_TABLE_CELL_H

#include <escher/even_odd_editable_text_cell.h>
#include <escher/even_odd_message_text_cell.h>
#include <escher/metric.h>
#include "inference/statistic/chi_square_and_slope/categorical_table_cell.h"
#include "inference/statistic/chi_square_and_slope/categorical_table_view_data_source.h"
#include "inference/shared/dynamic_cells_data_source.h"
#include "inference/models/statistic/goodness_test.h"

namespace Inference {

class InputGoodnessController;

class GoodnessTableCell : public DoubleColumnTableCell {
public:
  GoodnessTableCell(Escher::Responder * parentResponder, DynamicSizeTableViewDataSourceDelegate * dynamicSizeTableViewDataSourceDelegate, Escher::SelectableTableViewDelegate * selectableTableViewDelegate, GoodnessTest * test, InputGoodnessController * inputGoodnessController);

  // Responder
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;

  // DynamicSizeTableViewDataSource
  bool recomputeDimensions() override;

private:
  static_assert(GoodnessTest::k_maxNumberOfColumns == DoubleColumnTableCell::k_maxNumberOfColumns, "GoodnessTest is not adjusted to the DoubleColumnTableCell");
  static constexpr I18n::Message k_columnHeaders[GoodnessTest::k_maxNumberOfColumns] = {I18n::Message::Observed, I18n::Message::Expected};

  // ClearColumnHelper
  int fillColumnName(int column, char * buffer) override;

  GoodnessTest * statistic() { return static_cast<GoodnessTest *>(m_tableModel); }

  Escher::HighlightCell * headerCell(int index) override { return &m_header[index]; }

  Escher::EvenOddMessageTextCell m_header[k_maxNumberOfColumns];
  InputGoodnessController * m_inputGoodnessController;
};

}  // namespace Inference

#endif /* PROBABILITY_GUI_CATEGORICAL_TABLE_CELL_H */
