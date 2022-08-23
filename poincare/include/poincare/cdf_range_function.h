#ifndef POINCARE_CDF_RANGE_FUNCTION_H
#define POINCARE_CDF_RANGE_FUNCTION_H

#include <poincare/distribution.h>
#include <poincare/distribution_method.h>

namespace Poincare {

class CDFRangeFunction final : public DistributionMethod {
  float EvaluateAtAbscissa(float * x, Distribution * distribution, const float * parameters) override {
    return distribution->CumulativeDistributiveFunctionForRange(x[0], x[1], parameters);
  }

  double EvaluateAtAbscissa(double * x, Distribution * distribution, const double * parameters) override {
    return distribution->CumulativeDistributiveFunctionForRange(x[0], x[1], parameters);
  }
};

}

#endif
