#ifndef INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_INPUT_HOMOGENEITY_CONTROLLER_H
#define INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_INPUT_HOMOGENEITY_CONTROLLER_H

#include "inference/statistic/categorical_controller.h"
#include "inference/statistic/chi_square_and_slope/input_homogeneity_table_cell.h"

namespace Inference {

class InputHomogeneityController : public InputCategoricalController {
 public:
  InputHomogeneityController(
      Escher::StackViewController* parent,
      Escher::ViewController* homogeneityResultsController,
      HomogeneityTest* statistic);

  // ViewController
  const char* title() override {
    return I18n::translate(I18n::Message::InputHomogeneityControllerTitle);
  }

 private:
  int indexOfSignificanceCell() const override {
    return indexOfTableCell() + 1;
  }
  InputCategoricalTableCell* categoricalTableCell() override {
    return &m_inputHomogeneityTable;
  }
  void createDynamicCells() override;

  InputHomogeneityTableCell m_inputHomogeneityTable;
};

}  // namespace Inference

#endif
