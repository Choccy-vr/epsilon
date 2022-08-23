#include "exam_pop_up_controller.h"
#include "apps_container.h"
#include "exam_mode_configuration.h"
#include <assert.h>
#include <apps/i18n.h>

using namespace Escher;

ExamPopUpController::ExamPopUpController(ExamPopUpControllerDelegate * delegate) :
  Shared::MessagePopUpController(
    Invocation(
      [](void * context, void * sender) {
        ExamPopUpController * controller = (ExamPopUpController *)context;
        Poincare::Preferences::ExamMode mode = controller->targetExamMode();
        Poincare::Preferences::ExamMode previousMode = Poincare::Preferences::sharedPreferences()->examMode();
        assert(mode != Poincare::Preferences::ExamMode::Unknown);
        assert(mode == Poincare::Preferences::ExamMode::PressToTest || controller->targetPressToTestParams().m_value == 0);
        Poincare::Preferences::sharedPreferences()->setExamMode(mode);
        Poincare::Preferences::sharedPreferences()->setPressToTestParams(controller->targetPressToTestParams());
        AppsContainer * container = AppsContainer::sharedAppsContainer();
        if (mode == Poincare::Preferences::ExamMode::Off) {
          if (previousMode == Poincare::Preferences::ExamMode::PressToTest) {
            Ion::Reset::core();
            return true;
          }
          Ion::LED::setColor(KDColorBlack);
          Ion::LED::updateColorWithPlugAndCharge();
        } else {
          container->activateExamMode(mode);
        }
        container->refreshPreferences();
        Container::activeApp()->dismissModalViewController();
        container->switchToBuiltinApp(container->homeAppSnapshot());
        /* Warning : By unplugging before confirmation, the examMode may be
         * deactivated within menus that depend on the examMode status (such as
         * PressToTest settings menu after having changed the country). */
        return true;
      }, this),
      I18n::Message::Default
  ),
  m_targetExamMode(Poincare::Preferences::ExamMode::Unknown),
  m_delegate(delegate)
{
}

void ExamPopUpController::setTargetExamMode(Poincare::Preferences::ExamMode mode) {
  m_targetExamMode = mode;
  setContentMessage(ExamModeConfiguration::examModeActivationWarningMessage(mode));
}

void ExamPopUpController::viewDidDisappear() {
  if (m_targetExamMode == Poincare::Preferences::ExamMode::Off) {
    m_delegate->examDeactivatingPopUpIsDismissed();
  }
}

bool ExamPopUpController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::USBEnumeration) {
    Container::activeApp()->dismissModalViewController();
    return false;
  }
  return MessagePopUpController::handleEvent(event);
}
