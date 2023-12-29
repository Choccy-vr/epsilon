#include <poincare/complex.h>
#include <poincare/layout_helper.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>

#include <algorithm>

extern "C" {
#include <math.h>
#include <string.h>
}

namespace Poincare {

int UndefinedNode::polynomialDegree(Context* context,
                                    const char* symbolName) const {
  /* Previously the return value was -1, but it was causing problems in the
  ¨* equations of type `y = piecewise(x,x>0,undefined,x<=0)` since the computed
   * yDeg here was -1 instead of 0. */
  return 0;
}

Layout UndefinedNode::createLayout(Preferences::PrintFloatMode floatDisplayMode,
                                   int numberOfSignificantDigits,
                                   Context* context) const {
  return LayoutHelper::String(Undefined::Name(), Undefined::NameSize() - 1);
}

bool UndefinedNode::derivate(const ReductionContext& reductionContext,
                             Symbol symbol, Expression symbolValue) {
  return true;
}

size_t UndefinedNode::serialize(char* buffer, size_t bufferSize,
                                Preferences::PrintFloatMode floatDisplayMode,
                                int numberOfSignificantDigits) const {
  return std::min<size_t>(strlcpy(buffer, Undefined::Name(), bufferSize),
                          bufferSize - 1);
}

template <typename T>
Evaluation<T> UndefinedNode::templatedApproximate() const {
  return Complex<T>::Undefined();
}

template Evaluation<float> UndefinedNode::templatedApproximate() const;
template Evaluation<double> UndefinedNode::templatedApproximate() const;
}  // namespace Poincare
