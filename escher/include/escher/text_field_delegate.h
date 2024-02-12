#ifndef ESCHER_TEXT_FIELD_DELEGATE_H
#define ESCHER_TEXT_FIELD_DELEGATE_H

#include <ion/events.h>

namespace Escher {
class AbstractTextField;

class TextFieldDelegate {
 public:
  virtual bool textFieldShouldFinishEditing(AbstractTextField* textField,
                                            Ion::Events::Event event) {
    return event == Ion::Events::OK || event == Ion::Events::EXE;
  }
  virtual void textFieldDidStartEditing(AbstractTextField* textField) {}
  virtual bool textFieldDidReceiveEvent(AbstractTextField* textField,
                                        Ion::Events::Event event) {
    return false;
  }
  virtual bool textFieldDidFinishEditing(AbstractTextField* textField,
                                         Ion::Events::Event event) {
    return false;
  }
  virtual void textFieldDidAbortEditing(AbstractTextField* textField) {}
  virtual void textFieldDidHandleEvent(AbstractTextField* textField) {}
  virtual bool textFieldIsEditable(AbstractTextField* textField) {
    return true;
  }
  virtual bool textFieldIsStorable(AbstractTextField* textField) {
    return true;
  }
  virtual void updateRepetitionIndexes(Ion::Events::Event event) {}
  virtual bool shouldInsertSingleQuoteInsteadOfDoubleQuotes() { return true; }
};

}  // namespace Escher
#endif
