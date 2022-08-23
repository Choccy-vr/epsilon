#ifndef POINCARE_COTANGENT_H
#define POINCARE_COTANGENT_H

#include <poincare/approximation_helper.h>
#include <poincare/expression.h>

namespace Poincare {

class CotangentNode final : public ExpressionNode {
public:
  static constexpr char functionName[] = "cot";

  // TreeNode
  size_t size() const override { return sizeof(CotangentNode); }
  int numberOfChildren() const override;
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "Cotangent";
  }
#endif

  // Properties
  Type type() const override { return Type::Cotangent; }

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
    return ApproximationHelper::MapOneChild<float>(this, approximationContext, computeOnComplex<float>);
  }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::MapOneChild<double>(this, approximationContext, computeOnComplex<double>);
  }
};

class Cotangent final : public HandleOneChild<Cotangent, CotangentNode> {
public:
  using Handle::Handle, Handle::Builder, Handle::s_functionHelper;
  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

}

#endif
