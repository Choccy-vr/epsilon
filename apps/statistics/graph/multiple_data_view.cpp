#include "multiple_data_view.h"
#include <assert.h>

using namespace Shared;
using namespace Escher;

namespace Statistics {

KDCoordinate MultipleDataView::subviewHeight() {
  int numberDataSubviews = m_store->numberOfValidSeries(Shared::DoublePairStore::DefaultValidSeries);
  assert(numberDataSubviews > 0);
  KDCoordinate bannerHeight = bannerView()->minimalSizeForOptimalDisplay().height();
  // +1 to make sure that all pixel rows are drawn
  return (bounds().height() - bannerHeight)/numberDataSubviews + 1;
}

void MultipleDataView::reload() {
  layoutSubviews();
  for (int i = 0; i < Store::k_numberOfSeries; i++) {
    curveViewForSeries(i)->reload();
  }
}

int MultipleDataView::numberOfSubviews() const {
  int result = m_store->numberOfValidSeries(Shared::DoublePairStore::DefaultValidSeries);
  assert(result <= Store::k_numberOfSeries);
  return result + 1; // +1 for the banner view
}

void MultipleDataView::drawRect(KDContext * ctx, KDRect rect) const {
  if (!m_displayBanner) {
    ctx->fillRect(bannerFrame(), KDColorWhite);
  }
}

View * MultipleDataView::subviewAtIndex(int index) {
  if (index == MultipleDataView::numberOfSubviews() -1) {
    return bannerView();
  }
  return curveViewForSeries(m_store->indexOfKthValidSeries(index, Shared::DoublePairStore::DefaultValidSeries));
}

void MultipleDataView::layoutDataSubviews(bool force) {
  KDCoordinate subHeight = subviewHeight();
  int displayedSubviewIndex = 0;
  for (int i = 0; i < Store::k_numberOfSeries; i++) {
    if (Shared::DoublePairStore::DefaultValidSeries(m_store, i)) {
      CurveView * curveView = curveViewForSeries(i);
      KDRect frame = KDRect(0, displayedSubviewIndex * subHeight, bounds().width(), subHeight);
      curveView->setFrame(frame, force);
      displayedSubviewIndex++;
    }
  }
}

}
