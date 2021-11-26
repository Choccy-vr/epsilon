#ifndef APPS_PROBABILITY_CONSTANTS_H
#define APPS_PROBABILITY_CONSTANTS_H

#include <poincare/print_float.h>

namespace Probability {

namespace Constants {

constexpr int k_shortBufferSize = Poincare::PrintFloat::charSizeForFloatsWithPrecision(
    Poincare::Preferences::ShortNumberOfSignificantDigits);
constexpr int k_largeBufferSize = Poincare::PrintFloat::charSizeForFloatsWithPrecision(
    Poincare::Preferences::LargeNumberOfSignificantDigits);

constexpr int k_shortFloatNumberOfChars = k_shortBufferSize - 1;
constexpr int k_largeFloatNumberOfChars = k_largeBufferSize - 1;

}  // namespace Constants

}  // namespace Probability

#endif /* APPS_PROBABILITY_CONSTANTS_H */
