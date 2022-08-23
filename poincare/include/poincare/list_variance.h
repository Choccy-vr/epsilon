#ifndef POINCARE_LIST_VARIANCE_H
#define POINCARE_LIST_VARIANCE_H

#include <poincare/list.h>

namespace Poincare {

template<int U>
class ListVarianceNode : public ExpressionNode {
public:
  size_t size() const override { return sizeof(ListVarianceNode<U>); }
  int numberOfChildren() const override { return U; }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "ListVariance";
  }
  void logAttributes(std::ostream & stream) const override {
    stream << " numberOfParameters=\"" << U << "\"";
  }
#endif
  Type type() const override { return Type::ListVariance; }
  LayoutShape leftLayoutShape() const override { return LayoutShape::MoreLetters; };
  LayoutShape rightLayoutShape() const override { return LayoutShape::BoundaryPunctuation; }

private:
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;

  Expression shallowReduce(const ReductionContext& reductionContext) override;

  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  template<typename T> Evaluation<T> templatedApproximate(ApproximationContext approximationContext) const;
};

class ListVariance : public Expression {
public:
  constexpr static const char * k_functionName = "var";
  constexpr static FunctionHelper s_functionHelperOneChild = FunctionHelper(k_functionName, 1, &Initializer<ListVarianceNode<1>>, sizeof(ListVarianceNode<1>));
  constexpr static FunctionHelper s_functionHelperTwoChildren = FunctionHelper(k_functionName, 2, &Initializer<ListVarianceNode<2>>, sizeof(ListVarianceNode<2>));

  ListVariance(const ListVarianceNode<1> * n) : Expression(n) {}
  ListVariance(const ListVarianceNode<2> * n) : Expression(n) {}

  static ListVariance Builder(Expression list) { return TreeHandle::FixedArityBuilder<ListVariance, ListVarianceNode<1>>({list}); }
  static ListVariance Builder(Expression values, Expression weights) { return TreeHandle::FixedArityBuilder<ListVariance, ListVarianceNode<2>>({values, weights}); }

  Expression shallowReduce(const ExpressionNode::ReductionContext& reductionContext);
};

}

#endif
