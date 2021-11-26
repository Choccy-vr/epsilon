#ifndef REGRESSION_EVEN_ODD_DOUBLE_BUFFER_TEXT_CELL_WITH_SEPARATOR_H
#define REGRESSION_EVEN_ODD_DOUBLE_BUFFER_TEXT_CELL_WITH_SEPARATOR_H

#include <escher/even_odd_cell.h>
#include <escher/even_odd_buffer_text_cell.h>
#include <escher/responder.h>

namespace Regression {

class EvenOddDoubleBufferTextCellWithSeparator : public Escher::EvenOddCell, public Escher::Responder {
public:
  EvenOddDoubleBufferTextCellWithSeparator(Escher::Responder * parentResponder = nullptr,
                                           float horizontalAlignment = KDFont::ALIGN_RIGHT,
                                           float verticalAlignment = KDFont::ALIGN_CENTER);
  const char * text() const override;
  const char * firstText() const;
  const char * secondText() const;
  void reloadCell() override;
  void setHighlighted(bool highlight) override;
  Escher::Responder * responder() override {
    return this;
  }
  void setEven(bool even) override;
  bool firstTextSelected();
  void selectFirstText(bool selectFirstText);
  void setFirstText(const char * textContent);
  void setSecondText(const char * textContent);
  void setTextColor(KDColor textColor);
  void drawRect(KDContext * ctx, KDRect rect) const override;
  int numberOfSubviews() const override;
  Escher::View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;
  bool handleEvent(Ion::Events::Event event) override;
protected:
  bool m_firstTextSelected;
  Escher::EvenOddBufferTextCell m_firstBufferTextView;
  Escher::EvenOddBufferTextCell m_secondBufferTextView;
};

}

#endif
