#ifndef APPS_PROBABILITY_GUI_SUBAPP_CELL_H
#define APPS_PROBABILITY_GUI_SUBAPP_CELL_H

#include <escher/bordered.h>
#include <escher/chevron_view.h>

#include "highlight_image_cell.h"
#include "highlight_message_view.h"

namespace Probability {

/* Cell made of an icon, a title, a chevron and a subtitle below. */
class SubappCell : public Escher::HighlightCell, public Escher::Bordered {
public:
  SubappCell();
  void drawRect(KDContext * ctx, KDRect rect) const override;
  void setHighlighted(bool highlighted) override;
  Escher::View * subviewAtIndex(int i) override;
  int numberOfSubviews() const override { return 4; }
  KDSize minimalSizeForOptimalDisplay() const override;
  void layoutSubviews(bool force = false) override;

  void setImage(const Escher::Image * image);
  void setMessages(I18n::Message title, I18n::Message subTitle);

private:
  constexpr static int k_verticalMarginTop = 10;
  constexpr static int k_verticalMarginBetweenTexts = 5;
  constexpr static int k_verticalMarginBottom = 7;
  constexpr static int k_horizontalMarginBetweenTextAndIcon = 15;

  HighlightImageCell m_icon;
  HighlightMessageView m_title;
  HighlightMessageView m_subTitle;
  Escher::ChevronView m_chevron;
};

}  // namespace Probability

#endif /* APPS_PROBABILITY_GUI_SUBAPP_CELL_H */
