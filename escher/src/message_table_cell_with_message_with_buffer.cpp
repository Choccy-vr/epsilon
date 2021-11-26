#include <escher/message_table_cell_with_message_with_buffer.h>
#include <escher/palette.h>

namespace Escher {

MessageTableCellWithMessageWithBuffer::MessageTableCellWithMessageWithBuffer(I18n::Message message) :
  MessageTableCellWithMessage(message),
  m_accessoryView(KDFont::LargeFont, 1.0f, 0.5f, KDColorBlack)
{
}

void MessageTableCellWithMessageWithBuffer::setAccessoryText(const char * textBody) {
  m_accessoryView.setText(textBody);
  layoutSubviews();
}

void MessageTableCellWithMessageWithBuffer::setHighlighted(bool highlight) {
  MessageTableCellWithMessage::setHighlighted(highlight);
  KDColor backgroundColor = isHighlighted()? Palette::Select : KDColorWhite;
  m_accessoryView.setBackgroundColor(backgroundColor);
}

}
