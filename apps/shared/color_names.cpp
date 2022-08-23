#include "color_names.h"

I18n::Message Shared::ColorNames::NameForColor(KDColor color) {
  for (int i=0; i<Count; i++) {
    if (color == Colors[i]) {
      return Messages[i];
    }
  }
  return I18n::Message::UndefinedType;
}
