#include "expression_display_permissions.h"

#include <poincare/unit.h>

using namespace Poincare;

namespace Shared {

namespace ExpressionDisplayPermissions {

static bool neverDisplayExactExpressionOfApproximation(
    Expression approximateOutput, Context* context) {
  /* The angle units could display exact output but we want to avoid exact
   * results that are not in radians like "(3/sqrt(2))°" because they are not
   * relevant for the user.
   * On the other hand, we'd like "cos(4°)" to be displayed as exact result.
   * To do so, the approximateOutput is checked rather than the exactOutput,
   * because the approximateOutput has a unit only if the degree unit is not
   * in a trig function. */
  return !approximateOutput.isUninitialized() && approximateOutput.hasUnit() &&
         !approximateOutput.isInRadians(context);
}

bool NeverDisplayReductionOfInput(Expression input, Context* context) {
  if (input.isUninitialized()) {
    return false;
  }
  return input.deepIsOfType(
      {
          ExpressionNode::Type::ConstantPhysics,
          ExpressionNode::Type::Randint,
          ExpressionNode::Type::RandintNoRepeat,
          ExpressionNode::Type::Random,
          ExpressionNode::Type::Round,
          ExpressionNode::Type::FracPart,
          ExpressionNode::Type::Integral,
          ExpressionNode::Type::Product,
          ExpressionNode::Type::Sum,
          ExpressionNode::Type::Derivative,
          ExpressionNode::Type::Sequence,
          ExpressionNode::Type::DistributionDispatcher,
      },
      context);
}

static bool isPrimeFactorization(Expression expression) {
  /* A prime factorization can only be built with integers, powers of integers,
   * and a multiplication. */
  return !expression.recursivelyMatches([](const Expression e) {
    return e.isUninitialized() ||
           !(e.type() == ExpressionNode::Type::BasedInteger ||
             e.type() == ExpressionNode::Type::Multiplication ||
             (e.type() == ExpressionNode::Type::Power &&
              e.childAtIndex(0).type() == ExpressionNode::Type::BasedInteger &&
              e.childAtIndex(1).type() == ExpressionNode::Type::BasedInteger));
  });
}

static bool exactExpressionIsForbidden(Expression exactOutput) {
  if (!Preferences::SharedPreferences()->examMode().forbidExactResults()) {
    return false;
  }
  if (exactOutput.type() == ExpressionNode::Type::Opposite) {
    return exactExpressionIsForbidden(exactOutput.childAtIndex(0));
  }
  bool isFraction = exactOutput.type() == ExpressionNode::Type::Division &&
                    exactOutput.childAtIndex(0).isNumber() &&
                    exactOutput.childAtIndex(1).isNumber();
  return !(exactOutput.isNumber() || isFraction ||
           isPrimeFactorization(exactOutput));
}

static bool neverDisplayExactOutput(Expression exactOutput, Context* context) {
  if (exactOutput.isUninitialized()) {
    return false;
  }
  return
      /* Force all outputs to be ApproximateOnly if required by the exam mode
       * configuration */
      exactExpressionIsForbidden(exactOutput) ||
      // Lists or Matrices with only nonreal/undefined children
      (exactOutput.isOfType(
           {ExpressionNode::Type::List, ExpressionNode::Type::Matrix}) &&
       exactOutput.allChildrenAreUndefined()) ||
      /* 1. If the output contains a comparison, we only display the
       * approximate output. (this can occur for pi > 3 for example, since
       * it's handled by approximation and not by reduction)
       * 2. If the output has remaining depencies, the exact output is not
       * displayed to avoid outputs like 5 ≈ undef and also because it could
       * be a reduction that failed and was interrupted which can lead to
       * dependencies not being properly bubbled-up */
      exactOutput.deepIsOfType(
          {ExpressionNode::Type::Comparison, ExpressionNode::Type::Dependency},
          context) ||
      // Angle units can have an exact output contrary to other units
      exactOutput.hasUnit(true);
}

bool ShouldOnlyDisplayApproximation(Expression input, Expression exactOutput,
                                    Expression approximateOutput,
                                    Context* context) {
  return NeverDisplayReductionOfInput(input, context) ||
         neverDisplayExactOutput(exactOutput, context) ||
         neverDisplayExactExpressionOfApproximation(approximateOutput, context);
}

}  // namespace ExpressionDisplayPermissions

}  // namespace Shared
