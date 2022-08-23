#ifndef GRAPH_DERIVATIVE_PARAM_CONTROLLER_H
#define GRAPH_DERIVATIVE_PARAM_CONTROLLER_H

#include <escher/message_table_cell.h>
#include <escher/message_table_cell_with_chevron.h>
#include <escher/message_table_cell_with_editable_text.h>
#include <escher/selectable_list_view_controller.h>
#include "../continuous_function_store.h"
#include "../../shared/column_parameter_controller.h"

namespace Graph {

class ValuesController;

class DerivativeParameterController : public Shared::ColumnParameterController {
public:
  DerivativeParameterController(ValuesController * valuesController);

  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override;
  int reusableCellCount() const override { return k_totalNumberOfCell; }
  Escher::HighlightCell * reusableCell(int index) override;
  void setRecord(Ion::Storage::Record record) {
    m_record = record;
  }
private:
  ContinuousFunctionStore * functionStore();
  constexpr static int k_totalNumberOfCell = 1;
  Shared::EditableCellTableViewController * editableCellTableViewController() override;
  Escher::MessageTableCell m_hideColumn;
  Ion::Storage::Record m_record;
  ValuesController * m_valuesController;
};

}

#endif
