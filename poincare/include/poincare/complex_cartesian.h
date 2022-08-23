#ifndef POINCARE_COMPLEX_CARTESIAN_H
#define POINCARE_COMPLEX_CARTESIAN_H

#include <poincare/expression.h>
#include <poincare/multiplication.h>

namespace Poincare {

class ComplexCartesianNode : public ExpressionNode {
public:

  // TreeNode
  size_t size() const override { return sizeof(ComplexCartesianNode); }
  int numberOfChildren() const override { return 2; }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "ComplexCartesian";
  }
#endif

  // Properties
  Sign sign(Context * context) const override { return childAtIndex(1)->nullStatus(context) == NullStatus::Null ? childAtIndex(0)->sign(context) : Sign::Unknown; }
  NullStatus nullStatus(Context * context) const override;
  Type type() const override { return Type::ComplexCartesian; }
private:
  // Layout
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override { assert(false); return Layout(); }
  // Evaluation
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  // Simplification
  Expression shallowBeautify(const ReductionContext& reductionContext) override;
  Expression shallowReduce(const ReductionContext& reductionContext) override;
  LayoutShape leftLayoutShape() const override {
    /* leftLayoutShape is called after beautifying expression. ComplexCartesian
     * is transformed in another expression at beautifying. */
    assert(false);
    return LayoutShape::BoundaryPunctuation;
  };

  template<typename T> Complex<T> templatedApproximate(ApproximationContext approximationContext) const;
};

class ComplexCartesian final : public Expression {
public:
  ComplexCartesian() : Expression() {}
  ComplexCartesian(const ComplexCartesianNode * node) : Expression(node) {}
  static ComplexCartesian Builder() { return TreeHandle::FixedArityBuilder<ComplexCartesian, ComplexCartesianNode>(); }
  static ComplexCartesian Builder(Expression child0, Expression child1) { return TreeHandle::FixedArityBuilder<ComplexCartesian, ComplexCartesianNode>({child0, child1}); }

  // Getters
  Expression real() { return childAtIndex(0); }
  Expression imag() { return childAtIndex(1); }

  // Simplification
  Expression shallowBeautify(const ExpressionNode::ReductionContext& reductionContext);
  Expression shallowReduce(const ExpressionNode::ReductionContext& reductionContext);

  // Common operations (done in-place)
  Expression squareNorm(const ExpressionNode::ReductionContext& reductionContext);
  Expression norm(const ExpressionNode::ReductionContext& reductionContext);
  Expression argument(const ExpressionNode::ReductionContext& reductionContext);
  ComplexCartesian inverse(const ExpressionNode::ReductionContext& reductionContext);
  ComplexCartesian squareRoot(const ExpressionNode::ReductionContext& reductionContext);
  ComplexCartesian powerInteger(int n, const ExpressionNode::ReductionContext& reductionContext);
  ComplexCartesian multiply(ComplexCartesian & other,const ExpressionNode::ReductionContext& reductionContext);
  ComplexCartesian power(ComplexCartesian & other, const ExpressionNode::ReductionContext& reductionContext);
private:
  static constexpr int k_maxNumberOfNodesBeforeInterrupting = 50;
  void factorAndArgumentOfFunction(Expression e, ExpressionNode::Type searchedType, Expression * factor, Expression * argument, const ExpressionNode::ReductionContext& reductionContext);
  ComplexCartesian interruptComputationIfManyNodes();
  static Multiplication squareRootHelper(Expression e, const ExpressionNode::ReductionContext& reductionContext);
  static Expression powerHelper(Expression norm, Expression trigo, const ExpressionNode::ReductionContext& reductionContext);
};

}

#endif
