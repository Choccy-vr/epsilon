#ifndef STATISTICS_STORE_H
#define STATISTICS_STORE_H

#include <apps/i18n.h>
#include <apps/shared/double_pair_store.h>
#include <stddef.h>

namespace Statistics {

class Store : public Shared::DoublePairStore {
friend class BoxRange;
public:
  Store();
  void setSortedIndex(uint8_t * buffer, size_t bufferSize);
  void invalidateSortedIndexes();
  bool graphViewHasBeenInvalidated() const { return m_graphViewInvalidated; }
  void graphViewHasBeenSelected() { m_graphViewInvalidated = false; }
  int validSeriesIndex(int series) const;
  // Histogram bars
  double barWidth() const { return m_barWidth; }
  void setBarWidth(double barWidth);
  double firstDrawnBarAbscissa() const { return m_firstDrawnBarAbscissa; }
  void setFirstDrawnBarAbscissa(double firstDrawnBarAbscissa) { m_firstDrawnBarAbscissa = firstDrawnBarAbscissa;}
  double heightOfBarAtIndex(int series, int index) const;
  float maxHeightOfBar(int series) const;
  float heightOfBarAtValue(int series, float value) const;
  double startOfBarAtIndex(int series, int index) const;
  double endOfBarAtIndex(int series, int index) const;
  int numberOfBars(int series) const;
  // Box plot
  bool displayOutliers() const { return m_displayOutliers; }
  void setDisplayOutliers(bool displayOutliers) { m_displayOutliers = displayOutliers; }
  I18n::Message boxPlotCalculationMessageAtIndex(int series, int index) const;
  double boxPlotCalculationAtIndex(int series, int index) const;
  bool boxPlotCalculationIsOutlier(int series, int index) const;
  int numberOfBoxPlotCalculations(int series) const;
  bool columnIsIntegersOnly(int series, int column) const;

  // Calculation
  double sumOfOccurrences(int series) const;
  // If handleNullFrequencies, values with a null frequency are accounted for
  double maxValueForAllSeries(bool handleNullFrequencies = false) const;
  double minValueForAllSeries(bool handleNullFrequencies = false) const;
  double maxValue(int series, bool handleNullFrequencies) const;
  double minValue(int series, bool handleNullFrequencies) const;
  // Overloading minValue and maxValue so they can be casted as CalculPointer
  double maxValue(int series) const { return maxValue(series, false); }
  double minValue(int series) const { return minValue(series, false); }
  double range(int series) const;
  double mean(int series) const;
  double variance(int series) const;
  double standardDeviation(int series) const;
  double sampleStandardDeviation(int series) const;
  double firstQuartile(int series) const;
  double thirdQuartile(int series) const;
  double quartileRange(int series) const;
  double median(int series) const;
  double lowerWhisker(int series) const;
  double upperWhisker(int series) const;
  double lowerFence(int series) const;
  double upperFence(int series) const;
  int numberOfLowerOutliers(int series) const;
  int numberOfUpperOutliers(int series) const;
  double lowerOutlierAtIndex(int series, int index) const;
  double upperOutlierAtIndex(int series, int index) const;
  double sum(int series) const;
  double squaredValueSum(int series) const;
  double squaredOffsettedValueSum(int series, double offset) const;
  int numberOfModes(int series) const;
  int totalNumberOfModes() const;
  double modeAtIndex(int series, int index) const;
  double modeFrequency(int series) const;

  /* Cumulated frequencies graphs:
   * Distinct values are aggregated and their frequency summed. */
  // Return number of distinct values
  int totalCumulatedFrequencyValues(int series) const;
  // Return the i-th distinct sorted value
  double cumulatedFrequencyValueAtIndex(int series, int i) const;
  // Return the cumulated frequency of the i-th distinct sorted value
  double cumulatedFrequencyResultAtIndex(int series, int i) const;

  /* Normal probability graphs:
   * Values are scattered into elements of frequency 1. */
  // Return the sumOfOccurrences, return 0 if any frequency is not an integer
  double totalNormalProbabilityValues(int series) const;
  // Return the sorted element at cumulated population i+1
  double normalProbabilityValueAtIndex(int series, int i) const;
  // Return the z-score of the i-th sorted element
  double normalProbabilityResultAtIndex(int series, int i) const;

  // DoublePairStore
  void sortColumn(int series, int column) override;
  void set(double f, int series, int i, int j) override;
  void memoizeValidSeries(int series) override;
  bool deleteValueAtIndex(int series, int i, int j) override;
  void deletePairOfSeriesAtIndex(int series, int j) override;
  void resetColumn(int series, int i) override;
  void deleteAllPairsOfSeries(int series) override;

  typedef double (Store::*CalculPointer)(int) const;
private:
  constexpr static int k_numberOfQuantiles = 5;
  constexpr static I18n::Message k_quantilesName[k_numberOfQuantiles] = {
    I18n::Message::StatisticsBoxLowerWhisker,
    I18n::Message::FirstQuartile,
    I18n::Message::Median,
    I18n::Message::ThirdQuartile,
    I18n::Message::StatisticsBoxUpperWhisker
  };
  constexpr static CalculPointer k_quantileCalculation[k_numberOfQuantiles] = {
    &Store::lowerWhisker,
    &Store::firstQuartile,
    &Store::median,
    &Store::thirdQuartile,
    &Store::upperWhisker
  };

  double defaultValue(int series, int i, int j) const override;
  double sumOfValuesBetween(int series, double x1, double x2) const;
  /* Find the i-th distinct value (if i is -1, browse the entire series) from
   * start to end (ordered by value).
   * Retrieve the i-th value and the number distinct values encountered.
   * If not handleNullFrequencies, ignore values with null frequency. */
  void countDistinctValues(int series, int start, int end, int i, bool handleNullFrequencies, double * value, int * distinctValues) const;
  /* Find the i-th mode (ordered by value). Also retrieve the total number of
   * modes and the mode frequency. */
  double computeModes(int series, int i, double * modeFreq, int * modesTotal) const;
  double sortedElementAtCumulatedFrequency(int series, double k, bool createMiddleElement = false) const;
  double sortedElementAtCumulatedPopulation(int series, double population, bool createMiddleElement = false) const;
  uint8_t lowerWhiskerSortedIndex(int series) const;
  uint8_t upperWhiskerSortedIndex(int series) const;
  // Return the value index from its sorted index (a 0 sorted index is the min)
  uint8_t valueIndexAtSortedIndex(int series, int i) const;
  // Sort and memoize values indexes in increasing order.
  void buildSortedIndex(int series) const;
  // Histogram bars
  double m_barWidth;
  double m_firstDrawnBarAbscissa;
  // Sorted value indexes are memoized to save computation
  static_assert(k_maxNumberOfPairs <= UINT8_MAX, "k_maxNumberOfPairs is too large.");
  mutable uint8_t * m_sortedIndex;
  mutable bool m_sortedIndexValid[k_numberOfSeries];
  /* Memoizing the max number of modes because the CalculationControllers needs
   * it in numberOfRows(), which is used a lot. */
  mutable int m_memoizedMaxNumberOfModes;
  bool m_graphViewInvalidated;
  bool m_displayOutliers;
};

}

#endif
