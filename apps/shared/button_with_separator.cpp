#include "button_with_separator.h"

using namespace Escher;

namespace Shared {

ButtonWithSeparator::ButtonWithSeparator(Responder * parentResponder,
                                         I18n::Message textBody,
                                         Escher::Invocation invocation,
                                         KDColor backgroundColor) :
      Button(parentResponder, textBody, invocation, KDFont::LargeFont, KDColorBlack),
      m_backgroundColor(backgroundColor) {
}

void ButtonWithSeparator::drawRect(KDContext * ctx, KDRect rect) const {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  ctx->fillRect(KDRect(0, 0, width, k_lineThickness), Palette::GrayBright);
  ctx->fillRect(KDRect(0, k_lineThickness, width, k_margin), m_backgroundColor);
  // Draw rectangle around cell
  ctx->fillRect(KDRect(0, k_lineThickness + k_margin, width, k_lineThickness), Palette::GrayBright);
  ctx->fillRect(KDRect(0, 2*k_lineThickness + k_margin, k_lineThickness, height - k_margin - k_lineThickness), Palette::GrayBright);
  ctx->fillRect(KDRect(width - k_lineThickness, 2*k_lineThickness + k_margin, k_lineThickness, height - k_margin - k_lineThickness), Palette::GrayBright);
  ctx->fillRect(KDRect(0, height - 3*k_lineThickness, width, k_lineThickness), Palette::GrayWhite);
  ctx->fillRect(KDRect(0, height - 2*k_lineThickness, width, k_lineThickness), Palette::GrayBright);
  ctx->fillRect(KDRect(k_lineThickness, height-k_lineThickness, width - 2*k_lineThickness, k_lineThickness), Palette::GrayMiddle);
}


void ButtonWithSeparator::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  m_messageTextView.setFrame(KDRect(k_lineThickness, k_margin + k_lineThickness, width-2*k_lineThickness, height - 4*k_lineThickness-k_margin), force);
}

KDSize ButtonWithSeparator::minimalSizeForOptimalDisplay() const {
  KDSize buttonSize = Escher::Button::minimalSizeForOptimalDisplay();
  return KDSize(buttonSize.width(), buttonSize.height() + k_margin + k_lineThickness);
}

}
