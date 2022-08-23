#ifndef POINCARE_FACTOR_H
#define POINCARE_FACTOR_H

#include <poincare/expression.h>
#include <poincare/rational.h>
#include <poincare/multiplication.h>
#include <cmath>

namespace Poincare {

class FactorNode /*final*/ : public ExpressionNode {
public:
  static constexpr char k_functionName[] = "factor";

  // TreeNode
  size_t size() const override { return sizeof(FactorNode); }
  int numberOfChildren() const override;
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "Factor";
  }
#endif
  Sign sign(Context * context) const override { return childAtIndex(0)->sign(context); }
  NullStatus nullStatus(Context * context) const override { return childAtIndex(0)->nullStatus(context); }
  Type type() const override { return Type::Factor; }
private:
  /* Layout */
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  /* Serialization */
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  /* Simplification */
  Expression shallowBeautify(const ReductionContext& reductionContext) override;
  Expression shallowReduce(const ReductionContext& reductionContext) override;
  LayoutShape leftLayoutShape() const override { return LayoutShape::MoreLetters; };
  LayoutShape rightLayoutShape() const override { return LayoutShape::BoundaryPunctuation; }

  /* Evaluation */
  Evaluation<float> approximate(SinglePrecision p, const ApproximationContext& approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, const ApproximationContext& approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  template<typename T> Evaluation<T> templatedApproximate(const ApproximationContext& approximationContext) const;
};

class Factor final : public ExpressionOneChild<Factor, FactorNode> {
public:
  using ExpressionBuilder::ExpressionBuilder;

  Multiplication createMultiplicationOfIntegerPrimeDecomposition(Integer i) const;

  // Expression
  Expression shallowBeautify(const ExpressionNode::ReductionContext& reductionContext);
  Expression shallowReduce(const ExpressionNode::ReductionContext& reductionContext);
};

}
#endif
