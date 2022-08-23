#ifndef SETTINGS_MAIN_CONTROLLER_H
#define SETTINGS_MAIN_CONTROLLER_H

#include <apps/shared/button_with_separator.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/pop_up_controller.h>
#include <escher/message_table_cell_with_chevron_and_message.h>
#include <escher/message_table_cell_with_switch.h>
#include "message_table_cell_with_gauge_with_separator.h"
#include "message_tree.h"
#include "sub_menu/about_controller.h"
#include "sub_menu/display_mode_controller.h"
#include "sub_menu/exam_mode_controller.h"
#include "sub_menu/localization_controller.h"
#include "sub_menu/preferences_controller.h"
#include "sub_menu/press_to_test_controller.h"
#include "sub_menu/test_mode_controller.h"

namespace Settings {

extern const MessageTree s_modelAngleChildren[3];
extern const MessageTree s_modelEditionModeChildren[2];
extern const MessageTree s_modelFloatDisplayModeChildren[4];
extern const MessageTree s_modelComplexFormatChildren[3];
extern const MessageTree s_modelFontChildren[2];
extern const MessageTree s_modelExamChildren[3];
extern const MessageTree s_modelTestModeMenu[2];
extern const MessageTree s_modelAboutChildren[3];
extern const MessageTree s_model;

class MainController : public Escher::SelectableListViewController {
public:
  MainController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate);
  bool handleEvent(Ion::Events::Event event) override;
  void pushModel(const Escher::MessageTree * messageTreeModel);
  void didBecomeFirstResponder() override;
  int numberOfRows() const override;
  KDCoordinate nonMemoizedRowHeight(int j) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtIndex(int index) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  void viewWillAppear() override;
  bool hasTestModeCell() const;
  TELEMETRY_ID("");
private:
  I18n::Message messageAtModelIndex(int i) const;
  static const MessageTree * model();
  Escher::StackViewController * stackController() const;
  ViewController * subControllerForCell(I18n::Message cellMessage);
  bool hasExamModeCell() const;
  bool hasPressToTestCell() const;
  int getModelIndex(int index) const;

  // Cell type
  constexpr static int k_defaultCellType = 0;
  constexpr static int k_brightnessCellType = 1;
  constexpr static int k_popUpCellType = 2;
  constexpr static int k_resetCellType = 3;
  // Model index
  constexpr static int k_indexOfExamModeCell = 8;
  // Max number of visible cells
  constexpr static int k_numberOfSimpleChevronCells = ((Ion::Display::Height - Escher::Metric::TitleBarHeight) / Escher::TableCell::k_minimalLargeFontCellHeight) + 2; // Remaining cell can be above and below so we add +2

  Escher::MessageTableCellWithChevronAndMessage m_cells[k_numberOfSimpleChevronCells];
  MessageTableCellWithGaugeWithSeparator m_brightnessCell;
  Escher::MessageTableCellWithSwitch m_popUpCell;
  Shared::ButtonWithSeparator m_resetButton;
  PreferencesController m_preferencesController;
  DisplayModeController m_displayModeController;
  LocalizationController m_localizationController;
  ExamModeController m_examModeController;
  PressToTestController m_pressToTestController;
  TestModeController m_testModeController;
  AboutController m_aboutController;
  Escher::MessagePopUpController m_resetController;
};

}

#endif
