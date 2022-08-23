#include <ion/usb.h>
#include <ion/events.h>
#include <ion/authentication.h>
#include <drivers/svcall.h>
#include <drivers/board.h>
#include <drivers/usb.h>

namespace Ion {
namespace USB {

bool SVC_ATTRIBUTES isPlugged() {
  SVC_RETURNING_R0(SVC_USB_IS_PLUGGED, bool)
}

}
}

namespace Ion {
namespace Device {
namespace USB {

Events::Event SVC_ATTRIBUTES shouldInterruptDFU() {
  SVC_RETURNING_R0(SVC_USB_SHOULD_INTERRUPT, Events::Event)
}

const char * stringDescriptor() {
  if (Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorks || Authentication::clearanceLevel() == Ion::Authentication::ClearanceLevel::NumWorksAndThirdPartyApps) {
    return Board::isRunningSlotA() ? Config::InterfaceFlashStringDescriptorAuthenticatedSlotA : Config::InterfaceFlashStringDescriptorAuthenticatedSlotB;
  } else {
    return Board::isRunningSlotA() ? Config::InterfaceFlashStringDescriptorThirdPartySlotA : Config::InterfaceFlashStringDescriptorThirdPartySlotB;
  }
}

}
}
}

