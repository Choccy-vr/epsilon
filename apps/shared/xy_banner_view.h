#ifndef SHARED_XY_BANNER_VIEW_H
#define SHARED_XY_BANNER_VIEW_H

#include "banner_view.h"
#include "editable_field_banner_view_delegate.h"

namespace Shared {

class XYBannerView : public BannerView, public EditableFieldBannerViewDelegate {
 public:
  XYBannerView(Escher::Responder* parentResponder,
               Escher::TextFieldDelegate* textFieldDelegate)
      : EditableFieldBannerViewDelegate(parentResponder, textFieldDelegate),
        m_ordinateView(k_bannerFieldFormat) {}

  BannerBufferTextView* abscissaSymbol() { return editableFieldLabel(); }
  Escher::TextField* abscissaValue() { return editableField(); }
  BannerBufferTextView* ordinateView() { return &m_ordinateView; }
  constexpr static int k_numberOfSubviews = 2;

 protected:
  Escher::View* subviewAtIndex(int index) override {
    assert(0 <= index && index < k_numberOfSubviews);
    return index == 0 ? editableView() : &m_ordinateView;
  }
  BannerBufferTextView m_ordinateView;

 private:
  int numberOfSubviews() const override { return k_numberOfSubviews; }
};

}  // namespace Shared

#endif
