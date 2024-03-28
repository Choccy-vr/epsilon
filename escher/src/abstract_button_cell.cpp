#include <assert.h>
#include <escher/abstract_button_cell.h>
#include <escher/palette.h>

namespace Escher {

AbstractButtonCell::AbstractButtonCell(Responder* parentResponder,
                                       I18n::Message textBody,
                                       Invocation invocation, KDFont::Size font,
                                       KDColor textColor)
    : HighlightCell(),
      Responder(parentResponder),
      m_messageTextView(textBody,
                        {.style = {.glyphColor = textColor, .font = font},
                         .horizontalAlignment = KDGlyph::k_alignCenter}),
      m_invocation(invocation),
      m_font(font) {}

View* AbstractButtonCell::subviewAtIndex(int index) {
  assert(index == 0);
  return &m_messageTextView;
}

bool AbstractButtonCell::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    m_invocation.perform(this);
    return true;
  }
  return event == Ion::Events::Var || event == Ion::Events::Sto ||
         event == Ion::Events::Clear;
}

void AbstractButtonCell::setHighlighted(bool highlight) {
  HighlightCell::setHighlighted(highlight);
  KDColor backgroundColor =
      highlight ? highlightedBackgroundColor() : KDColorWhite;
  m_messageTextView.setBackgroundColor(backgroundColor);
  markWholeFrameAsDirty();
}

KDSize AbstractButtonCell::minimalSizeForOptimalDisplay() const {
  KDSize textSize = m_messageTextView.minimalSizeForOptimalDisplay();
  if (m_font == KDFont::Size::Small) {
    return KDSize(textSize.width() + k_horizontalMarginSmall,
                  textSize.height() + k_verticalMarginSmall);
  }
  return KDSize(textSize.width() + k_horizontalMarginLarge,
                textSize.height() + k_verticalMarginLarge);
}

}  // namespace Escher
