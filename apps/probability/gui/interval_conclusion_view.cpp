#include "interval_conclusion_view.h"

#include <apps/i18n.h>
#include <kandinsky/font.h>
#include <poincare/print.h>
#include <poincare/preferences.h>

#include "probability/text_helpers.h"

namespace Probability {

IntervalConclusionView::IntervalConclusionView() {
  setFont(KDFont::LargeFont);
  setAlignment(KDContext::k_alignCenter, KDContext::k_alignCenter);
  setBackgroundColor(Escher::Palette::WallScreen);
}

void IntervalConclusionView::setInterval(double center, double marginOfError) {
  constexpr static int bufferSize = Constants::k_shortBufferSize;
  char bufferCenter[bufferSize];
  char buffermarginOfError[bufferSize];

  defaultConvertFloatToText(center, bufferCenter, bufferSize);
  defaultConvertFloatToText(marginOfError, buffermarginOfError, bufferSize);
  // Use the same precision as in StatisticCurveView::drawInterval
  Poincare::Print::customPrintf(m_buffer, k_maxNumberOfChar, "%s\n%*.*ed ± %*.*ed",
           I18n::translate(I18n::Message::ConfidenceInterval),
           center, Poincare::Preferences::PrintFloatMode::Decimal, Poincare::Preferences::LargeNumberOfSignificantDigits,
           marginOfError, Poincare::Preferences::PrintFloatMode::Decimal, Poincare::Preferences::LargeNumberOfSignificantDigits);
}

}  // namespace Probability
