#ifndef POINCARE_LIST_ACCESS_H
#define POINCARE_LIST_ACCESS_H

#include <poincare/expression.h>
#include <poincare/symbol.h>
#include <poincare/serialization_helper.h>

namespace Poincare {

template<int U>
class ListAccessNode : public ExpressionNode {
public:
  static constexpr int k_listChildIndex = U;

  size_t size() const override { return sizeof(ListAccessNode); }
  int numberOfChildren() const override { return U + 1; }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << (U == 1 ? "ListElement" : "ListSlice");
  }
#endif
  Type type() const override;
  LayoutShape leftLayoutShape() const override { return LayoutShape::BoundaryPunctuation; };

private:
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override { return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, symbolChild()->name(), false, U - 1); }
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;

  Expression shallowReduce(ReductionContext reductionContext) override;

  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  template<typename T> Evaluation<T> templatedApproximate(ApproximationContext approximationContext) const;

  SymbolNode * symbolChild() const { assert(childAtIndex(k_listChildIndex)->type() == Type::Symbol); return static_cast<SymbolNode *>(childAtIndex(k_listChildIndex)); }
};

class ListElement : public Expression {
public:
  ListElement(const ListAccessNode<1> * n) : Expression(n) {}
  static ListElement Builder(Expression index, Expression list) { return TreeHandle::FixedArityBuilder<ListElement, ListAccessNode<1>>({index, list}); }

  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

class ListSlice : public Expression {
public:
  ListSlice(const ListAccessNode<2> * n) : Expression(n) {}
  static ListSlice Builder(Expression firstIndex, Expression lastIndex, Expression list) { return TreeHandle::FixedArityBuilder<ListSlice, ListAccessNode<2>>({firstIndex, lastIndex, list}); }

  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
};

}

#endif
