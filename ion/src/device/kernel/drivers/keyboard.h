#ifndef ION_DEVICE_KERNEL_KEYBOARD_H
#define ION_DEVICE_KERNEL_KEYBOARD_H

#include <shared/drivers/keyboard.h>

namespace Ion {
namespace Device {
namespace Keyboard {

using namespace Ion::Keyboard;

void init(bool fromBootloader, bool activateInterruption = true);
void shutdown();
void initInterruptions();
void shutdownInterruptions();
void debounce();
void poll();
void resetMemoizedState();
void handleInterruption();

State popState();

inline uint8_t rowForKey(Key key) {
  return (int)key/Config::numberOfColumns;
}
inline uint8_t columnForKey(Key key) {
  return (int)key%Config::numberOfColumns;
}

inline bool columnIsActive(uint8_t column) {
  uint8_t pin = Config::ColumnPins[column];
  return !(Config::ColumnGPIO.IDR()->getBitRange(pin,pin));
}

}
}
}

#endif
