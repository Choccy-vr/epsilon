#include "exam_pop_up_controller.h"

#include <apps/i18n.h>
#include <assert.h>
#include <ion/usb.h>

#include <array>

#include "apps_container.h"

using namespace Escher;
using namespace Poincare;

ExamPopUpController::ExamPopUpController()
    : Shared::MessagePopUpController(
          Invocation::Builder<ExamPopUpController>(
              [](ExamPopUpController* controller, void* sender) {
                return controller->handleButton();
              },
              this),
          I18n::Message::Default) {}

void ExamPopUpController::setTargetExamMode(ExamMode mode) {
  m_targetExamMode = mode;
  setContentMessage(activationWarningMessage());
}

void ExamPopUpController::viewDidDisappear() {
  if (!m_targetExamMode.isActive()) {
    Ion::USB::clearEnumerationInterrupt();
  }
}

bool ExamPopUpController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::USBEnumeration) {
    App::app()->modalViewController()->dismissModal();
    return false;
  }
  return MessagePopUpController::handleEvent(event);
}

I18n::Message ExamPopUpController::activationWarningMessage() const {
  constexpr size_t numberOfModes =
      static_cast<size_t>(ExamMode::Ruleset::NumberOfRulesets);
  constexpr size_t messagesPerMode = 2;
  constexpr I18n::Message messages[] = {
      // Off
      I18n::Message::ExitExamMode,
      I18n::Message::ExitPressToTestExamMode,
      // Standard
      I18n::Message::ActiveExamModeMessage,
      I18n::Message::ActiveExamModeWithResetMessage,
      // Dutch
      I18n::Message::ActiveDutchExamModeMessage,
      I18n::Message::ActiveDutchExamModeWithResetMessage,
      // IBTest
      I18n::Message::ActiveUSAExamModeMessage,
      I18n::Message::ActiveTxPaScIbExamModeWithResetMessage,
      // PressToTest
      I18n::Message::ActivePressToTestModeMessage,
      I18n::Message::ActivePressToTestWithResetMessage,
      // Portuguese
      I18n::Message::ActivePortugueseExamModeMessage,
      I18n::Message::ActiveExamModeWithResetMessage,
      // English
      I18n::Message::ActiveEnglishExamModeMessage,
      I18n::Message::ActiveEnglishExamModeWithResetMessage,
      // STAAR
      I18n::Message::ActiveUSAExamModeMessage,
      I18n::Message::ActiveTxPaScIbExamModeWithResetMessage,
      // Pennsylvania
      I18n::Message::ActiveUSAExamModeMessage,
      I18n::Message::ActiveTxPaScIbExamModeWithResetMessage,
      // SouthCarolina
      I18n::Message::ActiveUSAExamModeMessage,
      I18n::Message::ActiveTxPaScIbExamModeWithResetMessage,
      // NorthCarolina
      I18n::Message::ActiveUSAExamModeMessage,
      I18n::Message::ActiveTxPaScIbExamModeWithResetMessage,
  };
  static_assert(std::size(messages) == numberOfModes * messagesPerMode,
                "messages size is invalid");
  ExamMode::Ruleset rules = m_targetExamMode.ruleset();
  size_t index = static_cast<size_t>(rules) * messagesPerMode;
  index += (rules == ExamMode::Ruleset::Off &&
            Preferences::SharedPreferences()->examMode().ruleset() ==
                ExamMode::Ruleset::PressToTest) ||
           Ion::Authentication::clearanceLevel() !=
               Ion::Authentication::ClearanceLevel::NumWorks;
  assert(index < numberOfModes * messagesPerMode);
  return messages[index];
}

bool ExamPopUpController::handleButton() const {
  /* Warning : By unplugging before confirmation, the examMode may then be
   *           deactivated while unplugged. */
  AppsContainer::sharedAppsContainer()->setExamMode(m_targetExamMode);
  return true;
}
