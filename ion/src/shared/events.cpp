#include "events.h"
#include "events_modifier.h"
#include <ion/keyboard.h>
#include <ion/timing.h>
#include <layout_events.h>
#include <limits.h>
#include <algorithm>
#include <string.h>

extern "C" {
#include <assert.h>
}

namespace Ion {
namespace Events {

const char * EventData::text() const {
  if (m_data == nullptr || m_data[0] == 0) {
    return nullptr;
  }
  return m_data;
}

Event::Event(Keyboard::Key key, bool shift, bool alpha, bool lock) {
  // We're mapping a key, shift and alpha to an event
  // This can be a bit more complicated than it seems since we want to fall back:
  // for example, alpha-up is just plain up.
  // Fallback order :
  // shift-X -> X
  // alpha-X -> X
  // shift-alpha-X -> alpha-X -> X
  assert((int)key >= 0 && (int)key < Keyboard::NumberOfKeys);
  m_id = Events::None.m_id;

  int noFallbackOffsets[] = {0};
  int shiftFallbackOffsets[] = {PageSize, 0};
  int alphaFallbackOffsets[] = {2*PageSize, 0};
  int shiftAlphaFallbackOffsets[] = {3*PageSize, 2*PageSize, 0};

  int * fallbackOffsets[] = {noFallbackOffsets, shiftFallbackOffsets, alphaFallbackOffsets, shiftAlphaFallbackOffsets};

  int * fallbackOffset = fallbackOffsets[shift+2*alpha];
  int i=0;
  int offset = 0;
  do {
    offset = fallbackOffset[i++];
    m_id = offset + (int)key;
  } while (offset > 0 && !s_dataForEvent[m_id].isDefined() && m_id < 4*PageSize);

  //If we press percent in alphalock, change to backspace
  if (m_id == static_cast<uint8_t>(Ion::Events::Percent) && lock){
    m_id = static_cast<uint8_t>(Ion::Events::Backspace);
  }
  assert(m_id != Events::None.m_id);
}

const char * Event::defaultText() const {
  /* As the ExternalText event is only available on the simulator, we save a
   * comparison by not handling it on the device. */
  if (m_id >= 4*PageSize) {
    return nullptr;
  }
  return s_dataForEvent[m_id].text();
}

Event sLastEvent = Events::None;
Keyboard::State sLastKeyboardState(0);
Keyboard::State sCurrentKeyboardState(0);
uint64_t sKeysSeenUp = -1;
bool sLastEventShift = false;
bool sLastEventAlpha = false;
bool sEventIsRepeating = false;

Event sharedGetEvent(int * timeout) {
  constexpr int delayBeforeRepeat = 200;
  constexpr int delayBetweenRepeat = 50;
  assert(*timeout > delayBeforeRepeat);
  assert(*timeout > delayBetweenRepeat);

  if (handlePreemption(false)) {
    return None;
  }

  int startTime = Ion::Timing::millis();
  while (true) {
    Event platformEvent = getPlatformEvent();
    if (platformEvent != None) {
      return platformEvent;
    }

    bool lock = isLockActive();
    uint64_t keysSeenTransitionningFromUpToDown;
    Keyboard::State state;
    while ((state = popKeyboardState()) != Keyboard::State(-1)) {
      sCurrentKeyboardState = state;
      keysSeenTransitionningFromUpToDown = state & sKeysSeenUp;
      sKeysSeenUp = ~state;

      if (keysSeenTransitionningFromUpToDown != 0) {
        sEventIsRepeating = false;
        resetLongRepetition();
        /* The key that triggered the event corresponds to the first non-zero bit
         * in "match". This is a rather simple logic operation for the which many
         * processors have an instruction (ARM thumb uses CLZ).
         * Unfortunately there's no way to express this in standard C, so we have
         * to resort to using a builtin function. */
        Keyboard::Key key = (Keyboard::Key)(63-__builtin_clzll(keysSeenTransitionningFromUpToDown));
        didPressNewKey(key);
        bool shift = isShiftActive() || state.keyDown(Keyboard::Key::Shift);
        bool alpha = isAlphaActive() || state.keyDown(Keyboard::Key::Alpha);
        Event event(key, shift, alpha, lock);
        updateModifiersFromEvent(event);
        sLastEvent = event;
        sLastKeyboardState = state;
        return event;
      }
    }

    int maximumDelay = *timeout;
    int delayForRepeat = INT_MAX;
    bool isRepeatableEvent = canRepeatEvent(sLastEvent)
                          && sLastKeyboardState == sCurrentKeyboardState
                          && sLastEventShift == sCurrentKeyboardState.keyDown(Keyboard::Key::Shift)
                          && sLastEventAlpha == (sCurrentKeyboardState.keyDown(Keyboard::Key::Alpha) || lock);
    if (isRepeatableEvent) {
      delayForRepeat = sEventIsRepeating ? delayBetweenRepeat : delayBeforeRepeat;
      maximumDelay = std::min(maximumDelay, delayForRepeat);
    }

    if (waitForInterruptingEvent(maximumDelay, timeout)) {
      continue;
    }

    if (*timeout == 0) {
      resetLongRepetition();
      return None;
    }

    int elapsedTime = Ion::Timing::millis() - startTime;

    /* At this point, we know that keysSeenTransitionningFromUpToDown has
     * always been zero. In other words, no new key has been pressed. */
    if (elapsedTime >= delayForRepeat) {
      assert(isRepeatableEvent);
      sEventIsRepeating = true;
      Ion::Events::incrementRepetitionFactor();
      return sLastEvent;
    }
  }
}

size_t sharedCopyText(uint8_t eventId, char * buffer, size_t bufferSize) {
  Event e(eventId);
  if (e.text()) {
    return strlcpy(buffer, e.text(), bufferSize);
  } else {
    return 0;
  }
}

bool sharedIsDefined(uint8_t eventId) {
  Event e(eventId);
  if (e.isKeyboardEvent()) {
    return s_dataForEvent[static_cast<uint8_t>(e)].isDefined();
  } else {
    return (e == None || e == Termination || e == USBEnumeration || e == USBPlug || e == BatteryCharging || e == ExternalText);
  }
}

}
}
