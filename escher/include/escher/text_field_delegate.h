#ifndef ESCHER_TEXT_FIELD_DELEGATE_H
#define ESCHER_TEXT_FIELD_DELEGATE_H

#include <ion/events.h>

namespace Escher {
class TextField;

class TextFieldDelegate {
public:
  virtual bool textFieldShouldFinishEditing(TextField * textField, Ion::Events::Event event) = 0;
  virtual void textFieldDidStartEditing(TextField * textField) {}
  virtual bool textFieldDidReceiveEvent(TextField * textField, Ion::Events::Event event) = 0;
  virtual bool textFieldDidFinishEditing(TextField * textField, const char * text, Ion::Events::Event event) { return false; }
  virtual bool textFieldDidAbortEditing(TextField * textField) { return false; }
  virtual bool textFieldDidHandleEvent(TextField * textField, bool returnValue, bool textSizeDidChange) { return returnValue; }
};

}
#endif
