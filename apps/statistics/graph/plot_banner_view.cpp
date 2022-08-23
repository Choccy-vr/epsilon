#include "plot_banner_view.h"
#include <assert.h>

namespace Statistics {

PlotBannerView::PlotBannerView() :
  m_seriesName(Font(), KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor()),
  m_value(Font(), KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor()),
  m_result(Font(), KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor()) {
}

Escher::View * PlotBannerView::subviewAtIndex(int index) {
  assert(0 <= index && index < numberOfSubviews());
  Escher::View * subviews[] = {&m_seriesName, &m_value, &m_result};
  return subviews[index];
}

}
