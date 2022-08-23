#ifndef POINCARE_CONTINUOUS_DISTRIBUTION_H
#define POINCARE_CONTINUOUS_DISTRIBUTION_H

#include <poincare/distribution.h>

namespace Poincare {

class ContinuousDistribution : public Distribution {
public:
  bool isContinuous() const override { return true; }

  // The range is inclusive on both ends
  float CumulativeDistributiveFunctionForRange(float x, float y, const float * parameters) const override {
    if (y <= x) {
      return 0.0f;
    }
    return CumulativeDistributiveFunctionAtAbscissa(y, parameters) - CumulativeDistributiveFunctionAtAbscissa(x, parameters);
  }

  double CumulativeDistributiveFunctionForRange(double x, double y, const double * parameters) const override {
    if (y <= x) {
      return 0.0;
    }
    return CumulativeDistributiveFunctionAtAbscissa(y, parameters) - CumulativeDistributiveFunctionAtAbscissa(x, parameters);
  }
};

}

#endif
