#ifndef PROBABILITY_MODELS_STATISTIC_HOMOGENEITY_TEST_H
#define PROBABILITY_MODELS_STATISTIC_HOMOGENEITY_TEST_H

#include "chi2_test.h"

namespace Probability {

class HomogeneityTest final : public Chi2Test {
public:
  // Used in HomogeneityTableDataSource
  constexpr static int k_maxNumberOfColumns = 9;
  constexpr static int k_maxNumberOfRows = 9;

  HomogeneityTest();
  CategoricalType categoricalType() const override { return CategoricalType::Homogeneity; }
  I18n::Message title() const override { return I18n::Message::InputHomogeneityControllerTitle; }
  void setGraphTitle(char * buffer, size_t bufferSize) const override;

  // Statistic
  bool validateInputs() override;
  // Test
  void compute() override;

  // Chi2Test
  bool deleteParameterAtPosition(int row, int column) override;
  void recomputeData() override;
  int maxNumberOfColumns() const override { return k_maxNumberOfColumns; };
  int maxNumberOfRows() const override { return k_maxNumberOfRows; };

  int numberOfStatisticParameters() const override {
    return k_maxNumberOfColumns * k_maxNumberOfRows;
  }

  int numberOfResultRows() { return m_numberOfResultRows; }
  int numberOfResultColumns() { return m_numberOfResultColumns; }
  double expectedValueAtLocation(int row, int column);

  double total() { return m_total;}
  double rowTotal(int row) { return m_rowTotals[row]; }
  double columnTotal(int column) { return m_columnTotals[column]; }

private:
  bool authorizedParameterAtIndex(double p, int i) const override;
  Index2D resultsIndexToIndex2D(int resultsIndex) const;
  int resultsIndexToArrayIndex(int resultsIndex) const;

  // Chi2Test
  double expectedValue(int resultsIndex) const override;
  double observedValue(int resultsIndex) const override;
  int numberOfValuePairs() const override;

  double observedValueAtPosition(Index2D index);
  double expectedValueAtPosition(Index2D index);
  int computeDegreesOfFreedom(Index2D max);
  double * parametersArray() override { return m_input; }
  void computeExpectedValues(Index2D max);

  double m_input[k_maxNumberOfColumns * k_maxNumberOfRows];
  double m_expectedValues[k_maxNumberOfColumns * k_maxNumberOfRows];
  double m_rowTotals[k_maxNumberOfRows];
  double m_columnTotals[k_maxNumberOfColumns];
  double m_total;
  int m_numberOfResultRows;
  int m_numberOfResultColumns;
};

}  // namespace Probability

#endif /* PROBABILITY_MODELS_STATISTIC_HOMOGENEITY_TEST_H */
