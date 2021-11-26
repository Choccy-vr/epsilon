#ifndef GRAPH_FUNCTION_PARAM_CONTROLLER_H
#define GRAPH_FUNCTION_PARAM_CONTROLLER_H

#include <escher/message_table_cell_with_switch.h>
#include "../../shared/expiring_pointer.h"
#include "../new_function.h"
#include "../../shared/values_function_parameter_controller.h"

namespace Graph {

class ValuesController;

class FunctionParameterController : public Shared::ValuesFunctionParameterController {
public:
  FunctionParameterController(ValuesController * valuesController);
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  void viewWillAppear() override;
private:
  Shared::ExpiringPointer<NewFunction> function();
#if COPY_COLUMN
  constexpr static int k_totalNumberOfCell = 2;
#else
  constexpr static int k_totalNumberOfCell = 1;
#endif
  Escher::MessageTableCellWithSwitch m_displayDerivativeColumn;
  ValuesController * m_valuesController;
  // Index of the column corresponding to the function in the values controller
  int m_selectedFunctionColumn;
};

}

#endif
