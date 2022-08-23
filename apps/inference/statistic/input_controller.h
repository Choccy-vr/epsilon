#ifndef INFERENCE_STATISTIC_INPUT_CONTROLLER_H
#define INFERENCE_STATISTIC_INPUT_CONTROLLER_H

#include <escher/highlight_cell.h>
#include <escher/input_event_handler_delegate.h>

#include "inference/shared/dynamic_cells_data_source.h"
#include "inference/shared/expression_cell_with_editable_text_with_message.h"
#include "inference/statistic/message_table_cell_with_separator.h"
#include <apps/shared/float_parameter_controller.h>
#include "inference/models/statistic/statistic.h"
#include "results_controller.h"

namespace Inference {

class InputController : public Shared::FloatParameterController<double>, public DynamicCellsDataSource<ExpressionCellWithEditableTextWithMessage, k_maxNumberOfExpressionCellsWithEditableTextWithMessage>, public DynamicCellsDataSourceDelegate<ExpressionCellWithEditableTextWithMessage> {
friend class InputSlopeController;
public:
  InputController(Escher::StackViewController * parent,
                  ResultsController * resultsController,
                  Statistic * statistic,
                  Escher::InputEventHandlerDelegate * handler);
  int numberOfRows() const override { return m_statistic->numberOfParameters() + 1 /* button */; }
  const char * title() override {
    InputTitle(this, m_statistic, m_titleBuffer, k_titleBufferSize);
    return m_titleBuffer;
  }
  ViewController::TitlesDisplay titlesDisplay() override;
  int typeAtIndex(int i) override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  void buttonAction() override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;

  void initCell(ExpressionCellWithEditableTextWithMessage, void * cell, int index) override;
  Escher::SelectableTableView * tableView() override { return &m_selectableTableView; }

  constexpr static int k_numberOfReusableCells =
      Ion::Display::Height / Escher::TableCell::k_minimalLargeFontCellHeight + 2;
protected:
  double parameterAtIndex(int i) override { return m_statistic->parameterAtIndex(i); }
  bool isCellEditing(Escher::HighlightCell * cell, int index) override;
  void setTextInCell(Escher::HighlightCell * cell, const char * text, int index) override;

private:
  int reusableParameterCellCount(int type) override { return k_numberOfReusableCells; }
  Escher::HighlightCell * reusableParameterCell(int index, int type) override;
  bool setParameterAtIndex(int parameterIndex, double f) override;
  static void InputTitle(Escher::ViewController * vc, Statistic * statistic, char * titleBuffer, size_t titleBufferSize);

  constexpr static int k_numberOfTitleSignificantDigit = Poincare::Preferences::VeryShortNumberOfSignificantDigits;
  constexpr static int k_titleBufferSize = sizeof("H0:= Ha: α=") + 7 /* μ1-μ2 */ +
                                           3 * (Poincare::PrintFloat::charSizeForFloatsWithPrecision(k_numberOfTitleSignificantDigit) - 1) + 2 /* op */ +
                                           10;
  char m_titleBuffer[k_titleBufferSize];
  Statistic * m_statistic;
  ResultsController * m_resultsController;

  constexpr static int k_significanceCellType = 2;

  MessageTableCellWithSeparator m_significanceCell;
};

}  // namespace Inference

#endif /* INFERENCE_STATISTIC_INPUT_CONTROLLER_H */
