#include <poincare/store.h>
#include <assert.h>
#include <ion/circuit_breaker.h>
#include <math.h>
#include <poincare/circuit_breaker_checkpoint.h>
#include <poincare/complex.h>
#include <poincare/context.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>
#include <stdlib.h>

namespace Poincare {

Expression StoreNode::shallowReduce(ReductionContext reductionContext) {
  return Store(this).shallowReduce(reductionContext);
}

template<typename T>
Evaluation<T> StoreNode::templatedApproximate(ApproximationContext approximationContext) const {
  /* If we are here, it means that the store node was not shallowReduced.
   * Otherwise, it would have been replaced by its symbol. We thus have to
   * setExpressionForSymbolAbstract. */
  Expression storedExpression = Store(this).storeValueForSymbol(approximationContext.context());
  assert(!storedExpression.isUninitialized());
  return storedExpression.node()->approximate(T(), approximationContext);
}

Expression Store::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  // Store the expression.
  Expression storedExpression = storeValueForSymbol(reductionContext.context());

  if (symbol().type() == ExpressionNode::Type::Symbol) {
    storedExpression = storedExpression.clone().reduce(reductionContext);
  }

  replaceWithInPlace(storedExpression);
  return storedExpression;
}

Expression Store::storeValueForSymbol(Context * context) const {
  assert(!value().isUninitialized());
  context->setExpressionForSymbolAbstract(value(), symbol());
  Expression storedExpression = context->expressionForSymbolAbstract(symbol(), false);

  if (storedExpression.isUninitialized()) {
    return Undefined::Builder();
  }
  return storedExpression;
}

}
