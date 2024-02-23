#ifndef GRAPH_BANNER_VIEW_H
#define GRAPH_BANNER_VIEW_H

#include <apps/shared/cursor_view.h>
#include <apps/shared/xy_banner_view.h>
#include <escher/message_text_view.h>

namespace Graph {

class BannerView : public Shared::XYBannerView {
 public:
  BannerView(Escher::Responder* parentResponder,
             Escher::TextFieldDelegate* textFieldDelegate);

  BannerBufferTextView* firstDerivativeView() { return &m_firstDerivativeView; }
  BannerBufferTextView* secondDerivativeView() {
    return &m_secondDerivativeView;
  }
  BannerBufferTextView* slopeView() { return &m_slopeView; }
  BannerBufferTextView* aView() { return &m_aView; }
  BannerBufferTextView* bView() { return &m_bView; }
  int numberOfInterestMessages() const;
  void addInterestMessage(I18n::Message message, Shared::CursorView* cursor);
  void emptyInterestMessages(Shared::CursorView* cursor);
  void setDisplayParameters(bool showInterest, bool showFirstDerivative,
                            bool showSecondDerivative, bool showSlope,
                            bool showTangent);

 private:
  constexpr static int k_maxNumberOfInterests = 3;

  int numberOfSubviews() const override {
    // there are 3 views for tangent (aView, bView, tangentEquationView)
    return XYBannerView::k_numberOfSubviews + numberOfInterestMessages() +
           m_showFirstDerivative + m_showSecondDerivative + m_showSlope +
           3 * m_showTangent;
  };
  Escher::View* subviewAtIndex(int index) override;
  bool lineBreakBeforeSubview(Escher::View* subview) const override;
  bool hasInterestMessage(int i) const {
    assert(i >= 0 && i < k_maxNumberOfInterests);
    return m_showInterest && m_interestMessageView[i].text()[0] != '\0';
  }

  Escher::MessageTextView m_interestMessageView[k_maxNumberOfInterests];
  BannerBufferTextView m_firstDerivativeView;
  BannerBufferTextView m_secondDerivativeView;
  BannerBufferTextView m_slopeView;
  Escher::MessageTextView m_tangentEquationView;
  BannerBufferTextView m_aView;
  BannerBufferTextView m_bView;
  bool m_showInterest : 1;
  bool m_showFirstDerivative : 1;
  bool m_showSecondDerivative : 1;
  bool m_showSlope : 1;
  bool m_showTangent : 1;
};

}  // namespace Graph

#endif
