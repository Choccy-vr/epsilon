#include <escher/pop_up_controller.h>
#include <escher/metric.h>
#include <escher/container.h>
#include <assert.h>

namespace Escher {

// PopUpController

PopUpController::PopUpController(Invocation OkInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage, TextView * detailTextView) :
  ViewController(nullptr),
  m_contentView(this, OkInvocation, warningMessage, okMessage, cancelMessage, detailTextView)
{ }

View * PopUpController::view() {
  return &m_contentView;
}

void PopUpController::didBecomeFirstResponder() {
  m_contentView.setSelectedButton(0);
}

bool PopUpController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Left && m_contentView.selectedButton() == 1) {
    m_contentView.setSelectedButton(0);
    return true;
  }
  if (event == Ion::Events::Right && m_contentView.selectedButton() == 0) {
    m_contentView.setSelectedButton(1);
    return true;
  }
  return false;
}

void PopUpController::presentModally() {
  Escher::Container::activeApp()->displayModalViewController(this, 0.f, 0.f, Escher::Metric::PopUpTopMargin, Escher::Metric::PopUpRightMargin, Escher::Metric::PopUpBottomMargin, Escher::Metric::PopUpLeftMargin);
}


// PopUp Content View

PopUpController::ContentView::ContentView(Responder * parentResponder, Invocation okInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage, TextView * detailTextView) :
  Responder(parentResponder),
  m_cancelButton(
    this, cancelMessage,
    Invocation(
      [](void * context, void * sender) {
        Container::activeApp()->dismissModalViewController();
        return true;
      }, this),
    KDFont::SmallFont),
  m_okButton(this, okMessage, okInvocation, KDFont::SmallFont),
  m_warningTextView(KDFont::SmallFont, warningMessage, KDContext::k_alignCenter, KDContext::k_alignCenter, KDColorWhite, KDColorBlack),
  m_detailTextView(detailTextView)
{
}

void PopUpController::ContentView::setSelectedButton(int selectedButton) {
  m_cancelButton.setHighlighted(selectedButton == 0);
  m_okButton.setHighlighted(selectedButton == 1);
  Container::activeApp()->setFirstResponder(selectedButton == 0 ? &m_cancelButton : &m_okButton);
}

int PopUpController::ContentView::selectedButton() {
  return m_cancelButton.isHighlighted() ? 0 : 1;
}

int PopUpController::ContentView::numberOfSubviews() const {
  // Text + WarningTextView + CancelButton + OkButton
  return 4;
}

View * PopUpController::ContentView::subviewAtIndex(int index) {
  View * views[] = {&m_warningTextView, &m_cancelButton, &m_okButton, m_detailTextView};
  return views[index];
}

void PopUpController::ContentView::layoutSubviews(bool force) {
  KDCoordinate height = bounds().height();
  KDCoordinate width = bounds().width();
  KDCoordinate textHeight = KDFont::SmallFont->glyphSize().height();
  KDCoordinate detailTextHeight = m_detailTextView->minimalSizeForOptimalDisplay().height();

  m_warningTextView.setFrame(KDRect(0, k_topMargin, width, textHeight), force);
  KDCoordinate remainingHeight = (height - k_topMargin - textHeight - k_buttonMargin - k_buttonHeight - detailTextHeight);
  m_detailTextView->setFrame(KDRect(0, k_topMargin + textHeight + remainingHeight/2, width, detailTextHeight), force);
  m_cancelButton.setFrame(KDRect(k_buttonMargin, height - k_buttonMargin - k_buttonHeight, (width - 3 * k_buttonMargin) / 2, k_buttonHeight), force);
  m_okButton.setFrame(KDRect(2 * k_buttonMargin + (width - 3 * k_buttonMargin) / 2, height - k_buttonMargin - k_buttonHeight, (width - 3 * k_buttonMargin) / 2, k_buttonHeight), force);

  m_detailTextView->setFont(KDFont::SmallFont);
  m_detailTextView->setAlignment(KDContext::k_alignCenter, KDContext::k_alignCenter);
  m_detailTextView->setBackgroundColor(KDColorBlack);
  m_detailTextView->setTextColor(KDColorWhite);
}

// MessagePopUpController

MessagePopUpController::MessagePopUpController(Invocation OkInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage) :
    PopUpController(OkInvocation, warningMessage, okMessage, cancelMessage, &m_messageTextView)
{ }


void MessagePopUpController::setContentMessage(I18n::Message message) {
  m_messageTextView.setMessage(message);
}

// BufferPopUpController

BufferPopUpController::BufferPopUpController(Invocation OkInvocation, I18n::Message warningMessage, I18n::Message okMessage, I18n::Message cancelMessage) :
  PopUpController(OkInvocation, warningMessage, okMessage, cancelMessage, &m_bufferTextView)
{ }

void BufferPopUpController::setContentText(const char * text) {
  m_bufferTextView.setText(text);
}

void BufferPopUpController::setMessageWithPlaceholder(I18n::Message message, const char * string) {
  m_bufferTextView.setMessageWithPlaceholder(message, string);
}

}
