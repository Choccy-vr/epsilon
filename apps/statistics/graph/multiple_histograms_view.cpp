#include "multiple_histograms_view.h"
#include <assert.h>

namespace Statistics {

MultipleHistogramsView::MultipleHistogramsView(Store * store, Shared::CurveViewRange * curveViewRange) :
  MultipleDataView(store),
  m_histogramView1(store, 0, curveViewRange, Shared::DoublePairStore::colorOfSeriesAtIndex(0)),
  m_histogramView2(store, 1, curveViewRange, Shared::DoublePairStore::colorOfSeriesAtIndex(1)),
  m_histogramView3(store, 2, curveViewRange, Shared::DoublePairStore::colorOfSeriesAtIndex(2))
{
  for (int i = 0; i < Store::k_numberOfSeries; i++) {
    HistogramView * histView = curveViewForSeries(i);
    histView->setDisplayLabels(false);
  }
}

HistogramView *  MultipleHistogramsView::curveViewForSeries(int series) {
  assert(series >= 0 && series < Shared::DoublePairStore::k_numberOfSeries);
  HistogramView * views[] = {&m_histogramView1, &m_histogramView2, &m_histogramView3};
  return views[series];
}

void MultipleHistogramsView::layoutSubviews(bool force) {
  MultipleDataView::layoutSubviews();
  assert(m_store->numberOfValidSeries() > 0);
  int displayedSubviewIndex = 0;
  for (int i = 0; i < Store::k_numberOfSeries; i++) {
    if (m_store->seriesIsValid(i)) {
      displayedSubviewIndex++;
    }
  }
}

void MultipleHistogramsView::changeDataViewSeriesSelection(int series, bool select) {
  MultipleDataView::changeDataViewSeriesSelection(series, select);
  curveViewForSeries(series)->setDisplayLabels(select);
  if (select == false) {
    // Set the hightlight to default selected bar to prevent blinking
    curveViewForSeries(series)->setHighlight(m_store->startOfBarAtIndex(series, k_defaultSelectedIndex), m_store->endOfBarAtIndex(series, k_defaultSelectedIndex));
  }
}

}
