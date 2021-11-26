#ifndef ION_SHARED_EVENTS_H
#define ION_SHARED_EVENTS_H

#include <ion/events.h>
#include <ion/keyboard.h>

namespace Ion {
namespace Events {

Event sharedGetEvent(int * timeout);
size_t sharedCopyText(uint8_t eventId, char * buffer, size_t bufferSize);
bool sharedIsDefined(uint8_t eventId);

/* Platform specific functions */
bool handlePreemption(bool stalling);
Event getPlatformEvent();
void didPressNewKey(Keyboard::Key key);
Keyboard::State popKeyboardState();
bool waitForInterruptingEvent(int maximumDelay, int * timeout);

}
}

#endif
