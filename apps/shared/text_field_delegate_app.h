#ifndef SHARED_TEXT_FIELD_DELEGATE_APP_H
#define SHARED_TEXT_FIELD_DELEGATE_APP_H

#include <poincare/context.h>
#include "input_event_handler_delegate_app.h"
#include <escher/editable_field.h>
#include <escher/text_field_delegate.h>
#include <apps/i18n.h>

namespace Shared {

class TextFieldDelegateApp : public InputEventHandlerDelegateApp, public Escher::TextFieldDelegate {
public:
  virtual ~TextFieldDelegateApp() = default;
  Poincare::Context * localContext() override;
  virtual CodePoint XNT() { return 'x'; }
  bool textFieldShouldFinishEditing(Escher::TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidReceiveEvent(Escher::TextField * textField, Ion::Events::Event event) override;
  bool isAcceptableText(const char * text);
  template<typename T>
  bool hasUndefinedValue(const char * text, T * value, bool enablePlusInfinity = false, bool enableMinusInfinity = false);
protected:
  TextFieldDelegateApp(Snapshot * snapshot, Escher::ViewController * rootViewController);
  bool fieldDidReceiveEvent(Escher::EditableField * field, Escher::Responder * responder, Ion::Events::Event event);
  bool isFinishingEvent(Ion::Events::Event event);
  virtual bool isAcceptableExpression(const Poincare::Expression expression);
  static bool ExpressionCanBeSerialized(const Poincare::Expression expression, bool replaceAns, Poincare::Expression ansExpression, Poincare::Context * context);
};

}

#endif
