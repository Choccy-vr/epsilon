#ifndef INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_GOODNESS_TABLE_CELL_H
#define INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_GOODNESS_TABLE_CELL_H

#include <escher/even_odd_editable_text_cell.h>
#include <escher/even_odd_message_text_cell.h>
#include <escher/metric.h>

#include "inference/models/statistic/goodness_test.h"
#include "inference/shared/dynamic_cells_data_source.h"
#include "inference/statistic/categorical_table_cell.h"
#include "inference/statistic/categorical_table_view_data_source.h"

namespace Inference {

class InputGoodnessController;

class InputGoodnessTableCell : public DoubleColumnTableCell {
 public:
  InputGoodnessTableCell(Escher::Responder *parentResponder, GoodnessTest *test,
                         InputGoodnessController *inputGoodnessController,
                         Escher::ScrollViewDelegate *scrollViewDelegate);

  // Responder
  bool textFieldDidFinishEditing(Escher::AbstractTextField *textField,
                                 Ion::Events::Event event) override;

  bool recomputeDimensionsAndReload(bool forceReloadTable = false,
                                    bool forceReloadPage = false) override;

 private:
  static_assert(GoodnessTest::k_maxNumberOfColumns <=
                    DoubleColumnTableCell::k_maxNumberOfColumns,
                "GoodnessTest is not adjusted to the DoubleColumnTableCell");
  constexpr static I18n::Message
      k_columnHeaders[GoodnessTest::k_maxNumberOfColumns] = {
          I18n::Message::Observed, I18n::Message::Expected};

  // ClearColumnHelper
  size_t fillColumnName(int column, char *buffer) override;

  Escher::HighlightCell *headerCell(int index) override {
    return &m_header[index];
  }
  GoodnessTest *statistic() { return static_cast<GoodnessTest *>(m_statistic); }
  CategoricalController *categoricalController() override;

  Escher::EvenOddMessageTextCell m_header[k_maxNumberOfColumns];
  InputGoodnessController *m_inputGoodnessController;
};

}  // namespace Inference

#endif
