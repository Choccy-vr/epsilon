#ifndef PROBABILITY_MODELS_STATISTIC_ONE_MEAN_T_INTERVAL_H
#define PROBABILITY_MODELS_STATISTIC_ONE_MEAN_T_INTERVAL_H

#include "interfaces/distributions.h"
#include "interfaces/significance_tests.h"
#include "interval.h"

namespace Probability {

class OneMeanTInterval : public Interval {
public:
  SignificanceTestType significanceTestType() const override { return SignificanceTestType::OneMean; }
  DistributionType distributionType() const override { return DistributionType::T; }
  I18n::Message title() const override { return OneMean::TTitle(); }

  // Significance Test: One Mean
  bool initializeDistribution(DistributionType distributionType) override { return OneMean::IntervalInitializeDistribution(this, distributionType); }
  int numberOfAvailableDistributions() const override { return OneMean::NumberOfAvailableDistributions(); }
  I18n::Message distributionTitle() const override { return OneMean::DistributionTitle(); }
  I18n::Message distributionDescription() const override { return OneMean::IntervalDistributionDescription(); }
  void initParameters() override { OneMean::InitTIntervalParameters(this); }
  bool authorizedParameterAtIndex(double p, int i) const override { return OneMean::TAuthorizedParameterAtIndex(i, p); }
  void setParameterAtIndex(double p, int index) override {
    p = OneMean::ProcessParamaterForIndex(p, index);
    Interval::setParameterAtIndex(p, index);
  }

  void compute() override { OneMean::ComputeTInterval(this); }

  const char * estimateSymbol() const override { return OneMean::EstimateSymbol(); }
  Poincare::Layout testCriticalValueSymbol(const KDFont * font = KDFont::LargeFont) override { return DistributionT::TestCriticalValueSymbol(font); }

  // Distribution: t
  float canonicalDensityFunction(float x) const override { return DistributionT::CanonicalDensityFunction(x, m_degreesOfFreedom); }
  double cumulativeDistributiveFunctionAtAbscissa(double x) const override { return DistributionT::CumulativeNormalizedDistributionFunction(x, m_degreesOfFreedom); }
  double cumulativeDistributiveInverseForProbability(double * p) override { return DistributionT::CumulativeNormalizedInverseDistributionFunction(*p, m_degreesOfFreedom); }

private:
  // Significance Test:: OneMean
  int numberOfStatisticParameters() const override { return OneMean::NumberOfParameters(); }
  ParameterRepresentation paramRepresentationAtIndex(int i) const override { return OneMean::TParameterRepresentationAtIndex(i); }
  double * parametersArray() override { return m_params; }

  // Distribution: t
  float computeYMax() const override { return DistributionT::YMax(m_degreesOfFreedom); }

  double m_params[OneMean::k_numberOfParams];
};

}  // namespace Probability

#endif /* PROBABILITY_MODELS_STATISTIC_ONE_MEAN_T_INTERVAL_H */
