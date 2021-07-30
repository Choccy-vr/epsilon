#ifndef SETTINGS_SELECTABLE_VIEW_WITH_MESSAGES_H
#define SETTINGS_SELECTABLE_VIEW_WITH_MESSAGES_H

#include <escher/selectable_table_view.h>
#include <escher/message_table_cell_with_editable_text.h>
#include <apps/i18n.h>

namespace Settings {

class SelectableViewWithMessages : public Escher::View {
public:
  SelectableViewWithMessages(Escher::SelectableTableView * selectableTableView);
  void drawRect(KDContext * ctx, KDRect rect) const override;
  void setMessages(I18n::Message * messages, int numberOfMessages);
  void reload();
private:
  int numberOfSubviews() const override { return 1 + m_numberOfMessages; }
  Escher::View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;
  Escher::SelectableTableView * m_selectableTableView;
  static constexpr int k_maxNumberOfLines = 4;
  static constexpr KDCoordinate k_minSelectableTableViewHeight = 60;
  Escher::MessageTextView m_messageLines[k_maxNumberOfLines];
  int m_numberOfMessages;
};

}

#endif
