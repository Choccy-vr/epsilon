#include "function_banner_delegate.h"

#include <poincare/serialization_helper.h>

#include "poincare_helpers.h"

using namespace Poincare;

namespace Shared {

void FunctionBannerDelegate::reloadBannerViewForCursorOnFunction(
    CurveViewCursor* cursor, Ion::Storage::Record record,
    FunctionStore* functionStore, Poincare::Context* context,
    bool cappedNumberOfSignificantDigits) {
  ExpiringPointer<Function> function = functionStore->modelForRecord(record);
  char buffer[k_textBufferSize];
  size_t numberOfChar = 0;
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, k_textBufferSize - numberOfChar - 1,
      function->symbol());
  assert(numberOfChar <= k_textBufferSize);
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, k_textBufferSize - numberOfChar, '=');
  bannerView()->abscissaSymbol()->setText(buffer);

  numberOfChar = function->printValue(
      cursor->t(), cursor->x(), cursor->y(), buffer, k_textBufferSize,
      numberOfSignificantDigits(cappedNumberOfSignificantDigits), context,
      true);

  assert(numberOfChar < k_textBufferSize);
  buffer[numberOfChar++] = '\0';
  bannerView()->abscissaValue()->setEditing(false);
  bannerView()->abscissaValue()->setText(buffer);

  numberOfChar = function->nameWithArgument(buffer, k_textBufferSize);
  assert(numberOfChar <= k_textBufferSize);
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, k_textBufferSize - numberOfChar, '=');
  numberOfChar += function->printValue(
      cursor->t(), cursor->x(), cursor->y(), buffer + numberOfChar,
      k_textBufferSize - numberOfChar,
      numberOfSignificantDigits(cappedNumberOfSignificantDigits), context,
      false);
  assert(numberOfChar < k_textBufferSize);
  buffer[numberOfChar++] = '\0';
  bannerView()->ordinateView()->setText(buffer);

  bannerView()->reload();
}

double FunctionBannerDelegate::GetValueDisplayedOnBanner(
    double t, Poincare::Context* context, int significantDigits,
    double deltaThreshold, bool roundToZero) {
  if (roundToZero && std::fabs(t) < deltaThreshold) {
    // Round to 0 to avoid rounding to unnecessary low non-zero value.
    return 0.0;
  }
  // Round to displayed value
  double displayedValue = PoincareHelpers::ValueOfFloatAsDisplayed<double>(
      t, significantDigits, context);
  // Return displayed value if difference from t is under deltaThreshold
  return std::fabs(displayedValue - t) < deltaThreshold ? displayedValue : t;
}

}  // namespace Shared
