#ifndef POINCARE_PARAMETERED_EXPRESSION_H
#define POINCARE_PARAMETERED_EXPRESSION_H

#include <poincare/expression.h>

namespace Poincare {

/* Parametered expressions are Integral, Derivative, Sum, Product and
 * List Sequence. Their child at index 0 is parametered, and the child
 * at index 1 is the parameter symbol.
 * */

class ParameteredExpressionNode : public ExpressionNode {
public:
  // Expression
  Expression replaceSymbolWithExpression(const SymbolAbstract & symbol, const Expression & expression) override;
  Expression deepReplaceReplaceableSymbols(Context * context, bool * isCircular, int maxSymbolsToReplace, int parameteredAncestorsCount, SymbolicComputation symbolicComputation) override;

  // Expression properties
  int getVariables(Context * context, isVariableTest isVariable, char * variables, int maxSizeVariable, int nextVariableIndex) const override;

protected:
  // Evaluation
  template<typename T> Evaluation<T> approximateFirstChildWithArgument(T x, ApproximationContext approximationContext) const;
  template<typename T> T firstChildScalarValueForArgument(T x, ApproximationContext approximationContext) const {
    return approximateFirstChildWithArgument(x, approximationContext).toScalar();
  }
  template<typename T> Evaluation<T> approximateExpressionWithArgument(ExpressionNode * child, T x, ApproximationContext approximationContext) const;

};

class ParameteredExpression : public Expression {
  friend class ParameteredExpressionNode;
public:
  // ParameteredExpression
  static constexpr int ParameteredChildIndex() { return 0; }
  static constexpr int ParameterChildIndex() { return 1; }

  // Expression
  /* We sometimes replace 'x' by 'UnknownX' to differentiate between a variable
   * and a function parameter. For instance, when defining the function
   * f(x)=cos(x) in Graph, we want x to be an unknown, instead of having f be
   * the constant function equal to cos(user variable that is named x).
   *
   * In parametered expressions, we do not want to replace all the 'x' with
   * unknowns: for instance, we want to change f(x)=diff(cos(x),x,x) into
   * f(X)=diff(cos(x),x,X), X being an unknown. ReplaceUnknownInExpression does
   * that. */
  Expression replaceSymbolWithExpression(const SymbolAbstract & symbol, const Expression & expression);
  Expression deepReplaceReplaceableSymbols(Context * context, bool * isCircular, int maxSymbolsToReplace, int parameteredAncestorsCount, ExpressionNode::SymbolicComputation symbolicComputation);
  Symbol parameter();
protected:
  ParameteredExpression(const ParameteredExpressionNode * node) : Expression(node) {}
};

}

#endif
