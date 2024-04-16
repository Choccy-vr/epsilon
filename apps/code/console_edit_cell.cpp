#include "console_edit_cell.h"

#include <apps/global_preferences.h>
#include <apps/i18n.h>
#include <assert.h>

#include <algorithm>

#include "app.h"
#include "console_controller.h"

using namespace Escher;

namespace Code {

ConsoleEditCell::ConsoleEditCell(Responder *parentResponder,
                                 TextFieldDelegate *delegate)
    : HighlightCell(),
      Responder(parentResponder),
      m_promptView(
          nullptr,
          {.style =
               {.font = GlobalPreferences::SharedGlobalPreferences()->font()}}),
      m_textField(
          this, nullptr, TextField::MaxBufferSize(), delegate,
          {.style = {
               .font =
                   GlobalPreferences::SharedGlobalPreferences()->font()}}) {}

int ConsoleEditCell::numberOfSubviews() const { return 2; }

View *ConsoleEditCell::subviewAtIndex(int index) {
  assert(index == 0 || index == 1);
  if (index == 0) {
    return &m_promptView;
  } else {
    return &m_textField;
  }
}

void ConsoleEditCell::layoutSubviews(bool force) {
  KDSize promptSize = m_promptView.minimalSizeForOptimalDisplay();
  setChildFrame(&m_promptView,
                KDRect(KDPointZero, promptSize.width(), bounds().height()),
                force);
  setChildFrame(
      &m_textField,
      KDRect(KDPoint(promptSize.width(), KDCoordinate(0)),
             bounds().width() - promptSize.width(), bounds().height()),
      force);
}

void ConsoleEditCell::didBecomeFirstResponder() {
  App::app()->setFirstResponder(&m_textField);
  setEditing(true);
}

void ConsoleEditCell::setEditing(bool isEditing) {
  m_textField.setEditing(isEditing);
}

void ConsoleEditCell::setText(const char *text) { m_textField.setText(text); }

void ConsoleEditCell::setPrompt(const char *prompt) {
  m_promptView.setText(prompt);
  layoutSubviews();
}

bool ConsoleEditCell::insertText(const char *text) {
  return m_textField.handleEventWithText(text);
}

}  // namespace Code
