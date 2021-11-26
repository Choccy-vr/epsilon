#include <drivers/config/usb.h>
#include <kernel/drivers/keyboard.h>
#include <ion/usb.h>

namespace Ion {
namespace USB {

bool isPlugged() {
  return Device::USB::Config::VbusPin.group().IDR()->get(Device::USB::Config::VbusPin.pin());

}

}
}

namespace Ion {
namespace Device {
namespace USB {

bool shouldInterruptDFU() {
  return Keyboard::columnIsActive(Keyboard::columnForKey(Ion::Keyboard::Key::Back));
}

}
}
}
