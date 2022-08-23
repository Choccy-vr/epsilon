#ifndef POINCARE_LIST_MAXIMUM_H
#define POINCARE_LIST_MAXIMUM_H

#include <poincare/expression.h>

namespace Poincare {

class ListMaximumNode : public ExpressionNode {
public:
  size_t size() const override { return sizeof(ListMaximumNode); }
  int numberOfChildren() const override;
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "ListMaximum";
  }
#endif
  Type type() const override { return Type::ListMaximum; }
  LayoutShape leftLayoutShape() const override { return LayoutShape::MoreLetters; };
  LayoutShape rightLayoutShape() const override { return LayoutShape::BoundaryPunctuation; }

private:
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;

  Expression shallowReduce(ReductionContext reductionContext) override;

  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  template<typename T> Evaluation<T> templatedApproximate(ApproximationContext approximationContext) const;
};

class ListMaximum : public Expression {
public:
  static constexpr FunctionHelper s_functionHelper = FunctionHelper("max", 1, &UntypedBuilderOneChild<ListMaximum>);

  ListMaximum(const ListMaximumNode * n) : Expression(n) {}
  static ListMaximum Builder(Expression list) { return TreeHandle::FixedArityBuilder<ListMaximum, ListMaximumNode>({list}); }

  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

}

#endif
