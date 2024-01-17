#ifndef SETTINGS_MAIN_CONTROLLER_H
#define SETTINGS_MAIN_CONTROLLER_H

#include <apps/shared/pop_up_controller.h>
#include <escher/button_cell.h>
#include <escher/chevron_view.h>
#include <escher/gauge_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/switch_view.h>

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
extern const MessageTree s_modelTestModeMenu[2];
extern const MessageTree
    s_modelAboutChildren[AboutController::k_totalNumberOfCell];
extern const MessageTree s_model;

class MainController : public Escher::SelectableListViewController<
                           Escher::StandardMemoizedListViewDataSource> {
 public:
  MainController(Escher::Responder* parentResponder);
  bool handleEvent(Ion::Events::Event event) override;
  void pushModel(const Escher::MessageTree* messageTreeModel);
  int numberOfRows() const override;
  KDCoordinate nonMemoizedRowHeight(int row) override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  int typeAtRow(int row) const override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  KDCoordinate separatorBeforeRow(int row) override;
  void viewWillAppear() override;
  bool hasTestModeCell() const;
  TELEMETRY_ID("");

 private:
  I18n::Message messageAtModelIndex(int i) const;
  static const MessageTree* model();
  Escher::StackViewController* stackController() const;
  ViewController* subControllerForCell(I18n::Message cellMessage);
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
  constexpr static int k_numberOfSimpleChevronCells =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          Escher::AbstractMenuCell::k_minimalLargeFontCellHeight);

  using SubMenuCell =
      Escher::MenuCell<Escher::MessageTextView, Escher::MessageTextView,
                       Escher::ChevronView>;

  SubMenuCell m_cells[k_numberOfSimpleChevronCells];
  Escher::MenuCell<Escher::MessageTextView, Escher::EmptyCellWidget,
                   Escher::GaugeView>
      m_brightnessCell;
  Escher::MenuCell<Escher::MessageTextView, Escher::EmptyCellWidget,
                   Escher::SwitchView>
      m_popUpCell;
  Escher::ButtonCell m_resetButton;
  PreferencesController m_preferencesController;
  DisplayModeController m_displayModeController;
  LocalizationController m_localizationController;
  ExamModeController m_examModeController;
  PressToTestController m_pressToTestController;
  TestModeController m_testModeController;
  AboutController m_aboutController;
  Shared::MessagePopUpController m_resetController;
};

}  // namespace Settings

#endif
