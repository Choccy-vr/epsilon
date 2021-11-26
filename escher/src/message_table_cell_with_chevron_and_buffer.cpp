#include <escher/message_table_cell_with_chevron_and_buffer.h>
#include <escher/palette.h>

namespace Escher {

MessageTableCellWithChevronAndBuffer::MessageTableCellWithChevronAndBuffer() :
  MessageTableCellWithChevron((I18n::Message)0),
  m_subLabelView(KDFont::SmallFont, KDContext::k_alignRight, KDContext::k_alignCenter, Palette::GrayDark)
{
}

void MessageTableCellWithChevronAndBuffer::setHighlighted(bool highlight) {
  MessageTableCellWithChevron::setHighlighted(highlight);
  KDColor backgroundColor = isHighlighted()? Palette::Select : KDColorWhite;
  m_subLabelView.setBackgroundColor(backgroundColor);
}

void MessageTableCellWithChevronAndBuffer::setSubLabelText(const char * textBody) {
  m_subLabelView.setText(textBody);
  layoutSubviews();
}

}
