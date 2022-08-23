#include <poincare/permute_coefficient.h>
#include <poincare/approximation_helper.h>
#include <poincare/undefined.h>
#include <poincare/letter_a_with_sub_and_superscript_layout.h>
#include <poincare/rational.h>
#include <poincare/layout_helper.h>
#include <poincare/preferences.h>
#include <poincare/serialization_helper.h>
#include <poincare/simplification_helper.h>

extern "C" {
#include <assert.h>
}
#include <cmath>

namespace Poincare {

constexpr Expression::FunctionHelper PermuteCoefficient::s_functionHelper;

int PermuteCoefficientNode::numberOfChildren() const { return PermuteCoefficient::s_functionHelper.numberOfChildren(); }

Layout PermuteCoefficientNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  if (Preferences::sharedPreferences()->combinatoricSymbols() == Preferences::CombinatoricSymbols::Default) {
    return LayoutHelper::Prefix(PermuteCoefficient(this), floatDisplayMode, numberOfSignificantDigits, PermuteCoefficient::s_functionHelper.name());
  } else {
    return LetterAWithSubAndSuperscriptLayout::Builder(
      childAtIndex(0)->createLayout(floatDisplayMode, numberOfSignificantDigits),
      childAtIndex(1)->createLayout(floatDisplayMode, numberOfSignificantDigits));
  }
}

int PermuteCoefficientNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, PermuteCoefficient::s_functionHelper.name());
}

Expression PermuteCoefficientNode::shallowReduce(ReductionContext reductionContext) {
  return PermuteCoefficient(this).shallowReduce(reductionContext);
}

template<typename T>
Evaluation<T> PermuteCoefficientNode::templatedApproximate(ApproximationContext approximationContext) const {
  return ApproximationHelper::Map<T>(
      this,
      approximationContext,
      [] (const std::complex<T> * c, int numberOfComplexes, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit, void * ctx) {
        assert(numberOfComplexes == 2);
        T n = ComplexNode<T>::ToScalar(c[0]);
        T k = ComplexNode<T>::ToScalar(c[1]);
        if (std::isnan(n) || std::isnan(k) || n != std::round(n) || k != std::round(k) || n < 0.0f || k < 0.0f) {
          return Complex<T>::RealUndefined();
        }
        if (k > n) {
          return Complex<T>::Builder(0.0);
        }
        T result = 1;
        for (int i = (int)n-(int)k+1; i <= (int)n; i++) {
          result *= i;
          if (std::isinf(result) || std::isnan(result)) {
            return Complex<T>::Builder(result);
          }
        }
        return Complex<T>::Builder(std::round(result));
      });
}


Expression PermuteCoefficient::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  {
    Expression e = SimplificationHelper::defaultShallowReduce(*this);
    if (!e.isUninitialized()) {
      return e;
    }
    e = SimplificationHelper::undefinedOnMatrix(*this, reductionContext);
    if (!e.isUninitialized()) {
      return e;
    }
    e = SimplificationHelper::distributeReductionOverLists(*this, reductionContext);
    if (!e.isUninitialized()) {
      return e;
    }
  }
  Expression c0 = childAtIndex(0);
  Expression c1 = childAtIndex(1);
  if (c0.type() == ExpressionNode::Type::Rational) {
    Rational r0 = static_cast<Rational &>(c0);
    if (!r0.isInteger() || r0.sign() == ExpressionNode::Sign::Negative) {
      return replaceWithUndefinedInPlace();
    }
  }
  if (c1.type() == ExpressionNode::Type::Rational) {
    Rational r1 = static_cast<Rational &>(c1);
    if (!r1.isInteger() || r1.sign() == ExpressionNode::Sign::Negative) {
      return replaceWithUndefinedInPlace();
    }
  }
  if (c0.type() != ExpressionNode::Type::Rational || c1.type() != ExpressionNode::Type::Rational) {
    return *this;
  }
  Rational r0 = static_cast<Rational &>(c0);
  Rational r1 = static_cast<Rational &>(c1);

  Integer n = r0.unsignedIntegerNumerator();
  Integer k = r1.unsignedIntegerNumerator();
  if (n.isLowerThan(k)) {
    Expression result = Rational::Builder(0);
    replaceWithInPlace(result);
    return result;
  }
  /* if n is too big, we do not reduce to avoid too long computation.
   * The permute coefficient will be evaluate approximatively later */
  if (Integer(k_maxNValue).isLowerThan(n)) {
    return *this;
  }
  Integer result(1);
  int clippedK = k.extractedInt(); // Authorized because k < n < k_maxNValue
  for (int i = 0; i < clippedK; i++) {
    Integer factor = Integer::Subtraction(n, Integer(i));
    result = Integer::Multiplication(result, factor);
  }
  assert(!result.isOverflow()); // < permute(k_maxNValue, k_maxNValue-1)~10^158
  Expression rationalResult = Rational::Builder(result);
  replaceWithInPlace(rationalResult);
  return rationalResult;

}

}
