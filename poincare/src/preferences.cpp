#include <assert.h>
#include <poincare/expression.h>
#include <poincare/preferences.h>
#include <poincare/unit.h>

namespace Poincare {

constexpr int Preferences::DefaultNumberOfPrintedSignificantDigits;
constexpr int Preferences::VeryLargeNumberOfSignificantDigits;
constexpr int Preferences::LargeNumberOfSignificantDigits;
constexpr int Preferences::MediumNumberOfSignificantDigits;
constexpr int Preferences::ShortNumberOfSignificantDigits;
constexpr int Preferences::VeryShortNumberOfSignificantDigits;

OMG::GlobalBox<Preferences> Preferences::sharedPreferences;

Preferences::Preferences()
    : m_calculationPreferences{
          .angleUnit = AngleUnit::Radian,
          .displayMode = Preferences::PrintFloatMode::Decimal,
          .editionMode = EditionMode::Edition2D,
          .complexFormat = Preferences::ComplexFormat::Real,
          .numberOfSignificantDigits =
              Preferences::DefaultNumberOfPrintedSignificantDigits} {}

Preferences::ComplexFormat Preferences::UpdatedComplexFormatWithExpressionInput(
    ComplexFormat complexFormat, const Expression& exp, Context* context) {
  if (complexFormat == ComplexFormat::Real && exp.hasComplexI(context)) {
    return k_defautComplexFormatIfNotReal;
  }
  return complexFormat;
}

ExamMode Preferences::examMode() const {
  if (m_examMode.isUninitialized()) {
    m_examMode = Ion::ExamMode::get();
  }
  return m_examMode;
}

void Preferences::setExamMode(ExamMode mode) {
  if (m_examMode == mode) {
    return;
  }
  m_examMode = mode;
  Ion::ExamMode::set(mode);
}

}  // namespace Poincare
