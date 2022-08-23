#include "uniform_distribution.h"
#include <assert.h>
#include <algorithm>
#include <cmath>
#include <float.h>
#include <poincare/code_point_layout.h>

namespace Probability {

float UniformDistribution::evaluateAtAbscissa(float t) const {
  float parameter1 = m_parameters[0];
  float parameter2 = m_parameters[1];
  if (parameter2 - parameter1 < FLT_EPSILON) {
    if (parameter1 - k_diracWidth<= t && t <= parameter2 + k_diracWidth) {
      return 2.0f * k_diracMaximum;
    }
    return 0.0f;
  }
  if (parameter1 <= t && t <= parameter2) {
    return (1.0f/(parameter2 - parameter1));
  }
  return 0.0f;
}

bool UniformDistribution::authorizedParameterAtIndex(double x, int index) const {
  if (!TwoParameterDistribution::authorizedParameterAtIndex(x, index)) {
    return false;
  }
  if (index == 0) {
    return true;
  }
  if (m_parameters[0] > x) {
    return false;
  }
  return true;
}

void UniformDistribution::setParameterAtIndex(double f, int index) {
  Inference::setParameterAtIndex(f, index);
  if (index == 0 && m_parameters[1] < m_parameters[0]) {
    // Add more than 1.0 if first parameter is greater than 100.
    m_parameters[1] = m_parameters[0] + std::max(1.0, std::round(std::fabs(m_parameters[0]) * 0.01));
  }
  computeCurveViewRange();
}

double UniformDistribution::cumulativeDistributiveFunctionAtAbscissa(double x) const {
  if (x <= m_parameters[0]) {
    return 0.0;
  }
  if (x < m_parameters[1]) {
    return (x - m_parameters[0])/(m_parameters[1] - m_parameters[0]);
  }
  return 1.0;
}

double UniformDistribution::cumulativeDistributiveInverseForProbability(double * p) {
  if (*p >= 1.0f) {
    return m_parameters[1];
  }
  if (*p <= 0.0f) {
    return m_parameters[0];
  }
  return m_parameters[0] * (1 - *p) + *p * m_parameters[1];
}

ParameterRepresentation UniformDistribution::paramRepresentationAtIndex(int i) const {
  switch (i) {
    case ParamsOrder::A:
    {
      Poincare::Layout a = Poincare::CodePointLayout::Builder('a');
      return ParameterRepresentation{a, I18n::Message::IntervalADescr};
    }
    case ParamsOrder::B:
    {
      Poincare::Layout b = Poincare::CodePointLayout::Builder('b');
      return ParameterRepresentation{b, I18n::Message::IntervalBDescr};
    }
    default:
      assert(false);
      return ParameterRepresentation{};
  }
}

float UniformDistribution::computeXMin() const {
  assert(m_parameters[1] >= m_parameters[0]);
  if (m_parameters[1] - m_parameters[0] < FLT_EPSILON) {
    // If parameter is too big, subtracting only 1.0 wouldn't do anything.
    return m_parameters[0] - std::max(1.0, std::fabs(m_parameters[0]) * k_displayLeftMarginRatio);
  }
  return m_parameters[0] - 0.6f * (m_parameters[1] - m_parameters[0]);
}

float UniformDistribution::computeXMax() const {
  if (m_parameters[1] - m_parameters[0] < FLT_EPSILON) {
    // If parameter is too big, adding only 1.0 wouldn't do anything.
    return m_parameters[0] + std::max(1.0, std::fabs(m_parameters[0]) * k_displayRightMarginRatio);
  }
  return m_parameters[1] + 0.6f * (m_parameters[1] - m_parameters[0]);
}

float UniformDistribution::computeYMax() const {
  float result = m_parameters[1] - m_parameters[0] < FLT_EPSILON ? k_diracMaximum : 1.0f/(m_parameters[1]-m_parameters[0]);
  if (result <= 0.0f || std::isnan(result) || std::isinf(result)) {
    result = 1.0f;
  }
  return result * (1.0f+ k_displayTopMarginRatio);
}


}
