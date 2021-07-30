#ifndef ION_DEVICE_N0110_CONFIG_USB_H
#define ION_DEVICE_N0110_CONFIG_USB_H

#include <regs/regs.h>
#include <drivers/config/board.h>
#include <drivers/config/internal_flash.h>
#include <regs/regs.h>

namespace Ion {
namespace Device {
namespace USB {
namespace Config {

using namespace Regs;

constexpr static AFGPIOPin VbusPin = AFGPIOPin(GPIOA, 9, GPIO::AFR::AlternateFunction::AF10, GPIO::PUPDR::Pull::None, GPIO::OSPEEDR::OutputSpeed::Fast);
constexpr static AFGPIOPin DmPin = AFGPIOPin(GPIOA, 11, GPIO::AFR::AlternateFunction::AF10, GPIO::PUPDR::Pull::None, GPIO::OSPEEDR::OutputSpeed::Fast);
constexpr static AFGPIOPin DpPin = AFGPIOPin(GPIOA, 12, GPIO::AFR::AlternateFunction::AF10, GPIO::PUPDR::Pull::None, GPIO::OSPEEDR::OutputSpeed::Fast);

static_assert(InternalFlash::Config::StartAddress + 2*Board::Config::BootloaderTotalSize == 0x08008000, "Bootloader's memory area should not be accessible via DFU");

constexpr static const char * InterfaceFlashStringDescriptor = "@Flash/0x08008000/02*016Kg/0x90000000/08*004Kg,01*032Kg,63*064Kg,64*064Kg";
constexpr static int BCDDevice = 0x0110;

}
}
}
}

#endif
