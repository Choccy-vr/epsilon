#include <poincare/subtraction.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <poincare/simplification_helper.h>
#include <poincare/addition.h>
#include <poincare/multiplication.h>
#include <poincare/opposite.h>
#include <poincare/rational.h>
#include <assert.h>

namespace Poincare {

int SubtractionNode::polynomialDegree(Context * context, const char * symbolName) const {
  int degree = 0;
  for (ExpressionNode * e : children()) {
    int d = e->polynomialDegree(context, symbolName);
    if (d < 0) {
      return -1;
    }
    degree = d > degree ? d : degree;
  }
  return degree;
}

// Private

bool SubtractionNode::childAtIndexNeedsUserParentheses(const Expression & child, int childIndex) const {
  if (childIndex == 0) {
    // First operand of a subtraction never requires parentheses
    return false;
  }
  if (child.isNumber() && static_cast<const Number &>(child).sign() == Sign::Negative) {
    return true;
  }
  if (child.type() == Type::Conjugate) {
    return childAtIndexNeedsUserParentheses(child.childAtIndex(0), childIndex);
  }
  Type types[] = {Type::Subtraction, Type::Opposite, Type::Addition};
  return child.isOfType(types, 3);
}

Layout SubtractionNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Infix(Subtraction(this), floatDisplayMode, numberOfSignificantDigits, "-");
}

int SubtractionNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
    return SerializationHelper::Infix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, "-");
}

Expression SubtractionNode::shallowReduce(ReductionContext reductionContext) {
  return Subtraction(this).shallowReduce(reductionContext);
}

Expression Subtraction::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  Expression e = SimplificationHelper::shallowReduceUndefined(*this);
  if (!e.isUninitialized()) {
    return e;
  }
  Expression m = Multiplication::Builder(Rational::Builder(-1), childAtIndex(1));
  Addition a = Addition::Builder(childAtIndex(0), m);
  m = m.shallowReduce(reductionContext);
  replaceWithInPlace(a);
  return a.shallowReduce(reductionContext);
}

template Evaluation<float> Poincare::SubtractionNode::Compute<float>(Evaluation<float> eval1, Evaluation<float> eval2, Poincare::ExpressionNode::ApproximationContext approximationContext);
template Evaluation<double> Poincare::AdditionNode::Compute<double>(Evaluation<double> eval1, Evaluation<double> eval2, Poincare::ExpressionNode::ApproximationContext approximationContext);

template Evaluation<float> Poincare::SubtractionNode::templatedApproximate<float>(Poincare::ExpressionNode::ApproximationContext approximationContext) const;
template Evaluation<double> Poincare::AdditionNode::templatedApproximate<double>(Poincare::ExpressionNode::ApproximationContext approximationContext) const;

}
