#include <assert.h>
#include <escher/button_state.h>

#include <algorithm>

namespace Escher {

ButtonState::ButtonState(Responder* parentResponder, I18n::Message textBody,
                         Invocation invocation, ToggleableView* stateView,
                         KDFont::Size font, KDColor textColor)
    : SimpleButtonCell(parentResponder, textBody, invocation, font, textColor),
      m_stateView(stateView) {}

void ButtonState::setState(bool state) {
  m_stateView->setState(state);
  // Some stateViews like dot and unequal are transparent
  markRectAsDirty(relativeChildFrame(m_stateView));
}

View* ButtonState::subviewAtIndex(int index) {
  assert(index >= 0 && index < 2);
  if (index == 0) {
    return &m_messageTextView;
  }
  assert(m_stateView);
  return m_stateView;
}

void ButtonState::layoutSubviews(bool force) {
  KDSize textSize = SimpleButtonCell::minimalSizeForOptimalDisplay();
  KDRect textRect = KDRect(0, 0, textSize.width(), bounds().height());
  // State view will be vertically centered and aligned on the left
  KDSize stateSize = m_stateView->minimalSizeForOptimalDisplay();
  KDCoordinate verticalOffset = (textSize.height() - stateSize.height()) / 2;
  KDRect stateRect = KDRect(textSize.width(), verticalOffset, stateSize.width(),
                            stateSize.height());

  setChildFrame(&m_messageTextView, textRect, force);
  setChildFrame(m_stateView, stateRect, force);
}

void ButtonState::drawRect(KDContext* ctx, KDRect rect) const {
  KDColor backColor =
      isHighlighted() ? highlightedBackgroundColor() : KDColorWhite;
  ctx->fillRect(bounds(), backColor);
}

KDSize ButtonState::minimalSizeForOptimalDisplay() const {
  KDSize textSize = SimpleButtonCell::minimalSizeForOptimalDisplay();
  KDSize stateSize = m_stateView->minimalSizeForOptimalDisplay();
  return KDSize(
      textSize.width() +
          (stateSize.width() > 0 ? stateSize.width() + k_stateMargin : 0),
      std::max(textSize.height(), stateSize.height()));
}

}  // namespace Escher
