#include "exponential_model.h"
#include "../store.h"
#include "../linear_model_helper.h"
#include <math.h>
#include <assert.h>
#include <poincare/code_point_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/vertical_offset_layout.h>

using namespace Poincare;

namespace Regression {

ExponentialModel::ExponentialModel(bool abxForm) : m_abxForm(abxForm) {
}

Layout ExponentialModel::layout() {
  if (m_layout.isUninitialized()) {
    Layout exponent;
    if (m_abxForm) {
      exponent = CodePointLayout::Builder('X', k_layoutFont);
    } else {
      exponent = HorizontalLayout::Builder({
        CodePointLayout::Builder('b', k_layoutFont),
        CodePointLayout::Builder(UCodePointMiddleDot, k_layoutFont),
        CodePointLayout::Builder('X', k_layoutFont)
        });
    }
    m_layout = HorizontalLayout::Builder({
      CodePointLayout::Builder('a', k_layoutFont),
      CodePointLayout::Builder(UCodePointMiddleDot, k_layoutFont),
      CodePointLayout::Builder(m_abxForm ? 'b' : 'e', k_layoutFont),
      VerticalOffsetLayout::Builder(
        exponent,
        VerticalOffsetLayoutNode::Position::Superscript
      )
    });
  }
  return m_layout;
}

double ExponentialModel::evaluate(double * modelCoefficients, double x) const {
  double a = modelCoefficients[0];
  double b = modelCoefficients[1];
  if (m_abxForm) {
    b = log(b);
  }
  // a*e^(bx)
  return a*exp(b*x);
}

double ExponentialModel::levelSet(double * modelCoefficients, double xMin, double xMax, double y, Poincare::Context * context) {
  double a = modelCoefficients[0];
  double b = modelCoefficients[1];
  if (m_abxForm) {
    b = log(b);
  }
  if (a == 0 || b == 0 || y/a <= 0) {
    return NAN;
  }
  return log(y/a)/b;
}

void ExponentialModel::specializedInitCoefficientsForFit(double * modelCoefficients, double defaultValue, Store * store, int series) const {
  assert(store != nullptr && series >= 0 && series < Store::k_numberOfSeries && store->seriesIsValid(series));
  /* We try a better initialization than the default value. We hope that this
   * will improve the gradient descent to find correct coefficients.
   *
   * By the change of variable z=ln(y), the equation y=a*exp(b*x) becomes
   * z=c*x+d with c=b and d=ln(a). That change of variable does not preserve the
   * regression error function, so it isn't the best regression.
   * However, it turns this exponential regression problem into a linear one,
   * for which the solution is set of initial coefficients for a fit.
   * That being said, one should check that the y values are all positive. (If
   * the y values are all negative, one may replace each of them by its
   * opposite. In the case where y values happen to be zero or of opposite
   * sign, we call the base class method as a fallback. */
  double sumOfX = 0;
  double sumOfY = 0;
  double sumOfXX = 0;
  double sumOfXY = 0;
  const int numberOfPoints = store->numberOfPairsOfSeries(series);
  const int sign = store->get(series, 1, 0) > 0 ? 1 : -1;
  for (int p = 0; p < numberOfPoints; p++) {
    const double x = store->get(series, 0, p);
    const double z = store->get(series, 1, p) * sign;
    if (z <= 0) {
      return Model::specializedInitCoefficientsForFit(modelCoefficients, defaultValue, store, series);
    }
    const double y = log(z);
    sumOfX += x;
    sumOfY += y;
    sumOfXX += x*x;
    sumOfXY += x*y;
  }
  const double meanOfX = sumOfX / numberOfPoints;
  const double meanOfY = sumOfY / numberOfPoints;
  const double meanOfXX = sumOfXX / numberOfPoints;
  const double meanOfXY = sumOfXY / numberOfPoints;
  const double variance = meanOfXX - meanOfX * meanOfX;
  const double covariance = meanOfXY - meanOfX * meanOfY;
  const double b = LinearModelHelper::Slope(covariance, variance);
  modelCoefficients[1] = m_abxForm ? exp(b) : b;
  modelCoefficients[0] =
    sign * exp(LinearModelHelper::YIntercept(meanOfY, meanOfX, b));
}

double ExponentialModel::partialDerivate(double * modelCoefficients, int derivateCoefficientIndex, double x) const {
  double b = m_abxForm ? log(modelCoefficients[1]) : modelCoefficients[1];
  if (m_abxForm) {
    b = log(b);
  }
  if (derivateCoefficientIndex == 0) {
    // Derivate with respect to a: exp(b*x)
    return exp(b*x);
  }
  assert(derivateCoefficientIndex == 1);
  // Derivate with respect to b: a*x*exp(b*x)
  double a = modelCoefficients[0];
  return a*x*exp(b*x);
}

}
