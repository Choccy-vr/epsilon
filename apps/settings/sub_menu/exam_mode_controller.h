#ifndef SETTINGS_EXAM_MODE_CONTROLLER_H
#define SETTINGS_EXAM_MODE_CONTROLLER_H

#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <poincare/preferences.h>

#include "generic_sub_controller.h"
#include "selectable_view_with_messages.h"

namespace Settings {

class ExamModeController : public GenericSubController {
 public:
  ExamModeController(Escher::Responder* parentResponder);
  const char* title() override {
    return I18n::translate(I18n::Message::ExamMode);
  }
  Escher::View* view() override { return &m_contentView; }
  bool handleEvent(Ion::Events::Event event) override;
  TELEMETRY_ID("ExamMode");
  void didEnterResponderChain(
      Escher::Responder* previousFirstResponder) override;
  int numberOfRows() const override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;

 private:
  constexpr static int k_numberOfDeactivationMessageLines = 3;
  // -2 for ExamMode::Ruleset::Off and ExamMode::Ruleset::Press-to-test
  constexpr static int k_maxNumberOfCells =
      static_cast<size_t>(Ion::ExamMode::Ruleset::NumberOfRulesets) - 2;

  int initialSelectedRow() const override;
  Poincare::ExamMode examMode();
  Poincare::ExamMode::Ruleset examModeRulesetAtIndex(size_t index) const;
  I18n::Message examModeActivationMessage(size_t index) const;

  SelectableViewWithMessages m_contentView;
  Escher::MenuCell<Escher::MessageTextView> m_cell[k_maxNumberOfCells];
};

}  // namespace Settings

#endif
