#include "store.h"

#include <assert.h>
#include <float.h>
#include <poincare/preferences.h>
#include <poincare/print_int.h>
#include <poincare/symbol.h>
#include <string.h>

#include <algorithm>
#include <array>
#include <cmath>

#include "app.h"

using namespace Poincare;
using namespace Shared;

namespace Regression {

static_assert(Store::k_numberOfSeries == 3,
              "Number of series changed, Regression::Store() needs to adapt "
              "(m_seriesChecksum)");

const char *Store::SeriesTitle(int series) {
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

Store::Store(Shared::GlobalContext *context,
             DoublePairStorePreferences *preferences,
             Model::Type *regressionTypes)
    : LinearRegressionStore(context, preferences),
      m_regressionTypes(regressionTypes),
      m_exponentialAbxModel(true),
      m_linearApbxModel(true),
      m_recomputeCoefficients{true, true, true} {
  initListsFromStorage();
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
    m_recomputeCoefficients[series] = true;
  }
}

/* Dots */

int Store::closestVerticalDot(OMG::VerticalDirection direction, double x,
                              double y, int currentSeries, int currentDot,
                              int *nextSeries, Context *globalContext) {
  double nextX = INFINITY;
  double nextY = INFINITY;
  int nextDot = -1;
  for (int series = 0; series < k_numberOfSeries; series++) {
    if (!seriesIsActive(series) ||
        (currentDot >= 0 && currentSeries == series)) {
      /* If the currentDot is valid, the next series should not be the current
       * series */
      continue;
    }
    int numberOfDots = numberOfPairsOfSeries(series);
    float xMin = App::app()->graphRange()->xMin();
    float xMax = App::app()->graphRange()->xMax();
    bool displayMean = seriesSatisfies(series, HasCoefficients);
    for (int i = 0; i < numberOfDots + displayMean; i++) {
      double currentX =
          i < numberOfDots ? get(series, 0, i) : meanOfColumn(series, 0);
      double currentY =
          i < numberOfDots ? get(series, 1, i) : meanOfColumn(series, 1);
      if (xMin <= currentX &&
          // The next dot is within the window abscissa bounds
          currentX <= xMax &&
          // The next dot is the closest to x in abscissa
          (std::fabs(currentX - x) <= std::fabs(nextX - x)) &&
          // The next dot is above/under y
          ((currentY > y && direction.isUp()) ||
           (currentY < y && direction.isDown()) ||
           (currentY == y &&
            ((currentDot < 0 && direction.isUp()) ||
             ((direction.isDown()) == (series > currentSeries))))) &&
          // Edge case: if 2 dots have the same abscissa but different ordinates
          (nextX != currentX || ((currentY <= nextY) == (direction.isUp())))) {
        nextX = currentX;
        nextY = currentY;
        nextDot = i;
        *nextSeries = series;
      }
    }
  }
  return nextDot;
}

int Store::nextDot(int series, OMG::HorizontalDirection direction, int dot,
                   bool displayMean) {
  double nextX = INFINITY;
  int nextDot = -1;
  double meanX = meanOfColumn(series, 0);
  double x = meanX;
  if (dot >= 0 && dot < numberOfPairsOfSeries(series)) {
    x = get(series, 0, dot);
  }
  /* We have to scan the Store in opposite ways for the 2 directions to ensure
   * to select all dots (even with equal abscissa) */
  if (direction.isRight()) {
    for (int index = 0; index < numberOfPairsOfSeries(series); index++) {
      double data = get(series, 0, index);
      /* The conditions to test are in this order:
       * - the next dot is the closest one in abscissa to x
       * - the next dot is not the same as the selected one
       * - the next dot is at the right of the selected one */
      if (std::fabs(data - x) < std::fabs(nextX - x) && (index != dot) &&
          (data >= x)) {
        // Handle edge case: 2 dots have same abscissa
        if (data != x || (index > dot)) {
          nextX = data;
          nextDot = index;
        }
      }
    }
    // Compare with the mean dot
    if (displayMean && std::fabs(meanX - x) < std::fabs(nextX - x) &&
        (numberOfPairsOfSeries(series) != dot) && (meanX >= x)) {
      if (meanX != x || (numberOfPairsOfSeries(series) > dot)) {
        nextDot = numberOfPairsOfSeries(series);
      }
    }
  } else {
    assert(direction.isLeft());
    // Compare with the mean dot
    if (displayMean && std::fabs(meanX - x) < std::fabs(nextX - x) &&
        (numberOfPairsOfSeries(series) != dot) && (meanX <= x)) {
      if ((meanX != x) || (numberOfPairsOfSeries(series) < dot)) {
        nextX = meanX;
        nextDot = numberOfPairsOfSeries(series);
      }
    }
    for (int index = numberOfPairsOfSeries(series) - 1; index >= 0; index--) {
      double data = get(series, 0, index);
      if (std::fabs(data - x) < std::fabs(nextX - x) && (index != dot) &&
          (data <= x)) {
        // Handle edge case: 2 dots have same abscissa
        if (data != x || (index < dot)) {
          nextX = data;
          nextDot = index;
        }
      }
    }
  }
  return nextDot;
}

/* Series */

void Store::updateSeriesValidity(int series) {
  LinearRegressionStore::updateSeriesValidity(series);
  if (!seriesIsValid(series)) {
    // Reset series regression type to None
    m_regressionTypes[series] = Model::Type::None;
    deleteRegressionFunction(series);
  }
}

bool Store::updateSeries(int series, bool delayUpdate) {
  m_recomputeCoefficients[series] = true;
  return DoublePairStore::updateSeries(series, delayUpdate);
}

/* Calculations */

void Store::updateCoefficients(int series, Context *globalContext) {
  assert(series >= 0 && series <= k_numberOfSeries);
  assert(seriesIsActive(series));
  if (!m_recomputeCoefficients[series]) {
    return;
  }

  Model *seriesModel = modelForSeries(series);
  seriesModel->fit(this, series, m_regressionCoefficients[series],
                   globalContext);
  m_recomputeCoefficients[series] = false;
  storeRegressionFunction(
      series, seriesModel->expression(m_regressionCoefficients[series]));

  if (!coefficientsAreDefined(series, globalContext)) {
    m_determinationCoefficient[series] = NAN;
    m_residualStandardDeviation[series] = NAN;
    return;
  }

  /* m_determinationCoefficient and m_residualStandardDeviation must be
   * updated after m_recomputeCoefficients updates to avoid infinite recursive
   * calls as computeDeterminationCoefficient and residualStandardDeviation
   * call yValueForXValue which calls coefficientsForSeries which calls
   * updateCoefficients */
  m_determinationCoefficient[series] =
      computeDeterminationCoefficient(series, globalContext);
  m_residualStandardDeviation[series] =
      computeResidualStandardDeviation(series, globalContext);
}

double *Store::coefficientsForSeries(int series, Context *globalContext) {
  updateCoefficients(series, globalContext);
  return m_regressionCoefficients[series];
}

bool Store::coefficientsAreDefined(int series, Context *globalContext,
                                   bool finite) {
  double *coefficients = coefficientsForSeries(series, globalContext);
  int numberOfCoefficients = modelForSeries(series)->numberOfCoefficients();
  for (int i = 0; i < numberOfCoefficients; i++) {
    if (std::isnan(coefficients[i]) ||
        (finite && !std::isfinite(coefficients[i]))) {
      return false;
    }
  }
  return true;
}

double Store::correlationCoefficient(int series) const {
  /* Returns the correlation coefficient (R) between the series X and Y
   * (transformed if series is a TransformedModel). In non-linear and
   * non-transformed regressions, its square is different from the
   * determinationCoefficient R2. it is then hidden to avoid confusion */
  if (!seriesSatisfies(series, DisplayR)) {
    return NAN;
  }
  bool applyLn = seriesSatisfies(series, FitsLnY);
  bool applyOpposite = applyLn && get(series, 1, 0) < 0.0;
  Shared::DoublePairStore::CalculationOptions options(
      seriesSatisfies(series, FitsLnX), applyLn, applyOpposite);
  double v0 = varianceOfColumn(series, 0, options);
  double v1 = varianceOfColumn(series, 1, options);
  if (std::isnan(v0) || std::isnan(v1)) {
    // Can happen if applyLn on negative/null values
    return NAN;
  }
  /* Compare v0 and v1 to EpsilonLax to check if they are equal to zero (since
   * approximation errors could give them > 0 while they are not.)*/
  double result = (std::abs(v0) < Float<double>::EpsilonLax() ||
                   std::abs(v1) < Float<double>::EpsilonLax())
                      ? 1.0
                      : covariance(series, options) / std::sqrt(v0 * v1);
  /* Due to errors, coefficient could slightly exceed 1.0. It needs to be
   * fixed here to prevent r^2 from being bigger than 1. */
  if (std::abs(result) <= 1.0 + Float<double>::SqrtEpsilonLax()) {
    return std::clamp(result, -1.0, 1.0);
  }
  return NAN;
}

double Store::determinationCoefficientForSeries(int series,
                                                Context *globalContext) {
  /* Returns the Determination coefficient (R2).
   * It will be updated if the regression has been updated */
  updateCoefficients(series, globalContext);
  return m_determinationCoefficient[series];
}

double Store::residualStandardDeviation(int series, Context *globalContext) {
  updateCoefficients(series, globalContext);
  return m_residualStandardDeviation[series];
}

void Store::resetMemoization() {
  static_assert(static_cast<int>(Model::Type::None) == 0,
                "None type should be default at 0");
  memset(m_regressionTypes, 0, sizeof(Model::Type) * Store::k_numberOfSeries);
  memset(m_recomputeCoefficients, 0, sizeof(m_recomputeCoefficients));
}

float Store::maxValueOfColumn(int series, int i) const {
  float maxColumn = -FLT_MAX;
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    maxColumn = std::max<float>(maxColumn, get(series, i, k));
  }
  return maxColumn;
}

float Store::minValueOfColumn(int series, int i) const {
  float minColumn = FLT_MAX;
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    minColumn = std::min<float>(minColumn, get(series, i, k));
  }
  return minColumn;
}

double Store::yValueForXValue(int series, double x, Context *globalContext) {
  Model *model = regressionModel(m_regressionTypes[series]);
  double *coefficients = coefficientsForSeries(series, globalContext);
  return model->evaluate(coefficients, x);
}

double Store::xValueForYValue(int series, double y, Context *globalContext) {
  Model *model = regressionModel(m_regressionTypes[series]);
  double *coefficients = coefficientsForSeries(series, globalContext);
  return model->levelSet(coefficients, App::app()->graphRange()->xMin(),
                         App::app()->graphRange()->xMax(), y, globalContext);
}

double Store::residualAtIndexForSeries(int series, int index,
                                       Context *globalContext) {
  double x = get(series, 0, index);
  return get(series, 1, index) - yValueForXValue(series, x, globalContext);
}

bool Store::seriesNumberOfAbscissaeGreaterOrEqualTo(int series, int i) const {
  assert(series >= 0 && series < k_numberOfSeries);
  int count = 0;
  for (int j = 0; j < numberOfPairsOfSeries(series); j++) {
    if (count >= i) {
      return true;
    }
    double currentAbscissa = get(series, 0, j);
    bool firstOccurrence = true;
    for (int k = 0; k < j; k++) {
      if (get(series, 0, k) == currentAbscissa) {
        firstOccurrence = false;
        break;
      }
    }
    if (firstOccurrence) {
      count++;
    }
  }
  return count >= i;
}

bool Store::AnyActiveSeriesSatisfies(TypeProperty property) const {
  int numberOfDefinedSeries = numberOfActiveSeries();
  for (int i = 0; i < numberOfDefinedSeries; i++) {
    if (seriesSatisfies(seriesIndexFromActiveSeriesIndex(i), property)) {
      return true;
    }
  }
  return false;
}

double Store::computeDeterminationCoefficient(int series,
                                              Context *globalContext) {
  // Computes and returns the determination coefficient of the regression.
  if (seriesSatisfies(series, DisplayRSquared)) {
    /* With linear regressions and transformed models (Exponential, Logarithm
     * and Power), we use r^2, the square of the correlation coefficient between
     * the series Y (transformed) and the evaluated values.*/
    double r = correlationCoefficient(series);
    return r * r;
  }
  if (!seriesSatisfies(series, DisplayR2)) {
    /* R2 does not need to be computed if model is median-median, so we avoid
     * computation. If needed, it could be computed though. */
    return NAN;
  }
  assert(!seriesSatisfies(series, FitsLnY) &&
         !seriesSatisfies(series, FitsLnX));
  /* With proportional regression or badly fitted models, R2 can technically be
   * negative. R2<0 means that the regression is less effective than a
   * constant set to the series average. It should not happen with regression
   * models that can fit a constant observation. */
  // Residual sum of squares
  double ssr = 0;
  // Total sum of squares
  double sst = 0;
  const int numberOfPairs = numberOfPairsOfSeries(series);
  assert(numberOfPairs > 0);
  double mean = meanOfColumn(series, 1);
  for (int k = 0; k < numberOfPairs; k++) {
    // Difference between the observation and the estimated value of the model
    double estimation =
        yValueForXValue(series, get(series, 0, k), globalContext);
    double observation = get(series, 1, k);
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
  /* Check if regression fit was optimal.
   * TODO : Optimize regression fitting so that r2 cannot be negative.
   * assert(r2 >= 0 || seriesRegressionType(series) ==
   * Model::Type::Proportional); */
  return r2;
}

double Store::computeResidualStandardDeviation(int series,
                                               Context *globalContext) {
  int nCoeff =
      regressionModel(m_regressionTypes[series])->numberOfCoefficients();
  int n = numberOfPairsOfSeries(series);
  if (n <= nCoeff) {
    return NAN;
  }
  double sum = 0.;
  for (int i = 0; i < n; i++) {
    double res = residualAtIndexForSeries(series, i, globalContext);
    sum += res * res;
  }
  return std::sqrt(sum / (n - nCoeff));
}

Model *Store::regressionModel(int index) {
  Model *models[Model::k_numberOfModels] = {
      &m_noneModel,        &m_linearAxpbModel,      &m_proportionalModel,
      &m_quadraticModel,   &m_cubicModel,           &m_quarticModel,
      &m_logarithmicModel, &m_exponentialAebxModel, &m_exponentialAbxModel,
      &m_powerModel,       &m_trigonometricModel,   &m_logisticModel,
      &m_medianModel,      &m_linearApbxModel};
  static_assert(std::size(models) == Model::k_numberOfModels,
                "Inconsistency between the number of models in the store and "
                "the real number.");
  return models[index];
}

int Store::BuildFunctionName(int series, char *buffer, int bufferSize) {
  assert(bufferSize >= k_functionNameSize);
  size_t length = strlcpy(buffer, k_functionName, bufferSize);
  length += PrintInt::Left(1 + series, buffer + length, bufferSize - length);
  assert(length == k_functionNameSize - 1);
  buffer[length] = 0;
  return length;
}

Ion::Storage::Record Store::functionRecord(int series) const {
  char name[k_functionNameSize];
  BuildFunctionName(series, name, k_functionNameSize);
  return Ion::Storage::FileSystem::sharedFileSystem
      ->recordBaseNamedWithExtension(name, Ion::Storage::regExtension);
}

void Store::storeRegressionFunction(int series, Expression expression) const {
  if (expression.isUninitialized()) {
    return deleteRegressionFunction(series);
  }
  char name[k_functionNameSize];
  BuildFunctionName(series, name, k_functionNameSize);
  expression = expression.replaceSymbolWithExpression(
      Symbol::Builder(Model::k_xSymbol), Symbol::SystemSymbol());
  expression.storeWithNameAndExtension(name, Ion::Storage::regExtension);
}

void Store::deleteRegressionFunction(int series) const {
  Ion::Storage::Record r = functionRecord(series);
  if (!r.isNull()) {
    r.destroy();
  }
}

}  // namespace Regression
