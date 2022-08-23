#ifndef PROBABILITE_FISHER_DISTRIBUTION_H
#define PROBABILITE_FISHER_DISTRIBUTION_H

#include "two_parameter_distribution.h"

namespace Inference {

class FisherDistribution final : public TwoParameterDistribution {
public:
  FisherDistribution() : TwoParameterDistribution(Poincare::Distribution::Type::Fisher, 1.0, 1.0) { computeCurveViewRange(); }
  I18n::Message title() const override { return I18n::Message::FisherDistribution; }
  Type type() const override { return Type::Fisher; }
  const char * parameterNameAtIndex(int index) const override { return index == 0 ? "d1" : "d2"; }
  bool authorizedParameterAtIndex(double x, int index) const override;
private:
  constexpr static double k_maxParameter = 144.0; // The display works badly for d1 = d2 > 144.
  constexpr static float k_defaultMax = 3.0f;
  enum ParamsOrder { D1, D2 };
  ParameterRepresentation paramRepresentationAtIndex(int i) const override;
  float mode() const;
  float computeXMax() const override;
  float computeYMax() const override;
};

}

#endif
