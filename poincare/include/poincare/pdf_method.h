#ifndef POINCARE_PDF_METHOD_H
#define POINCARE_PDF_METHOD_H

#include <poincare/distribution.h>
#include <poincare/distribution_method.h>

namespace Poincare {

class PDFMethod final : public DistributionMethod {
  float EvaluateAtAbscissa(float * x, const Distribution * distribution, const float * parameters) const override {
    return distribution->EvaluateAtAbscissa(x[0], parameters);
  }

  double EvaluateAtAbscissa(double * x, const Distribution * distribution, const double * parameters) const override {
    return distribution->EvaluateAtAbscissa(x[0], parameters);
  }
};

}

#endif
