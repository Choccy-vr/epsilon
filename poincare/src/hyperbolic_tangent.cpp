#include <poincare/derivative.h>
#include <poincare/hyperbolic_cosine.h>
#include <poincare/hyperbolic_tangent.h>
#include <poincare/layout_helper.h>
#include <poincare/power.h>
#include <poincare/serialization_helper.h>

namespace Poincare {

Layout HyperbolicTangentNode::createLayout(
    Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits,
    Context* context) const {
  return LayoutHelper::Prefix(
      HyperbolicTangent(this), floatDisplayMode, numberOfSignificantDigits,
      HyperbolicTangent::s_functionHelper.aliasesList().mainAlias(), context);
}

size_t HyperbolicTangentNode::serialize(
    char* buffer, size_t bufferSize,
    Preferences::PrintFloatMode floatDisplayMode,
    int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(
      this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits,
      HyperbolicTangent::s_functionHelper.aliasesList().mainAlias());
}

template <typename T>
std::complex<T> HyperbolicTangentNode::computeOnComplex(
    const std::complex<T> c, Preferences::ComplexFormat,
    Preferences::AngleUnit angleUnit) {
  return ApproximationHelper::NeglectRealOrImaginaryPartIfNeglectable(
      std::tanh(c), c);
}

bool HyperbolicTangentNode::derivate(const ReductionContext& reductionContext,
                                     Symbol symbol, Expression symbolValue) {
  return HyperbolicTangent(this).derivate(reductionContext, symbol,
                                          symbolValue);
}

Expression HyperbolicTangentNode::unaryFunctionDifferential(
    const ReductionContext& reductionContext) {
  return HyperbolicTangent(this).unaryFunctionDifferential(reductionContext);
}

bool HyperbolicTangent::derivate(const ReductionContext& reductionContext,
                                 Symbol symbol, Expression symbolValue) {
  Derivative::DerivateUnaryFunction(*this, symbol, symbolValue,
                                    reductionContext);
  return true;
}

Expression HyperbolicTangent::unaryFunctionDifferential(
    const ReductionContext& reductionContext) {
  return Power::Builder(HyperbolicCosine::Builder(childAtIndex(0).clone()),
                        Rational::Builder(-2));
}

template std::complex<float> Poincare::HyperbolicTangentNode::computeOnComplex<
    float>(std::complex<float>, Preferences::ComplexFormat,
           Preferences::AngleUnit);
template std::complex<double> Poincare::HyperbolicTangentNode::computeOnComplex<
    double>(std::complex<double>, Preferences::ComplexFormat complexFormat,
            Preferences::AngleUnit);

}  // namespace Poincare
