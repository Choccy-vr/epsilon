#include "store.h"
#include <poincare/preferences.h>
#include <assert.h>
#include <float.h>
#include <cmath>
#include <string.h>
#include <algorithm>

using namespace Shared;

namespace Regression {

static_assert(Store::k_numberOfSeries == 3, "Number of series changed, Regression::Store() needs to adapt (m_seriesChecksum)");

const char * Store::SeriesTitle(int series) {
  /* Controller titles for menus targetting a specific series. These cannot
   * live on the stack and must be const char *. */
  switch (series) {
  case 0:
    return "X1/Y1";
  case 1:
    return "X2/Y2";
  default:
    assert(series == 2);
    return "X3/Y3";
  }
}

Store::Store(Shared::GlobalContext * context, Model::Type * regressionTypes) :
  InteractiveCurveViewRange(),
  LinearRegressionStore(context),
  m_regressionTypes(regressionTypes),
  m_exponentialAbxModel(true)
{
  initListsInPool();
}

void Store::reset() {
  deleteAllPairs();
  resetMemoization();
}

/* Regressions */
void Store::setSeriesRegressionType(int series, Model::Type type) {
  assert(series >= 0 && series < k_numberOfSeries);
  if (m_regressionTypes[series] != type) {
    m_regressionTypes[series] = type;
    m_regressionChanged[series] = true;
  }
}

/* Dots */

int Store::closestVerticalDot(int direction, double x, double y, int currentSeries, int currentDot, int * nextSeries, Poincare::Context * globalContext) {
  double nextX = INFINITY;
  double nextY = INFINITY;
  int selectedDot = -1;
  for (int series = 0; series < k_numberOfSeries; series++) {
    if (!seriesIsValid(series) || (currentDot >= 0 && currentSeries == series)) {
      /* If the currentDot is valid, the next series should not be the current
       * series */
      continue;
    }
    int numberOfPoints = numberOfPairsOfSeries(series);
    bool displayMean = seriesRegressionType(series) != Model::Type::None;
    for (int i = 0; i < numberOfPoints + displayMean; i++) {
      double currentX = i < numberOfPoints ? get(series, 0, i) : meanOfColumn(series, 0);
      double currentY = i < numberOfPoints ? get(series, 1, i) : meanOfColumn(series, 1);
      if (xMin() <= currentX && currentX <= xMax() // The next dot is within the window abscissa bounds
          && (std::fabs(currentX - x) <= std::fabs(nextX - x)) // The next dot is the closest to x in abscissa
          && ((currentY > y && direction > 0) // The next dot is above/under y
            || (currentY < y && direction < 0)
            || (currentY == y
              && ((currentDot < 0 && direction > 0)|| ((direction < 0) == (series > currentSeries)))))
          && (nextX != currentX // Edge case: if 2 dots have the same abscissa but different ordinates
            || ((currentY <= nextY) == (direction > 0))))
      {
        nextX = currentX;
        nextY = currentY;
        selectedDot = i;
        *nextSeries = series;
      }
    }
  }
  return selectedDot;
}

int Store::nextDot(int series, int direction, int dot, bool displayMean) {
  double nextX = INFINITY;
  int selectedDot = -1;
  double meanX = meanOfColumn(series, 0);
  double x = meanX;
  if (dot >= 0 && dot < numberOfPairsOfSeries(series)) {
    x = get(series, 0, dot);
  }
  /* We have to scan the Store in opposite ways for the 2 directions to ensure to
   * select all dots (even with equal abscissa) */
  if (direction > 0) {
    for (int index = 0; index < numberOfPairsOfSeries(series); index++) {
      double data = get(series, 0, index);
      /* The conditions to test are in this order:
       * - the next dot is the closest one in abscissa to x
       * - the next dot is not the same as the selected one
       * - the next dot is at the right of the selected one */
      if (std::fabs(data - x) < std::fabs(nextX - x) &&
          (index != dot) &&
          (data >= x)) {
        // Handle edge case: 2 dots have same abscissa
        if (data != x || (index > dot)) {
          nextX = data;
          selectedDot = index;
        }
      }
    }
    // Compare with the mean dot
    if (displayMean && std::fabs(meanX - x) < std::fabs(nextX - x) &&
        (numberOfPairsOfSeries(series) != dot) &&
        (meanX >= x)) {
      if (meanX != x || (numberOfPairsOfSeries(series) > dot)) {
        selectedDot = numberOfPairsOfSeries(series);
      }
    }
  } else {
    // Compare with the mean dot
    if (displayMean && std::fabs(meanX - x) < std::fabs(nextX - x) &&
        (numberOfPairsOfSeries(series) != dot) &&
        (meanX <= x)) {
      if ((meanX != x) || (numberOfPairsOfSeries(series) < dot)) {
        nextX = meanX;
        selectedDot = numberOfPairsOfSeries(series);
      }
    }
    for (int index = numberOfPairsOfSeries(series)-1; index >= 0; index--) {
      double data = get(series, 0, index);
      if (std::fabs(data - x) < std::fabs(nextX - x) &&
          (index != dot) &&
          (data <= x)) {
        // Handle edge case: 2 dots have same abscissa
        if (data != x || (index < dot)) {
          nextX = data;
          selectedDot = index;
        }
      }
    }
  }
  return selectedDot;
}

/* Series */

void Store::updateSeriesValidity(int series) {
  LinearRegressionStore::updateSeriesValidity(series);
  if (!m_validSeries[series]) {
    // Reset series regression type to None
    m_regressionTypes[series] = Model::Type::None;
  }
}

/* Calculations */

void Store::updateCoefficients(int series, Poincare::Context * globalContext) {
  assert(series >= 0 && series <= k_numberOfSeries);
  assert(seriesIsValid(series));
  uint32_t storeChecksumSeries = storeChecksumForSeries(series);
  if (m_regressionChanged[series] || (m_seriesChecksum[series] != storeChecksumSeries)) {
    Model * seriesModel = modelForSeries(series);
    seriesModel->fit(this, series, m_regressionCoefficients[series], globalContext);
    m_regressionChanged[series] = false;
    m_seriesChecksum[series] = storeChecksumSeries;
    /* m_determinationCoefficient must be updated after m_seriesChecksum and m_regressionChanged
     * updates to avoid infinite recursive calls as computeDeterminationCoefficient calls
     * yValueForXValue which calls coefficientsForSeries which calls updateCoefficients */
    m_determinationCoefficient[series] = computeDeterminationCoefficient(series, globalContext);
  }
}

double * Store::coefficientsForSeries(int series, Poincare::Context * globalContext) {
  updateCoefficients(series, globalContext);
  return m_regressionCoefficients[series];
}

bool Store::coefficientsAreDefined(int series, Poincare::Context * globalContext) {
  double * coefficients = coefficientsForSeries(series, globalContext);
  int numberOfCoefficients = modelForSeries(series)->numberOfCoefficients();
  for (int i = 0; i < numberOfCoefficients; i++) {
    if (std::isnan(coefficients[i])) {
      return false;
    }
  }
  return true;
}

double Store::determinationCoefficientForSeries(int series, Poincare::Context * globalContext) {
  /* Returns the Determination coefficient (R2).
   * It will be updated if the regression has been updated */
  updateCoefficients(series, globalContext);
  return m_determinationCoefficient[series];
}

void Store::resetMemoization() {
  static_assert(((int)Model::Type::None) == 0, "None type should be default at 0");
  memset(m_seriesChecksum, 0, sizeof(uint32_t) * Store::k_numberOfSeries);
  memset(m_regressionTypes, 0, sizeof(Model::Type) * Store::k_numberOfSeries);
  memset(m_regressionChanged, 0, sizeof(m_regressionChanged));
}

float Store::maxValueOfColumn(int series, int i) const {
  float maxColumn = -FLT_MAX;
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    maxColumn = std::max<float>(maxColumn, get(series,i,k));
  }
  return maxColumn;
}

float Store::minValueOfColumn(int series, int i) const {
  float minColumn = FLT_MAX;
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    minColumn = std::min<float>(minColumn, get(series,i,k));
  }
  return minColumn;
}

double Store::yValueForXValue(int series, double x, Poincare::Context * globalContext) {
  Model * model = regressionModel((int)m_regressionTypes[series]);
  double * coefficients = coefficientsForSeries(series, globalContext);
  return model->evaluate(coefficients, x);
}

double Store::xValueForYValue(int series, double y, Poincare::Context * globalContext) {
  Model * model = regressionModel((int)m_regressionTypes[series]);
  double * coefficients = coefficientsForSeries(series, globalContext);
  return model->levelSet(coefficients, xMin(), xMax(), y, globalContext);
}

double Store::residualAtIndexForSeries(int series, int index, Poincare::Context * globalContext) {
  double x = get(series, 0, index);
  return get(series, 1, index) - yValueForXValue(series, x, globalContext);
}

double Store::computeDeterminationCoefficient(int series, Poincare::Context * globalContext) {
  /* Computes and returns the determination coefficient (R2) of the regression.
   * For linear regressions, it is equal to the square of the correlation
   * coefficient between the series Y and the evaluated values.
   * With proportional regression or badly fitted models, R2 can technically be
   * negative. R2<0 means that the regression is less effective than a
   * constant set to the series average. It should not happen with regression
   * models that can fit a constant observation.
   * R2 does not need to be computed if model is median-median, so we avoid computation.
   * If needed, it could be computed though.
   * */
  if (m_regressionTypes[series] == Model::Type::Median || m_regressionTypes[series] == Model::Type::None) {
    return NAN;
  }
  /* Exponential Regression are fitted with a z = ln(+-y) change of variable.
   * This change must be replicated here when computing R2. */
  bool applyLn = m_regressionTypes[series] == Model::Type::ExponentialAbx || m_regressionTypes[series] == Model::Type::ExponentialAebx;
  bool applyOpposite = applyLn && get(series, 1, 0) < 0.0;
  // Residual sum of squares
  double ssr = 0;
  // Total sum of squares
  double sst = 0;
  const int numberOfPairs = numberOfPairsOfSeries(series);
  assert(numberOfPairs > 0);
  double mean = 0.0;
  if (!applyOpposite) {
    mean = meanOfColumn(series, 1, applyLn);
  } else {
    assert(applyLn);
    // Compute the mean of ln(-y)
    for (int k = 0; k < numberOfPairs; k++) {
      mean += std::log(-get(series, 1, k));
    }
    mean /= numberOfPairs;
  }
  for (int k = 0; k < numberOfPairs; k++) {
    // Difference between the observation and the estimated value of the model
    double estimation = yValueForXValue(series, get(series, 0, k), globalContext);
    double observation = get(series, 1, k);
    if (applyOpposite) {
      estimation *= -1.0;
      observation *= -1.0;
    }
    if (applyLn) {
      estimation = std::log(estimation);
      observation = std::log(observation);
    }
    if (std::isnan(estimation) || std::isinf(estimation)) {
      // Data Not Suitable for estimation
      return NAN;
    }
    double residual = observation - estimation;
    ssr += residual * residual;
    // Difference between the observation and the overall observations mean
    double difference = observation - mean;
    sst += difference * difference;
  }
  if (sst == 0.0) {
    /* Observation was constant, r2 is undefined. Return 1 if estimations
     * exactly matched observations. 0 is usually returned otherwise. */
    return (ssr <= DBL_EPSILON) ? 1.0 : 0.0;
  }
  double r2 = 1.0 - ssr / sst;
  // Check if regression fit was optimal.
  // TODO : Optimize regression fitting so that r2 cannot be negative.
  // assert(r2 >= 0 || seriesRegressionType(series) == Model::Type::Proportional);
  return r2;
}

Model * Store::regressionModel(int index) {
  Model * models[Model::k_numberOfModels] = {&m_noneModel, &m_linearModel, &m_proportionalModel, &m_quadraticModel, &m_cubicModel, &m_quarticModel, &m_logarithmicModel, &m_exponentialAebxModel, &m_exponentialAbxModel, &m_powerModel, &m_trigonometricModel, &m_logisticModel, &m_medianModel};
  static_assert(sizeof(models) / sizeof(Model *) == Model::k_numberOfModels, "Inconsistency between the number of models in the store and the real number.");
  return models[index];
}

}
