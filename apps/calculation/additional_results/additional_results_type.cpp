#include "additional_results_type.h"

#include <apps/apps_container_helper.h>
#include <poincare/preferences.h>
#include <poincare/trigonometry.h>
#include <poincare/unit.h>

#include "../calculation.h"
#include "scientific_notation_helper.h"
#include "trigonometry_helper.h"
#include "unit_comparison_helper.h"
#include "vector_helper.h"

using namespace Poincare;
using namespace Shared;

namespace Calculation {

AdditionalResultsType AdditionalResultsType::AdditionalResultsForExpressions(
    const Expression input, const Expression exactOutput,
    const Expression approximateOutput,
    const Preferences::CalculationPreferences calculationPreferences) {
  if (ForbidAdditionalResults(input, exactOutput, approximateOutput)) {
    return AdditionalResultsType{.empty = true};
  }
  if (HasComplex(approximateOutput, calculationPreferences)) {
    return AdditionalResultsType{.complex = true};
  }
  if (exactOutput.isScalarComplex(calculationPreferences)) {
    // Cf comment in HasComplex
    return AdditionalResultsType{.empty = true};
  }
  bool inputHasAngleUnit, exactHasAngleUnit, approximateHasAngleUnit;
  bool inputHasUnit = input.hasUnit(true, &inputHasAngleUnit);
  bool exactHasUnit = exactOutput.hasUnit(true, &exactHasAngleUnit);
  bool approximateHasUnit =
      approximateOutput.hasUnit(true, &approximateHasAngleUnit);
  assert(exactHasUnit == approximateHasUnit);
  (void)approximateHasUnit;  // Silence compiler
  if (inputHasUnit || exactHasUnit) {
    /* We display units additional results based on exact output. If input has
     * units but not output (ex: L/(L/3)), we don't display any results. */
    return exactHasUnit && HasUnit(exactOutput, calculationPreferences)
               ? AdditionalResultsType{.unit = true}
               : AdditionalResultsType{.empty = true};
  }
  if (HasDirectTrigo(input, exactOutput, calculationPreferences)) {
    return AdditionalResultsType{.directTrigonometry = true};
  }
  if (HasInverseTrigo(input, exactOutput, calculationPreferences)) {
    return AdditionalResultsType{.inverseTrigonometry = true};
  }
  if (HasVector(exactOutput, approximateOutput, calculationPreferences)) {
    return AdditionalResultsType{.vector = true};
  }
  if (approximateOutput.deepIsMatrix()) {
    return HasMatrix(approximateOutput) ? AdditionalResultsType{.matrix = true}
                                        : AdditionalResultsType{.empty = true};
  }
  if (exactHasAngleUnit || approximateHasAngleUnit) {
    return exactHasAngleUnit && HasUnit(exactOutput, calculationPreferences)
               ? AdditionalResultsType{.unit = true}
               : AdditionalResultsType{.empty = true};
  }
  AdditionalResultsType type = {};
  if (!inputHasAngleUnit && HasFunction(input, approximateOutput)) {
    type.function = true;
  }
  if (HasScientificNotation(approximateOutput)) {
    type.scientificNotation = true;
  }
  if (HasInteger(exactOutput)) {
    type.integer = true;
  } else if (HasRational(exactOutput)) {
    type.rational = true;
  }
  if (type.isUninitialized()) {
    type.empty = true;
  }
  assert(!type.isUninitialized());
  return type;
}

bool AdditionalResultsType::ForbidAdditionalResults(
    const Expression input, const Expression exactOutput,
    const Expression approximateOutput) {
  /* Special case for Store:
   * Store nodes have to be at the root of the expression, which prevents
   * from creating new expressions with store node as a child. We don't
   * return any additional outputs for them to avoid bothering with special
   * cases. */
  if (Preferences::sharedPreferences->examMode().forbidAdditionalResults() ||
      input.isUninitialized() || exactOutput.isUninitialized() ||
      approximateOutput.isUninitialized() ||
      input.type() == ExpressionNode::Type::Store ||
      exactOutput.type() == ExpressionNode::Type::List ||
      approximateOutput.type() == ExpressionNode::Type::List ||
      approximateOutput.recursivelyMatches([](const Expression e) {
        return e.isUndefined() || e.type() == ExpressionNode::Type::Infinity;
      })) {
    return true;
  }
  assert(!input.isUndefined() && !exactOutput.isUndefined());
  return false;
}

bool AdditionalResultsType::HasComplex(
    const Expression approximateOutput,
    const Preferences::CalculationPreferences calculationPreferences) {
  /* We have 2 edge cases:
   * 1) exact output assessed to scalar complex but not approximate output
   * ex:
   *    In polar format, for input -10 the exact output is 10e^(iπ) and the
   *    approximate output is 10e^(3.14i). Due to rounding errors, the imaginary
   *    part of the approximate output is not zero so it is considered as scalar
   *    complex, while the exact output is not.
   * 2) approximate output assessed to scalar complex but not exact output
   * ex:
   *    For input i^(2×e^(7i^(2×e^322))), the exact output approximates to a
   *    complex with very small norm but PrintFloat::ConvertFloatToTextPrivate
   *    rounds it to 0 so the approximation output is 0i. Thus, the imaginary
   *    part of the approximate output is zero so it is not considered as scalar
   *    complex, while the exact output is.
   * We chosed to handle the 2nd case and not to display any additional results
   * in the 1st case. */
  return approximateOutput.isScalarComplex(calculationPreferences);
}

bool AdditionalResultsType::HasDirectTrigo(
    const Expression input, const Expression exactOutput,
    const Preferences::CalculationPreferences calculationPreferences) {
  assert(!exactOutput.hasUnit(true));
  Context *globalContext =
      AppsContainerHelper::sharedAppsContainerGlobalContext();
  Expression exactAngle = TrigonometryHelper::ExtractExactAngleFromDirectTrigo(
      input, exactOutput, globalContext, calculationPreferences);
  return !exactAngle.isUninitialized();
}

bool AdditionalResultsType::HasInverseTrigo(
    const Expression input, const Expression exactOutput,
    const Preferences::CalculationPreferences calculationPreferences) {
  // If the result is complex, it is treated as a complex result instead.
  assert(!exactOutput.isScalarComplex(calculationPreferences));
  assert(!exactOutput.hasUnit(true));
  return (Trigonometry::IsInverseTrigonometryFunction(input)) ||
         Trigonometry::IsInverseTrigonometryFunction(exactOutput);
}

bool AdditionalResultsType::HasUnit(
    const Expression exactOutput,
    const Preferences::CalculationPreferences calculationPreferences) {
  assert(exactOutput.hasUnit());
  Context *globalContext =
      AppsContainerHelper::sharedAppsContainerGlobalContext();
  Preferences::ComplexFormat complexFormat =
      calculationPreferences.complexFormat();
  Preferences::AngleUnit angleUnit = calculationPreferences.angleUnit();
  Expression unit;
  Expression clone = exactOutput.clone();
  PoincareHelpers::CloneAndReduceAndRemoveUnit(
      &clone, &unit, globalContext,
      {.complexFormat = complexFormat,
       .angleUnit = angleUnit,
       .symbolicComputation =
           SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined,
       .unitConversion = UnitConversion::None});
  double value = PoincareHelpers::ApproximateToScalar<double>(
      clone, globalContext,
      {.complexFormat = complexFormat, .angleUnit = angleUnit});
  if (!unit.isUninitialized() &&
      (Unit::ShouldDisplayAdditionalOutputs(
           value, unit,
           GlobalPreferences::sharedGlobalPreferences->unitFormat()) ||
       UnitComparison::ShouldDisplayUnitComparison(value, unit))) {
    /* Sometimes with angle units, the reduction with UnitConversion::None
     * will be defined but not the reduction with UnitConversion::Default,
     * which will make the unit list controller crash.  */
    unit = Expression();
    clone = exactOutput.clone();
    PoincareHelpers::CloneAndReduceAndRemoveUnit(
        &clone, &unit, globalContext,
        {.complexFormat = complexFormat, .angleUnit = angleUnit});
    return !unit.isUninitialized();
  }
  return false;
}

bool AdditionalResultsType::HasVector(
    const Expression exactOutput, const Expression approximateOutput,
    const Preferences::CalculationPreferences calculationPreferences) {
  Context *globalContext =
      AppsContainerHelper::sharedAppsContainerGlobalContext();
  Expression norm = VectorHelper::BuildVectorNorm(
      exactOutput.clone(), globalContext, calculationPreferences);
  if (norm.isUninitialized()) {
    return false;
  }
  assert(!norm.isUndefined());
  int nChildren = approximateOutput.numberOfChildren();
  for (int i = 0; i < nChildren; ++i) {
    if (approximateOutput.childAtIndex(i).isScalarComplex(
            calculationPreferences)) {
      return false;
    }
  }
  return true;
}

bool AdditionalResultsType::HasMatrix(const Expression approximateOutput) {
  assert(!approximateOutput.isUninitialized());
  assert(!approximateOutput.hasUnit(true));
  return approximateOutput.type() == ExpressionNode::Type::Matrix &&
         !approximateOutput.recursivelyMatches(Expression::IsUndefined);
}

static bool expressionIsInterestingFunction(const Expression e) {
  assert(!e.isUninitialized());
  if (e.isOfType({ExpressionNode::Type::Opposite,
                  ExpressionNode::Type::Parenthesis})) {
    return expressionIsInterestingFunction(e.childAtIndex(0));
  }
  return !e.isNumber() &&
         !e.isOfType({ExpressionNode::Type::ConstantMaths,
                      ExpressionNode::Type::UnitConvert}) &&
         !e.deepIsOfType({ExpressionNode::Type::Sequence,
                          ExpressionNode::Type::Factor,
                          ExpressionNode::Type::RealPart,
                          ExpressionNode::Type::ImaginaryPart,
                          ExpressionNode::Type::ComplexArgument,
                          ExpressionNode::Type::Conjugate}) &&
         e.numberOfNumericalValues() == 1;
}

bool AdditionalResultsType::HasFunction(const Expression input,
                                        const Expression approximateOutput) {
  // We want a single numerical value and to avoid showing the identity function
  assert(!input.isUninitialized());
  assert(!input.hasUnit());
  assert(!approximateOutput.isUndefined());
  assert(!approximateOutput.hasUnit());
  assert(approximateOutput.type() != ExpressionNode::Type::Matrix);
  return approximateOutput.type() != ExpressionNode::Type::Nonreal &&
         expressionIsInterestingFunction(input);
}

bool AdditionalResultsType::HasScientificNotation(
    const Expression approximateOutput) {
  assert(!approximateOutput.isUninitialized());
  assert(!approximateOutput.hasUnit());
  Context *globalContext =
      AppsContainerHelper::sharedAppsContainerGlobalContext();
  if (approximateOutput.type() == ExpressionNode::Type::Nonreal ||
      Preferences::sharedPreferences->displayMode() ==
          Preferences::PrintFloatMode::Scientific) {
    return false;
  }
  Layout historyResult = approximateOutput.createLayout(
      Preferences::sharedPreferences->displayMode(),
      Preferences::sharedPreferences->numberOfSignificantDigits(),
      globalContext);
  return !historyResult.isIdenticalTo(
      ScientificNotationHelper::ScientificLayout(approximateOutput,
                                                 globalContext));
}

bool AdditionalResultsType::HasInteger(const Expression exactOutput) {
  assert(!exactOutput.isUninitialized());
  assert(!exactOutput.hasUnit());
  constexpr const char *k_maximalIntegerWithAdditionalResults =
      "10000000000000000";
  return exactOutput.isBasedIntegerCappedBy(
      k_maximalIntegerWithAdditionalResults);
}

bool AdditionalResultsType::HasRational(const Expression exactOutput) {
  // Find forms like [12]/[23] or -[12]/[23]
  assert(!exactOutput.isUninitialized());
  assert(!exactOutput.hasUnit());
  return exactOutput.isDivisionOfIntegers() ||
         (exactOutput.type() == ExpressionNode::Type::Opposite &&
          exactOutput.childAtIndex(0).isDivisionOfIntegers());
}

}  // namespace Calculation
