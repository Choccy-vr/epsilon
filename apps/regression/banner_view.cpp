#include "banner_view.h"
#include <apps/i18n.h>
#include <assert.h>

using namespace Escher;

namespace Regression {

BannerView::BannerView() :
  XYBannerView(),
  m_otherView(Font(), KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor()),
  m_dataNotSuitableView(Font(), I18n::Message::DataNotSuitableForRegression, KDContext::k_alignCenter, KDContext::k_alignCenter, TextColor(), BackgroundColor()),
  m_displayOtherView(false),
  m_otherViewIsFirst(false),
  m_displayDataNotSuitable(false)
{}

void BannerView::setDisplayParameters(bool displayOtherView, bool otherViewIsFirst, bool displayDataNotSuitable) {
  assert(displayOtherView || !otherViewIsFirst);
  m_displayOtherView = displayOtherView;
  m_otherViewIsFirst = otherViewIsFirst;
  m_displayDataNotSuitable = displayDataNotSuitable;
}

View * BannerView::subviewAtIndex(int index) {
  assert(0 <= index && index < numberOfSubviews());
  if (m_displayOtherView && ((m_otherViewIsFirst && index == 0) || (!m_otherViewIsFirst && index == XYBannerView::k_numberOfSubviews))) {
    return &m_otherView;
  }
  index -= (m_displayOtherView && m_otherViewIsFirst);
  if (index >= XYBannerView::k_numberOfSubviews) {
    return &m_dataNotSuitableView;
  }
  return XYBannerView::subviewAtIndex(index);
}

}
