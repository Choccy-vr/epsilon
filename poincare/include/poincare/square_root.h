#ifndef POINCARE_SQUARE_ROOT_H
#define POINCARE_SQUARE_ROOT_H

#include <poincare/approximation_helper.h>
#include <poincare/expression.h>
#include <poincare/multiplication.h>

namespace Poincare {

class SquareRootNode /*final*/ : public ExpressionNode  {
public:
  // ExpressionNode
  Sign sign(Context * context) const override { return childAtIndex(0)->sign(context) == Sign::Positive ? Sign::Positive : Sign::Unknown ; }
  NullStatus nullStatus(Context * context) const override { return childAtIndex(0)->nullStatus(context); }
  Type type() const override { return Type::SquareRoot; }

  // TreeNode
  size_t size() const override { return sizeof(SquareRootNode); }
  int numberOfChildren() const override;
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "SquareRoot";
  }
#endif

  template<typename T> static Complex<T> computeOnComplex(const std::complex<T> c, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit);

private:
  // Layout
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  // Simplification
  Expression shallowReduce(ReductionContext reductionContext) override;
  LayoutShape leftLayoutShape() const override { return LayoutShape::Root; };
  // Evaluation
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::MapOneChild<float>(this, approximationContext, computeOnComplex<float>);
  }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::MapOneChild<double>(this, approximationContext, computeOnComplex<double>);
  }
};

class SquareRoot final : public Expression {
public:
  SquareRoot(const SquareRootNode * n) : Expression(n) {}
  static SquareRoot Builder(Expression child) { return TreeHandle::FixedArityBuilder<SquareRoot, SquareRootNode>({child}); }
  /* Reduce an expression of the form √(a√b + c√d) */
  static Expression ReduceNestedRadicals(Expression a, Expression b, Expression c, Expression d, ExpressionNode::ReductionContext reductionContext);
  static bool SplitRadical(Expression term, Expression * factor, Expression * underRoot);
  static constexpr Expression::FunctionHelper s_functionHelper = Expression::FunctionHelper("√", 1, Initializer<SquareRootNode>, sizeof(SquareRootNode));
  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

}

#endif
