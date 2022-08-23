#include <poincare/list_minimum.h>
#include <poincare/layout_helper.h>
#include <poincare/list_helpers.h>
#include <poincare/serialization_helper.h>
#include <poincare/undefined.h>

namespace Poincare {

const Expression::FunctionHelper ListMinimum::s_functionHelper;

int ListMinimumNode::numberOfChildren() const {
  return ListMinimum::s_functionHelper.numberOfChildren();
}

int ListMinimumNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, ListMinimum::s_functionHelper.name());
}

Layout ListMinimumNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Prefix(ListMinimum(this), floatDisplayMode, numberOfSignificantDigits, ListMinimum::s_functionHelper.name());
}

Expression ListMinimumNode::shallowReduce(ReductionContext reductionContext) {
  return ListMinimum(this).shallowReduce(reductionContext);
}

template<typename T> Evaluation<T> ListMinimumNode::templatedApproximate(ApproximationContext approximationContext) const {
  ExpressionNode * child = childAtIndex(0);
  if (child->type() != ExpressionNode::Type::List) {
    return Complex<T>::Undefined();
  }
  return ListHelpers::ExtremumApproximationOfListNode<T>(static_cast<ListNode *>(child), approximationContext, true);
}

Expression ListMinimum::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  Expression child = childAtIndex(0);
  if (child.type() != ExpressionNode::Type::List) {
    return replaceWithUndefinedInPlace();
  }

  Expression result = ListHelpers::ExtremumOfList(static_cast<List &>(child), reductionContext, true);
  replaceWithInPlace(result);
  return result;
}

template Evaluation<float> ListMinimumNode::templatedApproximate<float>(ApproximationContext approximationContext) const;
template Evaluation<double> ListMinimumNode::templatedApproximate<double>(ApproximationContext approximationContext) const;

}
