#include <assert.h>
#include <poincare/addition.h>
#include <poincare/approximation_helper.h>
#include <poincare/constant.h>
#include <poincare/division.h>
#include <poincare/layout_helper.h>
#include <poincare/naperian_logarithm.h>
#include <poincare/nth_root.h>
#include <poincare/nth_root_layout.h>
#include <poincare/power.h>
#include <poincare/serialization_helper.h>
#include <poincare/simplification_helper.h>
#include <poincare/subtraction.h>
#include <poincare/undefined.h>

#include <cmath>
#include <utility>

namespace Poincare {

int NthRootNode::numberOfChildren() const {
  return NthRoot::s_functionHelper.numberOfChildren();
}

Layout NthRootNode::createLayout(Preferences::PrintFloatMode floatDisplayMode,
                                 int numberOfSignificantDigits,
                                 Context* context) const {
  return NthRootLayout::Builder(
      childAtIndex(0)->createLayout(floatDisplayMode, numberOfSignificantDigits,
                                    context),
      childAtIndex(1)->createLayout(floatDisplayMode, numberOfSignificantDigits,
                                    context));
}

size_t NthRootNode::serialize(char* buffer, size_t bufferSize,
                              Preferences::PrintFloatMode floatDisplayMode,
                              int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(
      this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits,
      NthRoot::s_functionHelper.aliasesList().mainAlias());
}

Expression NthRootNode::shallowReduce(
    const ReductionContext& reductionContext) {
  return NthRoot(this).shallowReduce(reductionContext);
}

template <typename T>
Evaluation<T> NthRootNode::templatedApproximate(
    const ApproximationContext& approximationContext) const {
  return ApproximationHelper::Map<T>(
      this, approximationContext,
      [](const std::complex<T>* c, int numberOfComplexes,
         Preferences::ComplexFormat complexFormat,
         Preferences::AngleUnit angleUnit, void* ctx) -> std::complex<T> {
        assert(numberOfComplexes == 2);
        std::complex<T> basec = c[0];
        std::complex<T> indexc = c[1];
        /* If the complexFormat is Real, we look for nthroot of form root(x,q)
         * with x real and q integer because they might have a real form which
         * does not correspond to the principale angle. */
        if (complexFormat == Preferences::ComplexFormat::Real &&
            indexc.imag() == 0.0 &&
            std::round(indexc.real()) == indexc.real()) {
          // root(x, q) with q integer and x real
          std::complex<T> result =
              PowerNode::computeNotPrincipalRealRootOfRationalPow(
                  basec, static_cast<T>(1.0), indexc.real());
          if (!std::isnan(result.real()) && !std::isnan(result.imag())) {
            return result;
          }
        }
        return PowerNode::computeOnComplex<T>(
            basec, std::complex<T>(1.0) / (indexc), complexFormat);
      });
}

Expression NthRoot::shallowReduce(ReductionContext reductionContext) {
  {
    Expression e = SimplificationHelper::defaultShallowReduce(
        *this, &reductionContext,
        SimplificationHelper::BooleanReduction::UndefinedOnBooleans,
        SimplificationHelper::UnitReduction::KeepUnits,
        SimplificationHelper::MatrixReduction::UndefinedOnMatrix,
        SimplificationHelper::ListReduction::DistributeOverLists);
    if (!e.isUninitialized()) {
      return e;
    }
  }
  Expression invIndex = Power::Builder(childAtIndex(1), Rational::Builder(-1));
  Power p = Power::Builder(childAtIndex(0), invIndex);
  invIndex.shallowReduce(reductionContext);
  replaceWithInPlace(p);
  return p.shallowReduce(reductionContext);
}

}  // namespace Poincare
