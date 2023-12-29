#include <poincare/code_point_layout.h>
#include <poincare/constant.h>
#include <poincare/factorial.h>
#include <poincare/horizontal_layout.h>
#include <poincare/parenthesis.h>
#include <poincare/rational.h>
#include <poincare/serialization_helper.h>
#include <poincare/simplification_helper.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>

#include <cmath>
#include <utility>

namespace Poincare {

// Property

bool FactorialNode::childAtIndexNeedsUserParentheses(const Expression& child,
                                                     int childIndex) const {
  if (child.isNumber() &&
      static_cast<const Number&>(child).isPositive() == TrinaryBoolean::False) {
    return true;
  }
  if (child.isOfType({Type::Conjugate, Type::Dependency})) {
    return childAtIndexNeedsUserParentheses(child.childAtIndex(0), childIndex);
  }
  return child.isOfType({Type::Subtraction, Type::Opposite,
                         Type::Multiplication, Type::Addition});
}

// Layout

bool FactorialNode::childNeedsSystemParenthesesAtSerialization(
    const TreeNode* child) const {
  /*  2
   * --- ! ---> [2/3]!
   *  3
   */
  return SerializationHelper::PostfixChildNeedsSystemParenthesesAtSerialization(
      child);
}

// Simplification

Expression FactorialNode::shallowReduce(
    const ReductionContext& reductionContext) {
  return Factorial(this).shallowReduce(reductionContext);
}

template <typename T>
std::complex<T> FactorialNode::computeOnComplex(
    const std::complex<T> c, Preferences::ComplexFormat,
    Preferences::AngleUnit angleUnit) {
  T n = c.real();
  if (c.imag() != 0 || std::isnan(n) || n != (int)n || n < 0) {
    return complexRealNAN<T>();
  }
  T result = 1;
  for (int i = 1; i <= (int)n; i++) {
    result *= static_cast<T>(i);
    if (std::isinf(result)) {
      return result;
    }
  }
  return std::round(result);
}

Layout FactorialNode::createLayout(Preferences::PrintFloatMode floatDisplayMode,
                                   int numberOfSignificantDigits,
                                   Context* context) const {
  HorizontalLayout result = HorizontalLayout::Builder();
  result.addOrMergeChildAtIndex(
      childAtIndex(0)->createLayout(floatDisplayMode, numberOfSignificantDigits,
                                    context),
      0);
  int childrenCount = result.numberOfChildren();
  result.addChildAtIndexInPlace(CodePointLayout::Builder('!'), childrenCount,
                                childrenCount);
  return std::move(result);
}

size_t FactorialNode::serialize(char* buffer, size_t bufferSize,
                                Preferences::PrintFloatMode floatDisplayMode,
                                int numberOfSignificantDigits) const {
  size_t numberOfChar = SerializationHelper::SerializeChild(
      childAtIndex(0), this, buffer, bufferSize, floatDisplayMode,
      numberOfSignificantDigits);
  if ((numberOfChar < 0) || (numberOfChar >= bufferSize - 1)) {
    return numberOfChar;
  }
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, bufferSize - numberOfChar, '!');
  return numberOfChar;
}

Expression Factorial::shallowReduce(ReductionContext reductionContext) {
  {
    Expression e = SimplificationHelper::defaultShallowReduce(
        *this, &reductionContext,
        SimplificationHelper::BooleanReduction::UndefinedOnBooleans,
        SimplificationHelper::UnitReduction::BanUnits,
        SimplificationHelper::MatrixReduction::UndefinedOnMatrix,
        SimplificationHelper::ListReduction::DistributeOverLists);
    if (!e.isUninitialized()) {
      return e;
    }
  }
  Expression c = childAtIndex(0);
  if (c.type() == ExpressionNode::Type::Rational) {
    Rational r = c.convert<Rational>();
    if (!r.isInteger() || r.isPositive() == TrinaryBoolean::False) {
      return replaceWithUndefinedInPlace();
    }
    if (Integer(k_maxOperandValue).isLowerThan(r.unsignedIntegerNumerator())) {
      return *this;
    }
    Rational fact =
        Rational::Builder(Integer::Factorial(r.unsignedIntegerNumerator()));
    // Because fact < k_maxOperandValue!
    assert(!fact.numeratorOrDenominatorIsInfinity());
    replaceWithInPlace(fact);
    return std::move(fact);
  }
  if (c.isOfType({ExpressionNode::Type::ConstantMaths,
                  ExpressionNode::Type::ConstantPhysics})) {
    // e! = undef, i! = undef, pi! = undef
    return replaceWithUndefinedInPlace();
  }
  return *this;
}

#if 0
int Factorial::simplificationOrderGreaterType(const Expression * e) const {
  if (SimplificationOrder(childAtIndex(0),e) == 0) {
    return 1;
  }
  return SimplificationOrder(childAtIndex(0), e);
}

int Factorial::simplificationOrderSameType(const Expression * e) const {
  return SimplificationOrder(childAtIndex(0), e->childAtIndex(0));
}
#endif

}  // namespace Poincare
