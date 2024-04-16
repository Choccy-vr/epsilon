#include <drivers/svcall.h>
#include <drivers/usb.h>
#include <ion/authentication.h>
#include <ion/usb.h>
#include <shared/drivers/board_shared.h>

namespace Ion {
namespace USB {

bool SVC_ATTRIBUTES isPlugged() { SVC_RETURNING_R0(SVC_USB_IS_PLUGGED, bool) }

}  // namespace USB
}  // namespace Ion

namespace Ion {
namespace Device {
namespace USB {

bool SVC_ATTRIBUTES shouldInterruptDFU(Keyboard::State exitKeys,
                                       bool bubbleUpEvents) {
  SVC_RETURNING_R0(SVC_USB_SHOULD_INTERRUPT, bool)
}

const char* stringDescriptor() {
  if (Authentication::clearanceLevel() ==
          Ion::Authentication::ClearanceLevel::NumWorks ||
      Authentication::clearanceLevel() ==
          Ion::Authentication::ClearanceLevel::NumWorksAndThirdPartyApps) {
    return Board::isRunningSlotA()
               ? Config::InterfaceFlashStringDescriptorAuthenticatedSlotA
               : Config::InterfaceFlashStringDescriptorAuthenticatedSlotB;
  } else {
    return Board::isRunningSlotA()
               ? Config::InterfaceFlashStringDescriptorThirdPartySlotA
               : Config::InterfaceFlashStringDescriptorThirdPartySlotB;
  }
}

}  // namespace USB
}  // namespace Device
}  // namespace Ion
