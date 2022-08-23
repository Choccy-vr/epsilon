#ifndef PROBABILITY_CONTROLLERS_INPUT_GOODNESS_CONTROLLER_H
#define PROBABILITY_CONTROLLERS_INPUT_GOODNESS_CONTROLLER_H

#include "probability/abstract/categorical_controller.h"
#include "probability/gui/goodness_table_cell.h"

using namespace Escher;

namespace Probability {

class InputGoodnessController : public InputCategoricalController {
public:
  InputGoodnessController(StackViewController * parent, Escher::ViewController * resultsController, GoodnessTest * statistic, InputEventHandlerDelegate * inputEventHandlerDelegate);

  void updateDegreeOfFreedomCell();

  // Responder
  void didBecomeFirstResponder() override;

  // ViewController
  const char * title() override { return I18n::translate(I18n::Message::InputGoodnessControllerTitle); }

  // ListViewDataSource
  virtual Escher::HighlightCell * reusableCell(int index, int type) override;

private:
  constexpr static int k_indexOfDegreeOfFreedom = 1;

  EditableCategoricalTableCell * categoricalTableCell() override { return &m_goodnessTableCell; }
  int indexOfSignificanceCell() const override { return k_indexOfDegreeOfFreedom + 1; }
  int indexOfEditedParameterAtIndex(int index) const override;

  Escher::MessageTableCellWithEditableTextWithMessage m_degreeOfFreedomCell;
  GoodnessTableCell m_goodnessTableCell;
};

}  // namespace Probability

#endif /* PROBABILITY_CONTROLLERS_INPUT_GOODNESS_CONTROLLER_H */
