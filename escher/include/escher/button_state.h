#ifndef ESCHER_BUTTON_STATE_H
#define ESCHER_BUTTON_STATE_H

#include <escher/button.h>
#include <escher/toggleable_view.h>

namespace Escher {

class ButtonState : public Button {
public:
  ButtonState(Responder * parentResponder, I18n::Message textBody, Invocation invocation, ToggleableView * stateView, const KDFont * font = KDFont::SmallFont, KDColor textColor = KDColorBlack);
  void setState(bool state);
  KDSize minimalSizeForOptimalDisplay() const override;
  void drawRect(KDContext * ctx, KDRect rect) const override;
private:
  // Dot right margin.
  constexpr static KDCoordinate k_stateMargin = 9;
  int numberOfSubviews() const override { return 2; }
  View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;
  ToggleableView * m_stateView;
};

}

#endif
