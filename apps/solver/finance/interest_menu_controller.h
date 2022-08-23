#ifndef SOLVER_SIMPLE_INTEREST_MENU_CONTROLLER_H
#define SOLVER_SIMPLE_INTEREST_MENU_CONTROLLER_H

#include <escher/message_table_cell_with_chevron_and_message.h>
#include <escher/message_text_view.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/stack_view_controller.h>
#include <escher/table_view_with_top_and_bottom_views.h>
#include <escher/view_controller.h>
#include <ion/events.h>
#include "data.h"
#include "interest_controller.h"

namespace Solver {

constexpr int k_numberOfInterestCells = InterestData::k_maxNumberOfUnknowns;

class InterestMenuController : public Escher::SelectableCellListPage<Escher::MessageTableCellWithChevronAndMessage, k_numberOfInterestCells, Escher::RegularListViewDataSource> {
public:
  InterestMenuController(Escher::StackViewController * parentResponder, InterestController * interestController, InterestData * data);
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event e) override;
  const char * title() override { return I18n::translate(m_data->menuTitle()); }
  ViewController::TitlesDisplay titlesDisplay() override { return ViewController::TitlesDisplay::DisplayLastTitle; }
  Escher::View * view() override { return &m_contentView; }
  int numberOfRows() const override { return m_data->numberOfUnknowns(); }
private:
  uint8_t paramaterAtIndex(int index) const;

  Escher::MessageTextView m_messageView;
  Escher::TableViewWithTopAndBottomViews m_contentView;
  InterestController * m_interestController;
  InterestData * m_data;
};

}  // namespace Solver

#endif /* SOLVER_SIMPLE_INTEREST_MENU_CONTROLLER_H */
