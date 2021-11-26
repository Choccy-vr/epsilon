#ifndef ION_DEVICE_FLASHER_CONFIG_USB_H
#define ION_DEVICE_FLASHER_CONFIG_USB_H

#include <drivers/config/usb_pins.h>

namespace Ion {
namespace Device {
namespace USB {
namespace Config {

constexpr static const char * InterfaceFlashStringDescriptor = "@Flash/0x08000000/04*016Kg/0x90000000/08*004Kg,01*032Kg,63*064Kg,64*064Kg";
constexpr static const char * InterfaceSRAMStringDescriptor = "@SRAM/0x20000000/01*192Ke";
constexpr static int BCDDevice = 0x0110;

}
}
}
}

#endif
