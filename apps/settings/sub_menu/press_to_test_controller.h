#ifndef SETTINGS_PRESS_TO_TEST_CONTROLLER_H
#define SETTINGS_PRESS_TO_TEST_CONTROLLER_H

#include <apps/i18n.h>
#include <apps/shared/button_with_separator.h>
#include <escher/memoized_list_view_data_source.h>
#include <escher/selectable_table_view.h>
#include <escher/view_controller.h>
#include <escher/table_view_with_top_and_bottom_views.h>
#include <poincare/preferences.h>
#include "press_to_test_switch.h"

namespace Settings {

class PressToTestController : public Escher::ViewController, public Escher::MemoizedListViewDataSource, public Escher::SelectableTableViewDataSource {
public:
  PressToTestController(Escher::Responder * parentResponder);
  const char * title() override { return I18n::translate(I18n::Message::PressToTest); }
  bool handleEvent(Ion::Events::Event event) override;
  TELEMETRY_ID("PressToTest");
  void didBecomeFirstResponder() override;
  void didEnterResponderChain(Escher::Responder * previousFirstResponder) override;
  int numberOfRows() const override;
  int typeAtIndex(int index) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  KDCoordinate nonMemoizedRowHeight(int j) override;
  Poincare::Preferences::PressToTestParams getPressToTestParams();
  void viewDidDisappear() override;
  Escher::View * view() override { return &m_view; }
  KDCoordinate cellWidth() override { return m_selectableTableView.columnWidth(0); }
private:
  /* Cell type */
  static constexpr int k_switchCellType = 0;
  static constexpr int k_buttonCellType = 1;
  /* Switch cells count */
  static constexpr int k_numberOfSwitchCells = 7;
  /* TODO : At most 6 switch cells only fit in the screen, but an OrientedLayout
   * currently gives its subviews(such as m_selectableTableView) a frame of
   * minimalSizeForOptimalDisplay size, which contains all cells. */
  static constexpr int k_numberOfReusableSwitchCells = k_numberOfSwitchCells;

  // Switch Cells
  void resetSwitches();
  void setParamAtIndex(int index, bool value);
  bool getParamAtIndex(int index);
  void setMessages();

  Escher::SelectableTableView m_selectableTableView;
  Escher::MessageTextView m_topMessageView;
  Escher::MessageTextView m_bottomMessageView;
  Escher::TableViewWithTopAndBottomViews m_view;
  PressToTestSwitch m_switchCells[k_numberOfReusableSwitchCells];
  Poincare::Preferences::PressToTestParams m_tempPressToTestParams;
  Shared::ButtonWithSeparator m_activateButton;

  static I18n::Message LabelAtIndex(int index);
  static I18n::Message SubLabelAtIndex(int index);
};

}

#endif
