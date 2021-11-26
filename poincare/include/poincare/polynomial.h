#ifndef POINCARE_POLYNOMIAL_H
#define POINCARE_POLYNOMIAL_H

#include <poincare/expression.h>
#include <poincare/rational.h>

namespace Poincare {

class Polynomial {
public:
  static int LinearPolynomialRoots(Expression a, Expression b, Expression * root, Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit);

  static int QuadraticPolynomialRoots(Expression a, Expression b, Expression c, Expression * root1, Expression * root2, Expression * delta, Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit, bool approximateSolutions = false);

  static int CubicPolynomialRoots(Expression a, Expression b, Expression c, Expression d, Expression * root1, Expression * root2, Expression * root3, Expression * delta, Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit, bool * approximateSolutions = nullptr);

private:
  static constexpr int k_maxNumberOfNodesBeforeApproximatingDelta = 16;
  static Expression ReducePolynomial(const Expression * coefficients, int degree, Expression parameter, ExpressionNode::ReductionContext reductionContext);
  static Rational ReduceRationalPolynomial(const Rational * coefficients, int degree, Rational parameter);
  static bool IsRoot(const Expression * coefficients, int degree, Expression root, ExpressionNode::ReductionContext reductionContext) { return ReducePolynomial(coefficients, degree, root, reductionContext).nullStatus(reductionContext.context()) == ExpressionNode::NullStatus::Null; }
  static Expression RationalRootSearch(const Expression * coefficients, int degree, ExpressionNode::ReductionContext reductionContext);
  static Expression SumRootSearch(const Expression * coefficients, int degree, int relevantCoefficient, ExpressionNode::ReductionContext reductionContext);
  static Expression CardanoNumber(Expression delta0, Expression delta1, bool * approximate, ExpressionNode::ReductionContext reductionContext);
};

}

#endif
