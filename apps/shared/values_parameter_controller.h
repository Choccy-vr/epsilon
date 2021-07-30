#ifndef SHARED_VALUES_PARAM_CONTROLLER_H
#define SHARED_VALUES_PARAM_CONTROLLER_H

#include <escher/message_table_cell.h>
#include <escher/message_table_cell_with_chevron.h>
#include <escher/selectable_list_view_controller.h>
#include "interval_parameter_controller.h"

namespace Shared {
class ValuesParameterController : public Escher::SelectableListViewController {
public:
  ValuesParameterController(Escher::Responder * parentResponder);
  const char * title() override;
  void setPageTitle(I18n::Message pageTitle) { m_pageTitle = pageTitle; }
  bool handleEvent(Ion::Events::Event event) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  void didBecomeFirstResponder() override;
  int numberOfRows() const override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
private:
#if COPY_COLUMN
  constexpr static int k_totalNumberOfCell = 3;
  Escher::MessageTableCellWithChevron m_copyColumn;
#else
  constexpr static int k_totalNumberOfCell = 2;
#endif
  I18n::Message m_pageTitle;
  Escher::MessageTableCell m_deleteColumn;
  Escher::MessageTableCellWithChevron m_setInterval;
};

}

#endif
