#include "text_field_delegate_app.h"
#include <apps/apps_container.h>
#include <apps/constant.h>
#include <apps/shared/poincare_helpers.h>
#include <poincare/comparison_operator.h>
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
bool TextFieldDelegateApp::hasUndefinedValue(const char * text, T * value, bool enablePlusInfinity, bool enableMinusInfinity) {
  *value = PoincareHelpers::ApproximateToScalar<T>(text, localContext());
  bool isUndefined = std::isnan(*value)
    || (!enablePlusInfinity && *value > 0 && std::isinf(*value))
    || (!enableMinusInfinity && *value < 0 && std::isinf(*value));
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
    CodePoint defaultXNT = XNT();
    int XNTIndex = Ion::Events::repetitionFactor();
    if (XNTIndex > 0) {
      // Cycle through XNT CodePoints, starting from default code point position
      constexpr size_t k_numberOfCodePoints = 4;
      constexpr CodePoint XNTCodePoints[k_numberOfCodePoints] = {'x', 'n', 't', UCodePointGreekSmallLetterTheta};
      for (size_t i = 0; i < k_numberOfCodePoints; i++) {
        XNTIndex++;
        if (XNTCodePoints[i] == defaultXNT) {
          break;
        }
      }
      // Unknown default code point
      defaultXNT = XNTCodePoints[XNTIndex % k_numberOfCodePoints];
    }
    return field->addXNTCodePoint(defaultXNT);
  }
  return false;
}

bool TextFieldDelegateApp::isFinishingEvent(Ion::Events::Event event) {
  return event == Ion::Events::OK || event == Ion::Events::EXE;
}

bool TextFieldDelegateApp::isAcceptableExpression(const Expression exp) {
  // Most TextFieldDelegateApps shouldn't accept Store or ComparisonOperators.
  return !(exp.isUninitialized() || exp.type() == ExpressionNode::Type::Store || ComparisonOperator::IsComparisonOperatorType(exp.type()));
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

template bool TextFieldDelegateApp::hasUndefinedValue(const char *, float *, bool, bool);
template bool TextFieldDelegateApp::hasUndefinedValue(const char *, double *, bool, bool);

}
