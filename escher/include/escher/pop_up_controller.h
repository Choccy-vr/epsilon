#ifndef ESCHER_POP_UP_CONTROLLER_H
#define ESCHER_POP_UP_CONTROLLER_H

#include <escher/button.h>
#include <escher/invocation.h>
#include <escher/buffer_text_view.h>
#include <escher/message_text_view.h>
#include <escher/view_controller.h>
#include <escher/i18n.h>

namespace Escher {

class HighContrastButton : public Button {
public:
  using Button::Button;
  KDColor highlightedBackgroundColor() const override { return Palette::YellowDark; }
};

class PopUpController : public ViewController {
public:
  PopUpController(Invocation OkInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage, TextView * detailTextView);
  View * view() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  void presentModally();
protected:
  class ContentView : public View, public Responder {
  public:
    ContentView(Responder * parentResponder, Invocation okInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage, TextView * detailTextView);
    void drawRect(KDContext * ctx, KDRect rect) const override { ctx->fillRect(bounds(), KDColorBlack); }
    void setSelectedButton(int selectedButton);
    int selectedButton();
    constexpr static KDCoordinate k_buttonMargin = 10;
    constexpr static KDCoordinate k_buttonHeight = 20;
    constexpr static KDCoordinate k_topMargin = 8;
  private:
    int numberOfSubviews() const override;
    View * subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    HighContrastButton m_cancelButton;
    HighContrastButton m_okButton;
    MessageTextView m_warningTextView;
    TextView * m_detailTextView;
  };
  ContentView m_contentView;
};

class MessagePopUpController : public PopUpController {
public:
  MessagePopUpController(Invocation OkInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage);
  void setContentMessage(I18n::Message message);
private:
  MessageTextView m_messageTextView;
};

class BufferPopUpController : public PopUpController {
public:
  BufferPopUpController(Invocation OkInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage);
  void setContentText(const char * text);
  void setMessageWithPlaceholder(I18n::Message message, const char * string = "");
private:
  BufferTextView m_bufferTextView;
};

}

#endif
