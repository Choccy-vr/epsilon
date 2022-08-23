#ifndef PROBABILITY_MODELS_STATISTIC_GOODNESS_TEST_H
#define PROBABILITY_MODELS_STATISTIC_GOODNESS_TEST_H

#include "chi2_test.h"

namespace Probability {

class GoodnessTest final : public Chi2Test {
public:
  GoodnessTest();
  CategoricalType categoricalType() const override { return CategoricalType::GoodnessOfFit; }
  I18n::Message title() const override { return I18n::Message::InputGoodnessControllerTitle; }
  int numberOfStatisticParameters() const override { return k_maxNumberOfRows * k_maxNumberOfColumns; }
  void setGraphTitle(char * buffer, size_t bufferSize) const override;
  void setResultTitle(char * buffer, size_t bufferSize, bool resultIsTopPage) const override;

  // Inference
  bool validateInputs() override;
  // Statistic
  int numberOfResults() const override { return 2; }
  void compute() override;

  // Chi2Test
  void recomputeData() override;
  int maxNumberOfColumns() const override { return k_maxNumberOfColumns; };
  int maxNumberOfRows() const override { return k_maxNumberOfRows; };

  void setDegreeOfFreedom(double degreeOfFreedom) { m_degreesOfFreedom = degreeOfFreedom; }
  /* Return the DegreesOfFreedom computed from the numberOfValuesPairs. Actual
   * statistic's degree of freedom may differ because it can be overridden by
   * the user. */
  int computeDegreesOfFreedom() { return numberOfValuePairs() - 1; }

  constexpr static int k_maxNumberOfColumns = 2;
private:
  constexpr static int k_maxNumberOfRows = 10;

  // Statistic
  bool authorizedParameterAtIndex(double p, int i) const override;

  // Chi2Test
  double expectedValue(int index) const override;
  double observedValue(int index) const override;
  int numberOfValuePairs() const override;

  double * parametersArray() override { return m_input; }
  void setExpectedValue(int index, double value);
  void setObservedValue(int index, double value);
  Index2D initialDimensions() const override { return Index2D{.row = 1, .col = 2}; }

  double m_input[k_maxNumberOfRows * k_maxNumberOfColumns];
};

}  // namespace Probability

#endif /* PROBABILITY_MODELS_STATISTIC_GOODNESS_TEST_H */
