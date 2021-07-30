#ifndef POINCARE_ARC_COSCANT_H
#define POINCARE_ARC_COSCANT_H

#include <poincare/approximation_helper.h>
#include <poincare/expression.h>

namespace Poincare {

class ArcCosecantNode final : public ExpressionNode {
public:
  // TreeNode
  size_t size() const override { return sizeof(ArcCosecantNode); }
  int numberOfChildren() const override;
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "ArcCosecant";
  }
#endif

  // Properties
  Type type() const override { return Type::ArcCosecant; }

  template<typename T> static Complex<T> computeOnComplex(const std::complex<T> c, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit = Preferences::AngleUnit::Radian);

private:
  // Layout
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  // Simplication
  Expression shallowReduce(ReductionContext reductionContext) override;
  LayoutShape leftLayoutShape() const override { return LayoutShape::MoreLetters; };
  LayoutShape rightLayoutShape() const override { return LayoutShape::BoundaryPunctuation; }
  // Evaluation
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::Map<float>(this, approximationContext, computeOnComplex<float>);
  }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::Map<double>(this, approximationContext, computeOnComplex<double>);
  }
};

class ArcCosecant final : public Expression {
public:
  ArcCosecant(const ArcCosecantNode * n) : Expression(n) {}
  static ArcCosecant Builder(Expression child) { return TreeHandle::FixedArityBuilder<ArcCosecant, ArcCosecantNode>({child}); }

  static constexpr Expression::FunctionHelper s_functionHelper = Expression::FunctionHelper("acsc", 1, &UntypedBuilderOneChild<ArcCosecant>);

  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

}

#endif
