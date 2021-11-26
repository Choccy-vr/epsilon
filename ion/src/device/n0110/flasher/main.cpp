#include "main.h"
#include "../shared/usb/calculator.h"
#include <ion.h>
#include <drivers/display.h>

void flasher_main() {
  // Initialize Flasher display
  Ion::Device::Display::pushRectUniform(KDRect(0,0,Ion::Display::Width,Ion::Display::Height), KDColorYellow);
  waitInDFU();
}

void waitInDFU() {
  while (true) {
    Ion::USB::enable();
    while (!Ion::USB::isEnumerated()) {
    }
    Ion::USB::DFU();
  }
}
