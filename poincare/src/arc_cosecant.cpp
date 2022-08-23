#include <poincare/arc_sine.h>
#include <poincare/complex.h>
#include <poincare/arc_cosecant.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <poincare/trigonometry.h>

#include <cmath>

namespace Poincare {

int ArcCosecantNode::numberOfChildren() const { return ArcCosecant::s_functionHelper.numberOfChildren(); }

template<typename T>
Complex<T> ArcCosecantNode::computeOnComplex(const std::complex<T> c, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit) {
  if (c == static_cast<T>(0.0)) {
    return Complex<T>::Undefined();
  }
  return ArcSineNode::computeOnComplex<T>(std::complex<T>(1) / c, complexFormat, angleUnit);
}

Layout ArcCosecantNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Prefix(ArcCosecant(this), floatDisplayMode, numberOfSignificantDigits, ArcCosecant::s_functionHelper.name());
}

int ArcCosecantNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, ArcCosecant::s_functionHelper.name());
}

Expression ArcCosecantNode::shallowReduce(ReductionContext reductionContext) {
  return ArcCosecant(this).shallowReduce(reductionContext);
}

Expression ArcCosecant::shallowReduce(ExpressionNode::ReductionContext reductionContext) {
  {
    Expression e = SimplificationHelper::defaultShallowReduce(*this);
    if (!e.isUninitialized()) {
      return e;
    }
  }
  return Trigonometry::shallowReduceInverseAdvancedFunction(*this, reductionContext);
}


}
