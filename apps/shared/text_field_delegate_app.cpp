#include "text_field_delegate_app.h"
#include <apps/apps_container.h>
#include <apps/constant.h>
#include <apps/shared/poincare_helpers.h>
#include <cmath>
#include <string.h>

using namespace Escher;
using namespace Poincare;

namespace Shared {

Context * TextFieldDelegateApp::localContext() {
  return AppsContainer::sharedAppsContainer()->globalContext();
}

bool TextFieldDelegateApp::textFieldShouldFinishEditing(TextField * textField, Ion::Events::Event event) {
  return isFinishingEvent(event);
}

bool TextFieldDelegateApp::textFieldDidReceiveEvent(TextField * textField, Ion::Events::Event event) {
  if (textField->isEditing() && textField->shouldFinishEditing(event)) {
    if (!isAcceptableText(textField->text())) {
      return true;
    }
  }
  if (fieldDidReceiveEvent(textField, textField, event)) {
    return true;
  }
  return false;
}


bool TextFieldDelegateApp::isAcceptableText(const char * text) {
  Expression exp = Expression::Parse(text, localContext());
  bool isAcceptable = isAcceptableExpression(exp);
  if (!isAcceptable) {
    displayWarning(I18n::Message::SyntaxError);
  }
  return isAcceptable;
}

template<typename T>
bool TextFieldDelegateApp::hasUndefinedValue(const char * text, T & value, bool enablePlusInfinity, bool enableMinusInfinity) {
  value = PoincareHelpers::ApproximateToScalar<T>(text, localContext());
  bool isUndefined = std::isnan(value)
    || (!enablePlusInfinity && value > 0 && std::isinf(value))
    || (!enableMinusInfinity && value < 0 && std::isinf(value));
  if (isUndefined) {
    displayWarning(I18n::Message::UndefinedValue);
  }
  return isUndefined;
}

/* Protected */

TextFieldDelegateApp::TextFieldDelegateApp(Snapshot * snapshot, ViewController * rootViewController) :
  InputEventHandlerDelegateApp(snapshot, rootViewController),
  TextFieldDelegate()
{
}

bool TextFieldDelegateApp::fieldDidReceiveEvent(EditableField * field, Responder * responder, Ion::Events::Event event) {
  if (event == Ion::Events::XNT) {
    return field->addXNTCodePoint(XNT());
  }
  return false;
}

bool TextFieldDelegateApp::isFinishingEvent(Ion::Events::Event event) {
  return event == Ion::Events::OK || event == Ion::Events::EXE;
}

bool TextFieldDelegateApp::isAcceptableExpression(const Expression exp) {
  return !(exp.isUninitialized() || exp.type() == ExpressionNode::Type::Store || exp.type() == ExpressionNode::Type::Equal);
}

bool TextFieldDelegateApp::ExpressionCanBeSerialized(const Expression expression, bool replaceAns, Expression ansExpression, Context * context) {
  if (expression.isUninitialized()) {
    return false;
  }
  Expression exp = expression;
  if (replaceAns){
    exp = expression.clone();
    Symbol ansSymbol = Symbol::Ans();
    exp = exp.replaceSymbolWithExpression(ansSymbol, ansExpression);
  }
  constexpr int maxSerializationSize = Constant::MaxSerializedExpressionSize;
  char buffer[maxSerializationSize];
  int length = PoincareHelpers::Serialize(exp, buffer, maxSerializationSize);
  /* If the buffer is totally full, it is VERY likely that writeTextInBuffer
   * escaped before printing utterly the expression. */
  if (length >= maxSerializationSize-1) {
    return false;
  }
  if (replaceAns) {
    exp = Expression::Parse(buffer, context);
    if (exp.isUninitialized()) {
      // The ans replacement made the expression unparsable
      return false;
    }
  }
  return true;
}

template bool TextFieldDelegateApp::hasUndefinedValue(const char *, float &, bool, bool);
template bool TextFieldDelegateApp::hasUndefinedValue(const char *, double &, bool, bool);

}
