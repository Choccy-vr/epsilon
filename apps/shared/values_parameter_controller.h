#ifndef SHARED_VALUES_PARAM_CONTROLLER_H
#define SHARED_VALUES_PARAM_CONTROLLER_H

#include <escher/chevron_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/selectable_list_view_controller.h>

#include "column_parameter_controller.h"
#include "interval_parameter_controller.h"

namespace Shared {

class ValuesController;

class ValuesParameterController : public ColumnParameterController {
 public:
  ValuesParameterController(Escher::Responder* parentResponder,
                            ValuesController* valuesController);
  int numberOfRows() const override { return k_totalNumberOfCell; }
  bool handleEvent(Ion::Events::Event event) override;
  Escher::HighlightCell* cell(int row) override;
  void initializeColumnParameters() override;

 private:
  ClearColumnHelper* clearColumnHelper() override;
  constexpr static int k_totalNumberOfCell = 2;

  Escher::MenuCell<Escher::MessageTextView> m_clearColumn;
  Escher::MenuCell<Escher::MessageTextView, Escher::EmptyCellWidget,
                   Escher::ChevronView>
      m_setInterval;

  ValuesController* m_valuesController;
};

}  // namespace Shared

#endif
