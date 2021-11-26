#ifndef ESCHER_MESSAGE_TABLE_CELL_WITH_MESSAGE_WITH_BUFFER_H
#define ESCHER_MESSAGE_TABLE_CELL_WITH_MESSAGE_WITH_BUFFER_H

#include <escher/message_table_cell_with_message.h>
#include <escher/buffer_text_view.h>

namespace Escher {

class MessageTableCellWithMessageWithBuffer : public MessageTableCellWithMessage {
public:
  MessageTableCellWithMessageWithBuffer(I18n::Message message = (I18n::Message)0);
  const View * accessoryView() const override { return &m_accessoryView; }
  void setHighlighted(bool highlight) override;
  void setAccessoryText(const char * textBody);
  const char * accessoryText();
  const char * text() const override {
    return m_accessoryView.text();
  }
protected:
  BufferTextView m_accessoryView;
};

}

#endif
