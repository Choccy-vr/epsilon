#include "pop_up_controller.h"
#include "../apps_container.h"

namespace HardwareTest {

PopUpController::PopUpController() :
  Escher::PopUpController(
    4,
    Escher::Invocation(
      [](void * context, void * sender) {
        AppsContainer * appsContainer = AppsContainer::sharedAppsContainer();
        appsContainer->switchTo(appsContainer->hardwareTestAppSnapshot());
        return true;
      }, this)
  )
{
  m_contentView.setMessage(0, I18n::Message::HardwareTestLaunch1);
  m_contentView.setMessage(1, I18n::Message::HardwareTestLaunch2);
  m_contentView.setMessage(2, I18n::Message::HardwareTestLaunch3);
  m_contentView.setMessage(3, I18n::Message::HardwareTestLaunch4);
}

}
