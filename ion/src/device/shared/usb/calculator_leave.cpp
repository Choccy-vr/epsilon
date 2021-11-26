#include "calculator.h"
#include <drivers/reset.h>
#include <ion/reset.h>

namespace Ion {
namespace Device {
namespace USB {

void Calculator::leave(uint32_t leaveAddress) {
  if (leaveAddress == Ion::Device::InternalFlash::Config::StartAddress) {
    Ion::Reset::core();
  } else {
    Reset::jump(leaveAddress);
  }
}

}
}
}
