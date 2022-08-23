#ifndef POINCARE_LIST_STANDARD_DEVIATION_H
#define POINCARE_LIST_STANDARD_DEVIATION_H

#include <poincare/expression.h>

namespace Poincare {

class ListStandardDeviationNode : public ExpressionNode {
public:
  size_t size() const override { return sizeof(ListStandardDeviationNode); }
  int numberOfChildren() const override;
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "ListStandardDeviation";
  }
#endif
  Type type() const override { return Type::ListStandardDeviation; }
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

class ListStandardDeviation : public Expression {
public:
  static constexpr FunctionHelper s_functionHelper = FunctionHelper("stddev", 1, &UntypedBuilderOneChild<ListStandardDeviation>);

  ListStandardDeviation(const ListStandardDeviationNode * n) : Expression(n) {}
  static ListStandardDeviation Builder(Expression list) { return TreeHandle::FixedArityBuilder<ListStandardDeviation, ListStandardDeviationNode>({list}); }

  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

}

#endif
