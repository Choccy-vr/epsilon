#include "events_modifier.h"
#include <assert.h>

namespace Ion {
namespace Events {

static ShiftAlphaStatus sShiftAlphaStatus = ShiftAlphaStatus::Default;

ShiftAlphaStatus shiftAlphaStatus() {
  return sShiftAlphaStatus;
}

void removeShift() {
  if (sShiftAlphaStatus == ShiftAlphaStatus::Shift) {
    sShiftAlphaStatus = ShiftAlphaStatus::Default;
  } else if (sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlpha ) {
    sShiftAlphaStatus = ShiftAlphaStatus::Alpha;
  } else if (sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlphaLock) {
    sShiftAlphaStatus = ShiftAlphaStatus::AlphaLock;
  }
}

bool isShiftActive() {
  return sShiftAlphaStatus == ShiftAlphaStatus::Shift || sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlpha || sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlphaLock;
}

bool isAlphaActive() {
  return sShiftAlphaStatus == ShiftAlphaStatus::Alpha || sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlpha || sShiftAlphaStatus == ShiftAlphaStatus::AlphaLock || sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlphaLock;
}

bool isLockActive() {
  return sShiftAlphaStatus == ShiftAlphaStatus::AlphaLock || sShiftAlphaStatus == ShiftAlphaStatus::ShiftAlphaLock;
}

void setShiftAlphaStatus(ShiftAlphaStatus s) {
  sShiftAlphaStatus = s;
}

void updateModifiersFromEvent(Event e) {
  assert(e.isKeyboardEvent());
  switch (sShiftAlphaStatus) {
    case ShiftAlphaStatus::Default:
      if (e == Shift) {
        sShiftAlphaStatus = ShiftAlphaStatus::Shift;
      } else if (e == Alpha) {
        sShiftAlphaStatus = ShiftAlphaStatus::Alpha;
      }
      break;
    case ShiftAlphaStatus::Shift:
      if (e == Shift) {
        sShiftAlphaStatus = ShiftAlphaStatus::Default;
      } else if (e == Alpha) {
        sShiftAlphaStatus = ShiftAlphaStatus::ShiftAlpha;
      } else {
        sShiftAlphaStatus = ShiftAlphaStatus::Default;
      }
      break;
    case ShiftAlphaStatus::Alpha:
      if (e == Shift) {
        sShiftAlphaStatus = ShiftAlphaStatus::ShiftAlpha;
      } else if (e == Alpha) {
        sShiftAlphaStatus = ShiftAlphaStatus::AlphaLock;
      } else {
        sShiftAlphaStatus = ShiftAlphaStatus::Default;
      }
      break;
    case ShiftAlphaStatus::ShiftAlpha:
      if (e == Shift) {
        sShiftAlphaStatus = ShiftAlphaStatus::Alpha;
      } else if (e == Alpha) {
        sShiftAlphaStatus = ShiftAlphaStatus::ShiftAlphaLock;
      } else {
        sShiftAlphaStatus = ShiftAlphaStatus::Default;
      }
      break;
    case ShiftAlphaStatus::AlphaLock:
      if (e == Shift) {
        sShiftAlphaStatus = ShiftAlphaStatus::ShiftAlphaLock;
      } else if (e == Alpha) {
        sShiftAlphaStatus = ShiftAlphaStatus::Default;
      }
      break;
    case ShiftAlphaStatus::ShiftAlphaLock:
      if (e == Shift) {
        sShiftAlphaStatus = ShiftAlphaStatus::AlphaLock;
      } else if (e == Alpha) {
        sShiftAlphaStatus = ShiftAlphaStatus::Default;
      }
      break;
  }
}

// Two factors are available :
static int sLongPressCounter = 0;
static int sRepetitionCounter = 0;

// How long the event has been pressed (Computed value)
int longPressFactor() {
  // The long press factor is increased by 4 every 20 loops in getEvent(2 sec)
  return (sLongPressCounter / 20) * 4 + 1;
}

// How much the event has been repeatedly pressed (Raw value)
int repetitionFactor() {
  return sRepetitionCounter;
}

void resetLongPress() {
  sLongPressCounter = 0;
}

void resetRepetition() {
  sRepetitionCounter = 0;
}

void incrementLongPress() {
  sLongPressCounter++;
}

void incrementRepetition() {
  sRepetitionCounter++;
}

}
}
