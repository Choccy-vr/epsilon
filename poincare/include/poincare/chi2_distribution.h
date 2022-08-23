#ifndef POINCARE_CHI2_DISTRIBUTION_H
#define POINCARE_CHI2_DISTRIBUTION_H

#include <poincare/distribution.h>
#include <float.h>

namespace Poincare {

class Chi2Distribution final : Distribution {
public:
  template <typename T> static T EvaluateAtAbscissa(T x, T k);
  float EvaluateAtAbscissa(float x, const float * parameters) override { return EvaluateAtAbscissa<float>(x, parameters[0]); }
  double EvaluateAtAbscissa(double x, const double * parameters) override { return EvaluateAtAbscissa<double>(x, parameters[0]); }

  template <typename T> static T CumulativeDistributiveFunctionAtAbscissa(T x, T k);
  float CumulativeDistributiveFunctionAtAbscissa(float x, const float * parameters) override { return CumulativeDistributiveFunctionAtAbscissa<float>(x, parameters[0]); }
  double CumulativeDistributiveFunctionAtAbscissa(double x, const double * parameters) override { return CumulativeDistributiveFunctionAtAbscissa<double>(x, parameters[0]); }

  template <typename T> static T CumulativeDistributiveInverseForProbability(T probability, T k);
  float CumulativeDistributiveInverseForProbability(float x, const float * parameters) override { return CumulativeDistributiveInverseForProbability<float>(x, parameters[0]); }
  double CumulativeDistributiveInverseForProbability(double x, const double * parameters) override { return CumulativeDistributiveInverseForProbability<double>(x, parameters[0]); }

  bool ParametersAreOK(const float * parameters) override { return false; }
  bool ParametersAreOK(const double * parameters) override { return false; }

  bool ExpressionParametersAreOK(bool * result, const Expression * parameters, Context * context) override { return false; }
private:
  static constexpr int k_maxRegularizedGammaIterations = 1000;
  static constexpr double k_regularizedGammaPrecision = DBL_EPSILON;
};

}

#endif
