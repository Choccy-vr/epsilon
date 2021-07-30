#include "box_banner_view.h"
#include <assert.h>

using namespace Escher;

namespace Statistics {

BoxBannerView::BoxBannerView() :
  m_seriesName(Font(), 0.5f, 0.5f, TextColor(), BackgroundColor()),
  m_calculationName(Font(), I18n::Message::Minimum, 0.5f, 0.5f, TextColor(), BackgroundColor()),
  m_calculationValue(Font(), 0.5f, 0.5f, TextColor(), BackgroundColor())
{
}

View * BoxBannerView::subviewAtIndex(int index) {
  assert(0 <= index && index < numberOfSubviews());
  View * subviews[] = {&m_seriesName, &m_calculationName, &m_calculationValue};
  return subviews[index];
}

}
