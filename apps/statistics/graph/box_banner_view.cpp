#include "box_banner_view.h"
#include <assert.h>

using namespace Escher;

namespace Statistics {

BoxBannerView::BoxBannerView() :
  m_seriesName(Font(), KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor()),
  m_calculationBuffer(Font(), KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor())
{
}

View * BoxBannerView::subviewAtIndex(int index) {
  assert(0 <= index && index < numberOfSubviews());
  View * subviews[] = {&m_seriesName, &m_calculationBuffer};
  return subviews[index];
}

}
