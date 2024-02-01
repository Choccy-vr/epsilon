#include "box_range.h"

using namespace Poincare;

namespace Statistics {

BoxRange::BoxRange(Store* store) : m_store(store) {}

float BoxRange::computeMinMax(bool isMax) const {
  Range1D<float> range1D = Range1D<float>::ValidRangeBetween(
      m_store->minValueForAllSeries(
          false, Shared::DoublePairStore::DefaultActiveSeriesTest),
      m_store->maxValueForAllSeries(
          false, Shared::DoublePairStore::DefaultActiveSeriesTest));
  float max = range1D.max();
  float min = range1D.min();
  float margin = k_displayRightMarginRatio * (max - min);
  return isMax ? max + margin : min - margin;
}

}  // namespace Statistics
