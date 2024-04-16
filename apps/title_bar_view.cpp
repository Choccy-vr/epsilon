#include "title_bar_view.h"

#include <escher/palette.h>
#include <poincare/preferences.h>
#include <poincare/print.h>

#include <array>

#include "exam_icon.h"
extern "C" {
#include <assert.h>
}

using namespace Poincare;
using namespace Escher;

TitleBarView::TitleBarView()
    : View(),
      m_titleView(I18n::Message::Default, k_glyphsFormat),
      m_preferenceView(k_glyphsFormat),
      m_examModeTextView(I18n::Message::Default, k_glyphsFormat) {
  m_preferenceView.setAlignment(KDGlyph::k_alignRight, KDGlyph::k_alignCenter);
  m_examModeIconView.setImage(ImageStore::ExamIcon);
}

void TitleBarView::drawRect(KDContext* ctx, KDRect rect) const {
  /* As we cheated to layout the title view, we have to fill a very thin
   * rectangle at the top with the background color. */
  ctx->fillRect(KDRect(0, 0, bounds().width(), 2), k_backgroundColor);
}

void TitleBarView::setTitle(I18n::Message title) {
  m_titleView.setMessage(title);
}

bool TitleBarView::setChargeState(Ion::Battery::Charge chargeState) {
  return m_batteryView.setChargeState(chargeState);
}

bool TitleBarView::setIsCharging(bool isCharging) {
  return m_batteryView.setIsCharging(isCharging);
}

bool TitleBarView::setIsPlugged(bool isPlugged) {
  return m_batteryView.setIsPlugged(isPlugged);
}

bool TitleBarView::setShiftAlphaLockStatus(
    Ion::Events::ShiftAlphaStatus status) {
  return m_shiftAlphaLockView.setStatus(status);
}

void TitleBarView::updateBatteryAnimation() {
  m_batteryView.updateBatteryAnimation();
}

int TitleBarView::numberOfSubviews() const { return 6; }

View* TitleBarView::subviewAtIndex(int index) {
  View* views[] = {&m_titleView,          &m_preferenceView,
                   &m_examModeIconView,   &m_examModeTextView,
                   &m_shiftAlphaLockView, &m_batteryView};
  assert(0 <= index && index < static_cast<int>(std::size(views)));
  return views[index];
}

void TitleBarView::layoutSubviews(bool force) {
  /* We here cheat to layout the main title. The application title is written
   * with upper cases. But, as upper letters are on the same baseline as lower
   * letters, they seem to be slightly above when they are perfectly centered
   * (because their glyph never cross the baseline). To avoid this effect, we
   * translate the frame of the title downwards.*/
  constexpr int k_verticalShift = 2;
  setChildFrame(&m_titleView,
                KDRect(0, k_verticalShift, bounds().width(),
                       bounds().height() - k_verticalShift),
                force);
  setChildFrame(&m_preferenceView,
                KDRect(Metric::TitleBarExternHorizontalMargin, 0,
                       m_preferenceView.minimalSizeForOptimalDisplay().width(),
                       bounds().height()),
                force);
  KDSize batterySize = m_batteryView.minimalSizeForOptimalDisplay();
  setChildFrame(
      &m_batteryView,
      KDRect(bounds().width() - batterySize.width() -
                 Metric::TitleBarExternHorizontalMargin,
             (bounds().height() - batterySize.height()) / 2, batterySize),
      force);
  if (Preferences::SharedPreferences()->examMode().isActive()) {
    setChildFrame(
        &m_examModeIconView,
        KDRect(k_examIconMargin, (bounds().height() - k_examIconHeight) / 2,
               k_examIconWidth, k_examIconHeight),
        force);
    setChildFrame(&m_examModeTextView,
                  KDRect(k_examIconMargin - k_examTextWidth, k_verticalShift,
                         k_examTextWidth, bounds().height() - k_verticalShift),
                  force);
    I18n::Message examModeMessage;
    switch (Preferences::SharedPreferences()->examMode().ruleset()) {
      case ExamMode::Ruleset::English:
        examModeMessage = I18n::Message::ExamModeTitleBarUK;
        break;
      case ExamMode::Ruleset::Dutch:
        examModeMessage = I18n::Message::ExamModeTitleBarNL;
        break;
      case ExamMode::Ruleset::Portuguese:
        examModeMessage = I18n::Message::ExamModeTitleBarPT;
        break;
      case ExamMode::Ruleset::IBTest:
        examModeMessage = I18n::Message::ExamModeTitleBarIB;
        break;
      case ExamMode::Ruleset::STAAR:
        examModeMessage = I18n::Message::ExamModeTitleBarSTAAR;
        break;
      case ExamMode::Ruleset::Pennsylvania:
        examModeMessage = I18n::Message::ExamModeTitleBarPennsylvania;
        break;
      case ExamMode::Ruleset::SouthCarolina:
        examModeMessage = I18n::Message::ExamModeTitleBarSouthCarolina;
        break;
      case ExamMode::Ruleset::NorthCarolina:
        examModeMessage = I18n::Message::ExamModeTitleBarNorthCarolina;
        break;
      default:
        examModeMessage = I18n::Message::Default;
    }
    m_examModeTextView.setMessage(examModeMessage);
  } else {
    setChildFrame(&m_examModeIconView, KDRectZero, force);
    m_examModeTextView.setMessage(I18n::Message::Default);
  }
  KDSize shiftAlphaLockSize =
      m_shiftAlphaLockView.minimalSizeForOptimalDisplay();
  setChildFrame(&m_shiftAlphaLockView,
                KDRect(bounds().width() - batterySize.width() -
                           Metric::TitleBarExternHorizontalMargin -
                           k_alphaRightMargin - shiftAlphaLockSize.width(),
                       (bounds().height() - shiftAlphaLockSize.height()) / 2,
                       shiftAlphaLockSize),
                force);
}

void TitleBarView::refreshPreferences() {
  char buffer[k_preferenceTextSize];
  Preferences* preferences = Preferences::SharedPreferences();
  // Display Sci/ or Eng/ if the print float mode is not decimal
  const Preferences::PrintFloatMode printFloatMode = preferences->displayMode();
  I18n::Message floatModeMessage =
      printFloatMode == Preferences::PrintFloatMode::Decimal
          ? I18n::Message::Default
          : (printFloatMode == Preferences::PrintFloatMode::Scientific
                 ? I18n::Message::Sci
                 : I18n::Message::Eng);
  // Display the angle unit
  const Preferences::AngleUnit angleUnit = preferences->angleUnit();
  I18n::Message angleMessage =
      angleUnit == Preferences::AngleUnit::Degree
          ? I18n::Message::Deg
          : (angleUnit == Preferences::AngleUnit::Radian ? I18n::Message::Rad
                                                         : I18n::Message::Gon);
  Poincare::Print::CustomPrintf(buffer, k_preferenceTextSize, "%s%s",
                                I18n::translate(floatModeMessage),
                                I18n::translate(angleMessage));
  m_preferenceView.setText(buffer);
  // Layout the exam mode icon if needed
  layoutSubviews();
}

void TitleBarView::reload() {
  refreshPreferences();
  markWholeFrameAsDirty();
}
