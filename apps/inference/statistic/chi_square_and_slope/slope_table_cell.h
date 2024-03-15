#ifndef INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_SLOPE_TABLE_CELL_H
#define INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_SLOPE_TABLE_CELL_H

#include "inference/models/statistic/slope_t_statistic.h"
#include "inference/statistic/chi_square_and_slope/categorical_table_cell.h"
#include "shared/buffer_function_title_cell.h"
#include "shared/column_helper.h"

namespace Inference {

class InputSlopeController;

class SlopeTableCell : public DoubleColumnTableCell,
                       public Shared::StoreColumnHelper {
 public:
  SlopeTableCell(Escher::Responder *parentResponder, Statistic *statistic,
                 Poincare::Context *parentContext,
                 InputSlopeController *inputSlopeController);

  constexpr static int k_numberOfReusableCells =
      SlopeTStatistic::k_maxNumberOfColumns * k_maxNumberOfReusableRows;

  void fillColumnsNames();

  // StoreColumnHelper
  SlopeTStatistic *store() override {
    return static_cast<SlopeTStatistic *>(tableModel());
  }

 private:
  Escher::HighlightCell *headerCell(int index) override {
    return &m_header[index];
  }

  // ClearColumnHelper
  size_t fillColumnName(int column, char *buffer) override {
    return fillColumnNameFromStore(column, buffer);
  }
  Escher::InputViewController *inputViewController() override;
  void reload() override;
  CategoricalController *categoricalController() override;

  Shared::BufferFunctionTitleCell
      m_header[SlopeTStatistic::k_maxNumberOfColumns];
  InputSlopeController *m_inputSlopeController;
};

}  // namespace Inference

#endif
