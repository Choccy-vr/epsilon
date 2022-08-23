#include "store.h"
#include <apps/global_preferences.h>
#include <poincare/normal_distribution.h>
#include <assert.h>
#include <algorithm>
#include <float.h>
#include <cmath>
#include <string.h>
#include <ion.h>
#include <limits.h>

using namespace Shared;

namespace Statistics {

static_assert(Store::k_numberOfSeries == 3, "The constructor of Statistics::Store should be changed");

Store::Store() :
  MemoizedCurveViewRange(),
  DoublePairStore(),
  m_barWidth(1.0),
  m_firstDrawnBarAbscissa(0.0),
  m_sortedIndex(nullptr),
  m_sortedIndexValid{false, false, false},
  m_memoizedMaxNumberOfModes(0),
  m_displayOutliers(GlobalPreferences::sharedGlobalPreferences()->outliersStatus() == CountryPreferences::OutlierDefaultVisibility::Displayed)
{
}

void Store::setSortedIndex(size_t * buffer, size_t bufferSize) {
  assert(k_numberOfSeries * k_maxNumberOfPairs <= bufferSize);
  m_sortedIndex = buffer;
}

void Store::invalidateSortedIndexes() {
  for (int i = 0; i < DoublePairStore::k_numberOfSeries; i++) {
    m_sortedIndexValid[i] = false;
  }
}

uint32_t Store::barChecksum() const {
  double data[2] = {m_barWidth, m_firstDrawnBarAbscissa};
  size_t dataLengthInBytes = 2*sizeof(double);
  assert((dataLengthInBytes & 0x3) == 0); // Assert that dataLengthInBytes is a multiple of 4
  return Ion::crc32Word((uint32_t *)data, dataLengthInBytes/sizeof(uint32_t));
}

/* Histogram bars */

void Store::setBarWidth(double barWidth) {
  assert(barWidth > 0.0);
  m_barWidth = barWidth;
}

double Store::heightOfBarAtIndex(int series, int index) const {
  return sumOfValuesBetween(series, startOfBarAtIndex(series, index), endOfBarAtIndex(series, index));
}

double Store::heightOfBarAtValue(int series, double value) const {
  double width = barWidth();
  int barNumber = std::floor((value - m_firstDrawnBarAbscissa)/width);
  double lowerBound = m_firstDrawnBarAbscissa + ((double)barNumber)*width;
  double upperBound = m_firstDrawnBarAbscissa + ((double)(barNumber+1))*width;
  return sumOfValuesBetween(series, lowerBound, upperBound);
}

double Store::startOfBarAtIndex(int series, int index) const {
  double firstBarAbscissa = m_firstDrawnBarAbscissa + m_barWidth*std::floor((minValue(series)- m_firstDrawnBarAbscissa)/m_barWidth);
  return firstBarAbscissa + index * m_barWidth;
}

double Store::endOfBarAtIndex(int series, int index) const {
  return startOfBarAtIndex(series, index+1);
}

double Store::numberOfBars(int series) const {
  double firstBarAbscissa = m_firstDrawnBarAbscissa + m_barWidth*std::floor((minValue(series)- m_firstDrawnBarAbscissa)/m_barWidth);
  return std::ceil((maxValue(series) - firstBarAbscissa)/m_barWidth)+1;
}

I18n::Message Store::boxPlotCalculationMessageAtIndex(int series, int index) const {
  if (index == 0) {
    return I18n::Message::Minimum;
  }
  int nbOfLowerOutliers = numberOfLowerOutliers(series);
  int nbOfUpperOutliers = numberOfUpperOutliers(series);
  if (index == nbOfLowerOutliers + k_numberOfQuantiles + nbOfUpperOutliers - 1) {
    // Handle maximum here to override UpperWhisker and Outlier messages
    return I18n::Message::Maximum;
  }
  if (index >= nbOfLowerOutliers && index < nbOfLowerOutliers + k_numberOfQuantiles) {
    return k_quantilesName[index - nbOfLowerOutliers];
  }
  assert(index < nbOfLowerOutliers + k_numberOfQuantiles + nbOfUpperOutliers);
  return I18n::Message::StatisticsBoxOutlier;
}

double Store::boxPlotCalculationAtIndex(int series, int index) const {
  assert(index >= 0);
  if (index == 0) {
    return minValue(series);
  }
  int nbOfLowerOutliers = numberOfLowerOutliers(series);
  if (index < nbOfLowerOutliers) {
    return lowerOutlierAtIndex(series, index);
  }
  index -= nbOfLowerOutliers;
  if (index < k_numberOfQuantiles) {
    return (this->*k_quantileCalculation[index])(series);
  }
  index -= k_numberOfQuantiles;
  assert(index >= 0 && index < numberOfUpperOutliers(series));
  return upperOutlierAtIndex(series, index);
}

bool Store::boxPlotCalculationIsOutlier(int series, int index) const {
  int nbOfLowerOutliers = numberOfLowerOutliers(series);
  return index < nbOfLowerOutliers || index >= nbOfLowerOutliers + k_numberOfQuantiles;
}

int Store::numberOfBoxPlotCalculations(int series) const {
  // Outliers + Lower/Upper Whisker + First/Third Quartile + Median
  return numberOfLowerOutliers(series) + k_numberOfQuantiles + numberOfUpperOutliers(series);
}

bool Store::scrollToSelectedBarIndex(int series, int index) {
  float startSelectedBar = startOfBarAtIndex(series, index);
  float windowRange = xMax() - xMin();
  float range = windowRange/(1+k_displayLeftMarginRatio+k_displayRightMarginRatio);
  if (xMin() + k_displayLeftMarginRatio*range > startSelectedBar) {
    // Only update the grid unit when setting xMax
    setHistogramXMin(startSelectedBar - k_displayLeftMarginRatio*range, false);
    setHistogramXMax(xMin() + windowRange, true);
    return true;
  }
  float endSelectedBar = endOfBarAtIndex(series, index);
  if (endSelectedBar > xMax() - k_displayRightMarginRatio*range) {
    setHistogramXMax(endSelectedBar + k_displayRightMarginRatio*range, false);
    setHistogramXMin(xMax() - windowRange, true);
    return true;
  }
  return false;
}

void Store::memoizeValidSeries(int series) {
  assert(series >= 0 && series < k_numberOfSeries);
  m_validSeries[series] = numberOfPairsOfSeries(series) > 0 && sumOfOccurrences(series) > 0;
}

bool Store::frequenciesAreInteger(int series) const {
  for (const double freq : m_data[series][1]) {
    if (std::fabs(freq - std::round(freq)) > DBL_EPSILON) {
      return false;
    }
  }
  return true;
}

/* Calculation */

double Store::sumOfOccurrences(int series) const {
  return sumOfColumn(series, 1);
}

double Store::maxValueForAllSeries(bool handleNullFrequencies) const {
  assert(DoublePairStore::k_numberOfSeries > 0);
  double result = maxValue(0, handleNullFrequencies);
  for (int i = 1; i < DoublePairStore::k_numberOfSeries; i++) {
    double maxCurrentSeries = maxValue(i, handleNullFrequencies);
    if (result < maxCurrentSeries) {
      result = maxCurrentSeries;
    }
  }
  return result;
}

double Store::minValueForAllSeries(bool handleNullFrequencies) const {
  assert(DoublePairStore::k_numberOfSeries > 0);
  double result = minValue(0, handleNullFrequencies);
  for (int i = 1; i < DoublePairStore::k_numberOfSeries; i++) {
    double minCurrentSeries = minValue(i, handleNullFrequencies);
    if (result > minCurrentSeries) {
      result = minCurrentSeries;
    }
  }
  return result;
}

double Store::maxValue(int series, bool handleNullFrequencies) const {
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = numberOfPairs - 1; k >= 0; k--) {
    // Unless handleNullFrequencies is true, look for the last non null value.
    int sortedIndex = valueIndexAtSortedIndex(series, k);
    if (handleNullFrequencies || get(series, 1, sortedIndex) > 0) {
      return get(series, 0, sortedIndex);
    }
  }
  return DBL_MIN;
}

double Store::minValue(int series, bool handleNullFrequencies) const {
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = 0; k < numberOfPairs; k++) {
    // Unless handleNullFrequencies is true, look for the first non null value.
    int sortedIndex = valueIndexAtSortedIndex(series, k);
    if (handleNullFrequencies || get(series, 1, sortedIndex) > 0) {
      return get(series, 0, sortedIndex);
    }
  }
  return DBL_MAX;
}

double Store::range(int series) const {
  return maxValue(series)-minValue(series);
}

double Store::mean(int series) const {
  return sum(series)/sumOfOccurrences(series);
}

double Store::variance(int series) const {
  /* We use the Var(X) = E[(X-E[X])^2] definition instead of Var(X) = E[X^2] - E[X]^2
   * to ensure a positive result and to minimize rounding errors */
  double m = mean(series);
  return squaredOffsettedValueSum(series, m)/sumOfOccurrences(series);
}

double Store::standardDeviation(int series) const {
  return std::sqrt(variance(series));
}

double Store::sampleStandardDeviation(int series) const {
  double n = sumOfOccurrences(series);
  double s = std::sqrt(n/(n-1.0));
  return s*standardDeviation(series);
}

/* Below is the equivalence between quartiles and cumulated population, for the
 * international definition of quartiles (as medians of the lower and upper
 * half-lists). Let N be the total population, and k an integer.
 *
 *           Data repartition   Cumulated population
 *              Q1      Q3        Q1       Q3
 *
 * N = 4k    --- --- --- ---      k        3k                --- k elements
 * N = 4k+1  --- ---O––– ---      k        3k+1               O  1 element
 * N = 4k+2  ---O--- ---O---      k+1/2    3k+1+1/2
 * N = 4k+3  ---O---O---O---      k+1/2    3k+2+1/2
 *
 * Using floor(N/2)/2 as the cumulated population for Q1 and ceil(3N/2)/2 for
 * Q3 gives the right results.
 *
 * As this method is not well defined for rational frequencies, we escape to
 * the more general definition if non-integral frequencies are found.
 * */
double Store::firstQuartile(int series) const {
  if (GlobalPreferences::sharedGlobalPreferences()->methodForQuartiles() == CountryPreferences::MethodForQuartiles::CumulatedFrequency || !frequenciesAreInteger(series)) {
    return sortedElementAtCumulatedFrequency(series, 1.0/4.0);
  }
  assert(GlobalPreferences::sharedGlobalPreferences()->methodForQuartiles() == CountryPreferences::MethodForQuartiles::MedianOfSublist);
  return sortedElementAtCumulatedPopulation(series, std::floor(sumOfOccurrences(series) / 2.) / 2., true);
}

double Store::thirdQuartile(int series) const {
  if (GlobalPreferences::sharedGlobalPreferences()->methodForQuartiles() == CountryPreferences::MethodForQuartiles::CumulatedFrequency || !frequenciesAreInteger(series)) {
    return sortedElementAtCumulatedFrequency(series, 3.0/4.0);
  }
  assert(GlobalPreferences::sharedGlobalPreferences()->methodForQuartiles() == CountryPreferences::MethodForQuartiles::MedianOfSublist);
  return sortedElementAtCumulatedPopulation(series, std::ceil(3./2. * sumOfOccurrences(series)) / 2., true);
}

double Store::quartileRange(int series) const {
  return thirdQuartile(series)-firstQuartile(series);
}

double Store::median(int series) const {
  return sortedElementAtCumulatedFrequency(series, 1.0/2.0, true);
}

double Store::lowerWhisker(int series) const {
  return get(series, 0, valueIndexAtSortedIndex(series, lowerWhiskerSortedIndex(series)));
}

double Store::upperWhisker(int series) const {
  return get(series, 0, valueIndexAtSortedIndex(series, upperWhiskerSortedIndex(series)));
}

double Store::lowerFence(int series) const {
  return firstQuartile(series) - 1.5 * quartileRange(series);
}

double Store::upperFence(int series) const {
  return thirdQuartile(series) + 1.5 * quartileRange(series);
}

int Store::numberOfLowerOutliers(int series) const {
  if (!m_displayOutliers) {
    return 0;
  }
  double value;
  int distinctValues;
  countDistinctValues(series, 0, lowerWhiskerSortedIndex(series), -1, false, &value, &distinctValues);
  return distinctValues;
}

int Store::numberOfUpperOutliers(int series) const {
  if (!m_displayOutliers) {
    return 0;
  }
  double value;
  int distinctValues;
  countDistinctValues(series, upperWhiskerSortedIndex(series) + 1, numberOfPairsOfSeries(series), -1, false, &value, &distinctValues);
  return distinctValues;
}

double Store::lowerOutlierAtIndex(int series, int index) const {
  assert(m_displayOutliers && index < numberOfLowerOutliers(series));
  double value;
  int distinctValues;
  countDistinctValues(series, 0, lowerWhiskerSortedIndex(series), index, false, &value, &distinctValues);
  return value;
}

double Store::upperOutlierAtIndex(int series, int index) const {
  assert(m_displayOutliers && index < numberOfUpperOutliers(series));
  double value;
  int distinctValues;
  countDistinctValues(series, upperWhiskerSortedIndex(series) + 1, numberOfPairsOfSeries(series), index, false, &value, &distinctValues);
  return value;
}

double Store::sum(int series) const {
  double result = 0;
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = 0; k < numberOfPairs; k++) {
    result += m_data[series][0][k]*m_data[series][1][k];
  }
  return result;
}

double Store::squaredValueSum(int series) const {
  return squaredOffsettedValueSum(series, 0.0);
}

double Store::squaredOffsettedValueSum(int series, double offset) const {
  double result = 0;
  const int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = 0; k < numberOfPairs; k++) {
    double value = m_data[series][0][k] - offset;
    result += value*value*m_data[series][1][k];
  }
  return result;
}

int Store::numberOfModes(int series) const {
  double modeFreq;
  int modesTotal;
  computeModes(series, -1, &modeFreq, &modesTotal);
  return modesTotal;
}

int Store::totalNumberOfModes() const {
  if (m_memoizedMaxNumberOfModes > 0) {
    return m_memoizedMaxNumberOfModes;
  }
  int maxNumberOfModes = 0;
  for (int i = 0; i < DoublePairStore::k_numberOfSeries; i++) {
    if (seriesIsValid(i)) {
      maxNumberOfModes = std::max(maxNumberOfModes, numberOfModes(i));
    }
  }
  m_memoizedMaxNumberOfModes = maxNumberOfModes;
  return maxNumberOfModes;
}

double Store::modeAtIndex(int series, int index) const {
  double modeFreq;
  int modesTotal;
  return computeModes(series, index, &modeFreq, &modesTotal);
}

double Store::modeFrequency(int series) const {
  double modeFreq;
  int modesTotal;
  computeModes(series, -1, &modeFreq, &modesTotal);
  return modeFreq;
}

double Store::computeModes(int series, int i, double * modeFreq, int * modesTotal) const {
  *modesTotal = 0;
  *modeFreq = DBL_MIN;
  double ithValue = NAN;
  double currentValue = NAN;
  double currentValueFrequency = NAN;
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int j = 0; j <= numberOfPairs; j++) {
    double value, valueFrequency;
    if (j < numberOfPairs) {
      int valueIndex = valueIndexAtSortedIndex(series, j);
      value = get(series, 0, valueIndex);
      valueFrequency = get(series, 1, valueIndex);
    } else {
      // Iterating one last time to process the last value
      value = valueFrequency = NAN;
    }
    // currentValue != value returns true if currentValue or value is NAN
    if (currentValue != value) {
      // A new value has been found
      if (currentValueFrequency > *modeFreq) {
        // A better mode has been found, reset solutions
        *modeFreq = currentValueFrequency;
        *modesTotal = 0;
        ithValue = NAN;
      }
      if (currentValueFrequency == *modeFreq) {
        // Another mode has been found
        if (*modesTotal == i) {
          ithValue = currentValue;
        }
        *modesTotal += 1;
      }
      currentValueFrequency = 0.0;
      currentValue = value;
    }
    currentValueFrequency += valueFrequency;
  }
  // A valid total, frequency and ithValue (if requested) have been calculated
  assert(*modesTotal > 0 && *modeFreq > 0.0 && std::isnan(ithValue) == (i == -1));
  return ithValue;
}

// TODO : Factorize it with buildSortedIndex
void Store::sortColumn(int series, int column) {
  m_sortedIndexValid[series] = false;
  DoublePairStore::sortColumn(series, column);
}

void Store::set(double f, int series, int i, int j) {
  m_sortedIndexValid[series] = false;
  m_memoizedMaxNumberOfModes = 0;
  DoublePairStore::set(f, series, i, j);
}

bool Store::deleteValueAtIndex(int series, int i, int j) {
  deletePairOfSeriesAtIndex(series, j);
  return true;
}

void Store::deletePairOfSeriesAtIndex(int series, int i) {
  m_sortedIndexValid[series] = false;
  m_memoizedMaxNumberOfModes = 0;
  DoublePairStore::deletePairOfSeriesAtIndex(series, i);
}

void Store::resetColumn(int series, int i) {
  m_sortedIndexValid[series] = false;
  m_memoizedMaxNumberOfModes = 0;
  DoublePairStore::resetColumn(series, i);
}

void Store::deleteAllPairsOfSeries(int series) {
  m_sortedIndexValid[series] = false;
  m_memoizedMaxNumberOfModes = 0;
  DoublePairStore::deleteAllPairsOfSeries(series);
}


/* Private methods */

double Store::defaultValue(int series, int i, int j) const {
  return (i == 0 && j > 1) ? 2 * m_data[series][i][j-1] - m_data[series][i][j-2] : 1.0;
}

double Store::sumOfValuesBetween(int series, double x1, double x2) const {
  double result = 0;
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = 0; k < numberOfPairs; k++) {
    int sortedIndex = valueIndexAtSortedIndex(series, k);
    double value = get(series, 0, sortedIndex);
    if (value >= x2) {
      break;
    }
    if (value >= x1) {
      result += get(series, 1, sortedIndex);
    }
  }
  return result;
}

double Store::sortedElementAtCumulatedFrequency(int series, double k, bool createMiddleElement) const {
  assert(k >= 0.0 && k <= 1.0);
  return sortedElementAtCumulatedPopulation(series, k * sumOfOccurrences(series), createMiddleElement);
}

double Store::sortedElementAtCumulatedPopulation(int series, double population, bool createMiddleElement) const {
  // Find the element after which cumulated population exceeds the population.
  int numberOfPairs = numberOfPairsOfSeries(series);
  int elementSortedIndex;
  double cumulatedPopulation = 0.0;
  for (int k = 0; k < numberOfPairs; k++) {
    elementSortedIndex = k;
    cumulatedPopulation += get(series, 1, valueIndexAtSortedIndex(series, k));
    if (cumulatedPopulation >= population-DBL_EPSILON) {
      break;
    }
  }

  if (createMiddleElement && std::fabs(cumulatedPopulation - population) < DBL_EPSILON) {
    /* There is an element of cumulated frequency population, so the result is
     * the mean between this element and the next element (in terms of cumulated
     * frequency) that has a non-null frequency. */
    for (int k = elementSortedIndex + 1; k < numberOfPairs; k++) {
      int nextElementSortedIndex = valueIndexAtSortedIndex(series, k);
      if (get(series, 1, nextElementSortedIndex) > 0.0) {
        return (get(series, 0, valueIndexAtSortedIndex(series, elementSortedIndex)) + get(series, 0, nextElementSortedIndex)) / 2.0;
      }
    }
  }

  return get(series, 0, valueIndexAtSortedIndex(series, elementSortedIndex));
}

size_t Store::lowerWhiskerSortedIndex(int series) const {
  double lowFence = lowerFence(series);
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = 0; k < numberOfPairs; k++) {
    int valueIndex = valueIndexAtSortedIndex(series, k);
    if ((!m_displayOutliers || get(series, 0, valueIndex) >= lowFence) && get(series, 1, valueIndex) > 0) {
      return k;
    }
  }
  assert(false);
  return numberOfPairs;
}

size_t Store::upperWhiskerSortedIndex(int series) const {
  double uppFence = upperFence(series);
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int k = numberOfPairs - 1; k >= 0; k--) {
    int valueIndex = valueIndexAtSortedIndex(series, k);
    if ((!m_displayOutliers || get(series, 0, valueIndex) <= uppFence) && get(series, 1, valueIndex) > 0) {
      return k;
    }
  }
  assert(false);
  return numberOfPairs;
}

void Store::countDistinctValues(int series, int start, int end, int i, bool handleNullFrequencies, double * value, int * distinctValues) const {
  assert(start >= 0 && end <= numberOfPairsOfSeries(series) && start <= end);
  *distinctValues = 0;
  *value = NAN;
  for (int j = start; j < end; j++) {
    int valueIndex = valueIndexAtSortedIndex(series, j);
    if (handleNullFrequencies || get(series, 1, valueIndex) > 0) {
      double nextX = get(series, 0, valueIndexAtSortedIndex(series, j));
      // *value != nextX returns true if *value is NAN
      if (*value != nextX) {
        (*distinctValues)++;
        *value = nextX;
      }
      if (i == (*distinctValues) - 1) {
        // Found the i-th distinct value
        return;
      }
    }
  }
  assert(i == -1);
}

int Store::totalCumulatedFrequencyValues(int series) const {
  double value;
  int distinctValues;
  countDistinctValues(series, 0, numberOfPairsOfSeries(series), -1, true, &value, &distinctValues);
  return distinctValues;
}

double Store::cumulatedFrequencyValueAtIndex(int series, int i) const {
  double value;
  int distinctValues;
  countDistinctValues(series, 0, numberOfPairsOfSeries(series), i, true, &value, &distinctValues);
  return value;
}

double Store::cumulatedFrequencyResultAtIndex(int series, int i) const {
  double cumulatedOccurrences = 0.0, otherOccurrences = 0.0;
  double value = cumulatedFrequencyValueAtIndex(series, i);
  // Recompute sumOfOccurrences() here to save some computation.
  for (int j = 0; j < numberOfPairsOfSeries(series); j++) {
    double x = get(series, 0, valueIndexAtSortedIndex(series, j));
    (x <= value ? cumulatedOccurrences : otherOccurrences) += get(series, 1, valueIndexAtSortedIndex(series, j));
  }
  assert(cumulatedOccurrences + otherOccurrences == sumOfOccurrences(series));
  return 100.0 * cumulatedOccurrences / (cumulatedOccurrences + otherOccurrences);
}

int Store::totalNormalProbabilityValues(int series) const {
  if (!frequenciesAreInteger(series)) {
    return 0;
  }
  return static_cast<int>(std::round(sumOfOccurrences(series)));
}

double Store::normalProbabilityValueAtIndex(int series, int i) const {
  assert(frequenciesAreInteger(series));
  return sortedElementAtCumulatedPopulation(series, i + 1, false);
}

double Store::normalProbabilityResultAtIndex(int series, int i) const {
  int total = totalNormalProbabilityValues(series);
  assert(total > 0);
  // invnorm((i-0.5)/total,0,1)
  double plottingPosition = (static_cast<double>(i) + 0.5f) / static_cast<double>(total);
  return Poincare::NormalDistribution::CumulativeDistributiveInverseForProbability<double>(plottingPosition, 0.0, 1.0);
}

size_t Store::valueIndexAtSortedIndex(int series, int i) const {
  assert(m_sortedIndex && i >= 0 && i < numberOfPairsOfSeries(series));
  buildSortedIndex(series);
  return m_sortedIndex[series * k_maxNumberOfPairs + i];
}

void Store::buildSortedIndex(int series) const {
  // Index is already built
  if (m_sortedIndexValid[series]) {
    return;
  }
  // TODO : Factorize with Regression::Store::sortIndexByColumn
  int numberOfPairs = numberOfPairsOfSeries(series);
  for (int i = 0; i < numberOfPairs; i++) {
    m_sortedIndex[series * k_maxNumberOfPairs + i] = i;
  }
  /* Following lines is an insertion-sort algorithm which has the advantage of
   * being in-place and efficient when already sorted. */
  int i = 1;
  while (i < numberOfPairs) {
    int xIndex = m_sortedIndex[series * k_maxNumberOfPairs + i];
    double x = m_data[series][0][xIndex];
    int j = i - 1;
    while (j >= 0 && m_data[series][0][m_sortedIndex[series * k_maxNumberOfPairs + j]] > x) {
      m_sortedIndex[series * k_maxNumberOfPairs + j + 1] = m_sortedIndex[series * k_maxNumberOfPairs + j];
      j--;
    }
    m_sortedIndex[series * k_maxNumberOfPairs + j + 1] = xIndex;
    i++;
  }
  m_sortedIndexValid[series] = true;
}

}
