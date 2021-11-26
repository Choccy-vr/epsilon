#ifndef SHARED_EXPRESSION_FIELD_DELEGATE_APP_H
#define SHARED_EXPRESSION_FIELD_DELEGATE_APP_H

#include "text_field_delegate_app.h"
#include <escher/layout_field_delegate.h>

namespace Shared {

class ExpressionFieldDelegateApp : public TextFieldDelegateApp, public Escher::LayoutFieldDelegate {
public:
  virtual ~ExpressionFieldDelegateApp() = default;
  bool layoutFieldShouldFinishEditing(Escher::LayoutField * layoutField, Ion::Events::Event event) override;
  bool layoutFieldDidReceiveEvent(Escher::LayoutField * layoutField, Ion::Events::Event event) override;
  bool isAcceptableExpression(const Poincare::Expression expression) override;
protected:
  ExpressionFieldDelegateApp(Snapshot * snapshot, Escher::ViewController * rootViewController);
};

}

#endif
