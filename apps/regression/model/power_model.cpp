#include "power_model.h"
#include "../store.h"
#include <cmath>
#include <assert.h>
#include <poincare/code_point_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/print.h>
#include <poincare/vertical_offset_layout.h>

using namespace Poincare;

namespace Regression {

Layout PowerModel::layout() {
  if (m_layout.isUninitialized()) {
    m_layout = HorizontalLayout::Builder({
      CodePointLayout::Builder('a', k_layoutFont),
      CodePointLayout::Builder(UCodePointMiddleDot, k_layoutFont),
      CodePointLayout::Builder('X', k_layoutFont),
      VerticalOffsetLayout::Builder(
        CodePointLayout::Builder('b', k_layoutFont),
        VerticalOffsetLayoutNode::Position::Superscript
      ),
    });
  }
  return m_layout;
}

int PowerModel::buildEquationTemplate(char * buffer, size_t bufferSize, double * modelCoefficients, int significantDigits, Poincare::Preferences::PrintFloatMode displayMode) const {
  return Poincare::Print::safeCustomPrintf(buffer, bufferSize, "%*.*ed·x^%*.*ed",
      modelCoefficients[0], displayMode, significantDigits,
      modelCoefficients[1], displayMode, significantDigits);
}

double PowerModel::evaluate(double * modelCoefficients, double x) const {
  double a = modelCoefficients[0];
  double b = modelCoefficients[1];
  return a*std::pow(x,b);
}

double PowerModel::levelSet(double * modelCoefficients, double xMin, double xMax, double y, Poincare::Context * context) {
  double a = modelCoefficients[0];
  double b = modelCoefficients[1];
  if (a == 0 || b == 0|| y/a <= 0) {
    return NAN;
  }
  return std::exp(std::log(y/a)/b);
}

double PowerModel::partialDerivate(double * modelCoefficients, int derivateCoefficientIndex, double x) const {
  double a = modelCoefficients[0];
  double b = modelCoefficients[1];
  if (derivateCoefficientIndex == 0) {
    // Derivate with respect to a: pow(x,b)
    return std::pow(x,b);
  }
  assert(derivateCoefficientIndex == 1);
  assert(x >= 0);
  /* We assume all xi are positive.
   * For x = 0, a*pow(x,b) = 0, the partial derivate with respect to b is 0
   * For x > 0, a*pow(x,b) = a*exp(b*ln(x)), the partial derivate with respect
   *            to b is ln(x)*a*pow(x,b) */
  return x == 0.0 ? 0.0 : std::log(x) * a * std::pow(x, b);
}

void PowerModel::privateFit(Store * store, int series, double * modelCoefficients, Poincare::Context * context) {
  /* Y1 = aX1^b => ln(Y1) = ln(a) + b*ln(X1)*/
  modelCoefficients[0] = std::exp(store->yIntercept(series, true));
  modelCoefficients[1] = store->slope(series, true);
}

bool PowerModel::dataSuitableForFit(Store * store, int series) const {
  if (!Model::dataSuitableForFit(store, series)) {
    return false;
  }
  int numberOfPairs = store->numberOfPairsOfSeries(series);
  for (int j = 0; j < numberOfPairs; j++) {
    if (store->get(series, 0, j) < 0) {
      return false;
    }
  }
  return true;
}


}
