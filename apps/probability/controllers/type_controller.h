#ifndef APPS_PROBABILITY_CONTROLLERS_TYPE_CONTROLLER_H
#define APPS_PROBABILITY_CONTROLLERS_TYPE_CONTROLLER_H

#include <escher/highlight_cell.h>
#include <escher/message_table_cell_with_chevron_and_message.h>
#include <escher/message_text_view.h>
#include <escher/selectable_table_view.h>
#include <escher/stack_view_controller.h>
#include <escher/view_controller.h>
#include <ion/events.h>

#include "hypothesis_controller.h"
#include "input_controller.h"
#include "probability/gui/page_controller.h"

using namespace Escher;

namespace Probability {

/* Simple view to include list and description below */
class TypeView : public Escher::SolidColorView {
public:
  TypeView(SelectableTableView * list, MessageTextView * description) :
      Escher::SolidColorView(Escher::Palette::WallScreen),
      m_list(list),
      m_description(description) {}
  int numberOfSubviews() const override { return 2; }
  Escher::View * subviewAtIndex(int i) override;
  void layoutSubviews(bool force = false) override;

private:
  SelectableTableView * m_list;
  MessageTextView * m_description;
};

class TypeController : public SelectableListViewPage {
public:
  TypeController(StackViewController * parent,
                 HypothesisController * hypothesisController,
                 InputController * intervalInputController,
                 Data::Test * globalTest,
                 Data::TestType * globalTestType,
                 Statistic * statistic);
  View * view() override { return &m_contentView; }
  const char * title() override;
  ViewController::TitlesDisplay titlesDisplay() override {
    return ViewController::TitlesDisplay::DisplayLastTitle;
  }
  void didBecomeFirstResponder() override;
  // ListViewDataSource
  int numberOfRows() const override;
  HighlightCell * reusableCell(int i, int type) override { return &m_cells[i]; }
  bool handleEvent(Ion::Events::Event event) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int i) override;

  constexpr static int k_indexOfTTest = 0;
  constexpr static int k_indexOfPooledTest = 1;
  constexpr static int k_indexOfZTest = 2;
  constexpr static int k_indexOfDescription = 3;

private:
  int indexFromListIndex(int i) const;
  int listIndexFromIndex(int i) const;
  I18n::Message messageForTest(Data::SubApp subapp, Data::Test t) const;

  constexpr static int k_numberOfRows = 3;

  HypothesisController * m_hypothesisController;
  InputController * m_inputController;

  MessageTableCellWithChevronAndMessage m_cells[k_numberOfRows];
  TypeView m_contentView;
  MessageTextView m_description;

  char m_titleBuffer[30];

  Data::TestType * m_globalTestType;
  Statistic * m_statistic;
};

}  // namespace Probability

#endif /* APPS_PROBABILITY_CONTROLLERS_TYPE_CONTROLLER_H */
