#include "equation_store.h"
#include "../constant.h"
#include "../shared/poincare_helpers.h"
#include "../exam_mode_configuration.h"
#include "../global_preferences.h"
#include <limits.h>

#include <poincare/addition.h>
#include <poincare/constant.h>
#include <poincare/division.h>
#include <poincare/opposite.h>
#include <poincare/matrix.h>
#include <poincare/multiplication.h>
#include <poincare/polynomial.h>
#include <poincare/power.h>
#include <poincare/rational.h>
#include <poincare/square_root.h>
#include <poincare/subtraction.h>
#include <poincare/symbol.h>
#include <poincare/undefined.h>

using namespace Poincare;
using namespace Shared;

namespace Solver {

EquationStore::EquationStore() :
  ExpressionModelStore(),
  m_exactSolutionExactLayouts{},
  m_exactSolutionApproximateLayouts{},
  m_degree(-1),
  m_numberOfSolutions(0),
  m_numberOfUserVariables(0),
  m_type(Type::LinearSystem)
{
}

Ion::Storage::Record::ErrorStatus EquationStore::addEmptyModel() {
  char name[3] = {'e', '?', 0}; // name is going to be e0 or e1 or ... e5
  int currentNumber = 0;
  while (currentNumber < k_maxNumberOfEquations) {
    name[1] = '0'+currentNumber;
    if (Ion::Storage::sharedStorage()->recordBaseNamedWithExtension(name, Ion::Storage::eqExtension).isNull()) {
      break;
    }
    currentNumber++;
  }
  assert(currentNumber < k_maxNumberOfEquations);
  return Ion::Storage::sharedStorage()->createRecordWithExtension(name, Ion::Storage::eqExtension, nullptr, 0);
}

Shared::ExpressionModelHandle * EquationStore::setMemoizedModelAtIndex(int cacheIndex, Ion::Storage::Record record) const {
  assert(cacheIndex >= 0 && cacheIndex < maxNumberOfMemoizedModels());
  m_equations[cacheIndex] = Equation(record);
  return &m_equations[cacheIndex];
}

ExpressionModelHandle * EquationStore::memoizedModelAtIndex(int cacheIndex) const {
  assert(cacheIndex >= 0 && cacheIndex < maxNumberOfMemoizedModels());
  return &m_equations[cacheIndex];
}

void EquationStore::tidy() {
  ExpressionModelStore::tidy();
  tidySolution();
}

Poincare::Layout EquationStore::exactSolutionLayoutAtIndex(int i, bool exactLayout) {
  assert(m_type != Type::Monovariable && i >= 0 && (i < m_numberOfSolutions));
  if (exactLayout) {
    return m_exactSolutionExactLayouts[i];
  } else {
    return m_exactSolutionApproximateLayouts[i];
  }
}

double EquationStore::intervalBound(int index) const {
  assert(m_type == Type::Monovariable && index >= 0 && index < 2);
  return m_intervalApproximateSolutions[index];
}

void EquationStore::setIntervalBound(int index, double value) {
  assert(m_type == Type::Monovariable && index >= 0 && index < 2);
  m_intervalApproximateSolutions[index] = value;
  if (m_intervalApproximateSolutions[0] > m_intervalApproximateSolutions[1]) {
    if (index == 0) {
      m_intervalApproximateSolutions[1] = m_intervalApproximateSolutions[0]+1;
    } else {
      m_intervalApproximateSolutions[0] = m_intervalApproximateSolutions[1]-1;
    }
  }
}

void EquationStore::approximateSolve(Poincare::Context * context, bool shouldReplaceFunctionsButNotSymbols) {
  m_hasMoreThanMaxNumberOfApproximateSolution = false;
  Expression undevelopedExpression = modelForRecord(definedRecordAtIndex(0))->standardForm(context, shouldReplaceFunctionsButNotSymbols, ExpressionNode::ReductionTarget::SystemForApproximation);
  m_userVariablesUsed = !shouldReplaceFunctionsButNotSymbols;
  assert(m_variables[0][0] != 0 && m_variables[1][0] == 0);
  assert(m_type == Type::Monovariable);
  assert(m_intervalApproximateSolutions[0] <= m_intervalApproximateSolutions[1]);
  m_numberOfSolutions = 0;
  double start = m_intervalApproximateSolutions[0];
  double maximalStep = Poincare::Solver::DefaultMaximalStep(start, m_intervalApproximateSolutions[1]);
  /* In order to be able to call NextRoot on the previously found root, the
   * method cannot return its starting value. To allow the bound of the
   * interval to be part of the solutions, we need to start the search a
   * little sooner. */
  start -= maximalStep;
  /* The approximated value for a solution located on one of the bound could be
   * outside the interval, so we need to take a small margin. */
  double boundMargin = maximalStep * Poincare::Solver::k_zeroPrecision;
  double root;
  for (int i = 0; i <= k_maxNumberOfApproximateSolutions; i++) {
    root = PoincareHelpers::NextRoot(undevelopedExpression, m_variables[0], start, m_intervalApproximateSolutions[1], context, Poincare::Solver::k_relativePrecision, Poincare::Solver::k_minimalStep, maximalStep);

    /* Handle solutions found outside the reasonnable interval */
    if (root < m_intervalApproximateSolutions[0] - boundMargin) {
      /* A solution is found too soon, we simply ignore it and move on. */
      start = root;
      i--;
      continue;
    } else if (root > m_intervalApproximateSolutions[1] + boundMargin) {
      /* A solution is found but too late: there is no solution in the
       * interval, simply return NAN. */
      root = NAN;
    }

    if (i == k_maxNumberOfApproximateSolutions) {
      m_hasMoreThanMaxNumberOfApproximateSolution = !std::isnan(root);
      break;
    }
    m_approximateSolutions[i] = root;
    if (std::isnan(m_approximateSolutions[i])) {
      break;
    } else {
      start = m_approximateSolutions[i];
      m_numberOfSolutions++;
    }
  }
}

EquationStore::Error EquationStore::exactSolve(Poincare::Context * context, bool * replaceFunctionsButNotSymbols) {
  assert(replaceFunctionsButNotSymbols != nullptr);
  // First, solve the equation using predefined variables if there are
  *replaceFunctionsButNotSymbols = false;

  Error firstError = privateExactSolve(context, false);
  int firstNumberOfSolutions = numberOfSolutions();

  if (m_numberOfUserVariables > 0 && (firstError != Error::NoError || firstNumberOfSolutions == 0 || firstNumberOfSolutions == INT_MAX)) {
    // If there were predefined variables, and solution isn't good, try without
    *replaceFunctionsButNotSymbols = true;
    Error secondError = privateExactSolve(context, true);

    if (firstError == Error::NoError && secondError != Error::NoError && secondError != Error::RequireApproximateSolution) {
      // First solution was better, but has been tidied. It is recomputed.
      *replaceFunctionsButNotSymbols = false;
      return privateExactSolve(context, false);
    }
    return secondError;
  }
  return firstError;
}

/* Equations are solved according to the following procedure :
 * 1) We develop the equations using the reduction target "SystemForAnalysis".
 * This expands structures like Newton multinoms and allows us to detect
 * polynoms afterwards. ("(x+2)^2" in this form is not detected but is if
 * expanded).
 * 2) We look for classic forms of equations for which we have algorithms
 * that output the exact answer. If one is recognized in the input equation,
 * the exact answer is given to the user.
 * 3) If no classic form has been found in the developped form, we need to use
 * numerical approximation. Therefore, to prevent precision losses, we work
 * with the undevelopped form of the equation. Therefore we set reductionTarget
 * to SystemForApproximation. Solutions are then numericaly approximated
 * between the bounds provided by the user. */

EquationStore::Error EquationStore::privateExactSolve(Poincare::Context * context, bool replaceFunctionsButNotSymbols) {
  m_userVariablesUsed = !replaceFunctionsButNotSymbols;

  // Step 1. Get unknown and user-defined variables
  m_variables[0][0] = 0;
  int numberOfVariables = 0;

  // TODO we look twice for variables but not the same, is there a way to not do the same work twice?
  m_userVariables[0][0] = 0;
  m_numberOfUserVariables = 0;
  Expression simplifiedExpressions[k_maxNumberOfEquations];

  for (int i = 0; i < numberOfDefinedModels(); i++) {
    Shared::ExpiringPointer<Equation> eq = modelForRecord(definedRecordAtIndex(i));

    /* Start by looking for user variables, so that if we escape afterwards, we
     * know  if it might be due to a user variable. */
    if (m_numberOfUserVariables < Expression::k_maxNumberOfVariables) {
      const Expression eWithSymbols = eq->standardForm(context, true, ExpressionNode::ReductionTarget::SystemForAnalysis);
      /* If replaceFunctionsButNotSymbols is true we can memoize the expressions
       * for the rest of the function. Otherwise, we will memoize them at the
       * next call to standardForm */
      if (replaceFunctionsButNotSymbols == true) {
        simplifiedExpressions[i] = eWithSymbols;
      }
      int varCount = eWithSymbols.getVariables(context, [](const char * symbol, Poincare::Context * context) { return context->expressionTypeForIdentifier(symbol, strlen(symbol)) == Poincare::Context::SymbolAbstractType::Symbol; }, (char *)m_userVariables, Poincare::SymbolAbstract::k_maxNameSize, m_numberOfUserVariables);
      m_numberOfUserVariables = varCount < 0 ? Expression::k_maxNumberOfVariables : varCount;
    }
    if (simplifiedExpressions[i].isUninitialized()) {
      // The expression was not memoized before.
      simplifiedExpressions[i] = eq->standardForm(context, replaceFunctionsButNotSymbols, ExpressionNode::ReductionTarget::SystemForAnalysis);
    }
    const Expression e = simplifiedExpressions[i];
    if (e.isUninitialized() || e.type() == ExpressionNode::Type::Undefined || e.recursivelyMatches(Expression::IsMatrix, context, replaceFunctionsButNotSymbols ? ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions : ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition)) {
      return Error::EquationUndefined;
    }
    if (e.type() == ExpressionNode::Type::Unreal) {
      return Error::EquationUnreal;
    }
    numberOfVariables = e.getVariables(context, [](const char * symbol, Poincare::Context * context) { return true; }, (char *)m_variables, Poincare::SymbolAbstract::k_maxNameSize, numberOfVariables);
    if (numberOfVariables == -1) {
      return Error::TooManyVariables;
    }
    /* The equation has been parsed so there should be no
     * Error::VariableNameTooLong*/
    assert(numberOfVariables >= 0);
  }

  // Step 2. Linear System?

  /* Create matrix coefficients and vector constants as:
   *   coefficients * (x y z ...) = constants */
  Expression coefficients[k_maxNumberOfEquations][Expression::k_maxNumberOfVariables];
  Expression constants[k_maxNumberOfEquations];
  bool isLinear = true; // Invalid the linear system if one equation is non-linear
  Preferences * preferences = Preferences::sharedPreferences();
  for (int i = 0; i < numberOfDefinedModels(); i++) {
    isLinear = isLinear && simplifiedExpressions[i].getLinearCoefficients((char *)m_variables, Poincare::SymbolAbstract::k_maxNameSize, coefficients[i], &constants[i], context, updatedComplexFormat(context), preferences->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat(), replaceFunctionsButNotSymbols ? ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions : ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
    if (!isLinear) {
    // TODO: should we clean pool allocated memory if the system is not linear
#if 0
      for (int j = 0; j < i; j++) {
        for (int k = 0; k < numberOfVariables; k++) {
          coefficients[j][k] = Expression();
        }
        constants[j] = Expression();
      }
#endif
      if (numberOfDefinedModels() > 1 || numberOfVariables > 1) {
        return Error::NonLinearSystem;
      } else {
        break;
      }
    }
  }

  // Initialize result
  Expression exactSolutions[k_maxNumberOfExactSolutions];
  Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions];
  EquationStore::Error error;

  if (isLinear) {
    m_degree = 1;
    m_type = Type::LinearSystem;
    error = resolveLinearSystem(exactSolutions, exactSolutionsApproximations, coefficients, constants, context);
  } else {
    // Step 3. Polynomial & Monovariable?
    assert(numberOfVariables == 1 && numberOfDefinedModels() == 1);
    Expression polynomialCoefficients[Expression::k_maxNumberOfPolynomialCoefficients];
    m_degree = simplifiedExpressions[0]
      .getPolynomialReducedCoefficients(
          m_variables[0],
          polynomialCoefficients,
          context,
          updatedComplexFormat(context),
          preferences->angleUnit(),
          GlobalPreferences::sharedGlobalPreferences()->unitFormat(),
          replaceFunctionsButNotSymbols ?
            ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions :
            ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition);
    if (m_degree == 2 || m_degree == 3) {
      // Polynomial degree <= 3
      m_type = Type::PolynomialMonovariable;
      error = oneDimensialPolynomialSolve(exactSolutions, exactSolutionsApproximations, polynomialCoefficients, context);
    } else {
      // Step 4. Monovariable non-polynomial or polynomial with degree > 2
      m_type = Type::Monovariable;
      m_intervalApproximateSolutions[0] = -10;
      m_intervalApproximateSolutions[1] = 10;
      return Error::RequireApproximateSolution;
    }
  }
  // Create the results' layouts
  // Some exam mode configuration requires to display only approximate solutions
  int solutionIndex = 0;
  int initialNumberOfSolutions = m_numberOfSolutions <= k_maxNumberOfExactSolutions ? m_numberOfSolutions : -1;
  // We iterate through the solutions and the potential delta
  for (int i = 0; i < initialNumberOfSolutions; i++) {
    if (!exactSolutions[i].isUninitialized()) {
      assert(!exactSolutionsApproximations[i].isUninitialized());
      if (exactSolutionsApproximations[i].type() == ExpressionNode::Type::Unreal) {
        // Discard unreal solutions.
        m_numberOfSolutions--;
        continue;
      } else if (exactSolutionsApproximations[i].type() == ExpressionNode::Type::Undefined) {
        /* The solution is undefined, which means that the equation contained
         * unreduced undefined terms, such as a sequence or an integral. */
        m_numberOfSolutions--;
        error = Error::EquationUndefined;
        continue;
      }
      m_exactSolutionExactLayouts[solutionIndex] = PoincareHelpers::CreateLayout(exactSolutions[i]);
      m_exactSolutionApproximateLayouts[solutionIndex] = PoincareHelpers::CreateLayout(exactSolutionsApproximations[i]);
      // Check for identity between exact and approximate layouts
      char exactBuffer[::Constant::MaxSerializedExpressionSize];
      char approximateBuffer[::Constant::MaxSerializedExpressionSize];
      m_exactSolutionExactLayouts[solutionIndex].serializeForParsing(exactBuffer, ::Constant::MaxSerializedExpressionSize);
      m_exactSolutionApproximateLayouts[solutionIndex].serializeForParsing(approximateBuffer, ::Constant::MaxSerializedExpressionSize);
      /* Cheat: declare exact and approximate solutions to be identical in when
       * the exam mode forbids this exact solution to display only the
       * approximate solutions. */
      m_exactSolutionIdentity[solutionIndex] = ExamModeConfiguration::exactExpressionIsForbidden(GlobalPreferences::sharedGlobalPreferences()->examMode(), exactSolutions[i]) || strcmp(exactBuffer, approximateBuffer) == 0;
      if (!m_exactSolutionIdentity[solutionIndex]) {
        m_exactSolutionEquality[solutionIndex] = Expression::ParsedExpressionsAreEqual(exactBuffer, approximateBuffer, context, updatedComplexFormat(context), preferences->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat());
      }
      solutionIndex++;
    }
  }
  // Assert the remaining layouts had been properly tidied.
  assert(error != Error::NoError
     || solutionIndex >= k_maxNumberOfExactSolutions
     || (m_exactSolutionExactLayouts[solutionIndex].isUninitialized()
      && m_exactSolutionApproximateLayouts[solutionIndex].isUninitialized()));
  return error;
}

EquationStore::Error EquationStore::resolveLinearSystem(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[k_maxNumberOfEquations][Expression::k_maxNumberOfVariables], Expression constants[k_maxNumberOfEquations], Context * context) {
  Preferences::AngleUnit angleUnit = Preferences::sharedPreferences()->angleUnit();
  Preferences::UnitFormat unitFormat = GlobalPreferences::sharedGlobalPreferences()->unitFormat();
  // n unknown variables
  int n = 0;
  while (n < Expression::k_maxNumberOfVariables && m_variables[n][0] != 0) {
    n++;
  }
  int m = numberOfDefinedModels(); // m equations
  /* Create the matrix (A | b) for the equation Ax=b */
  Matrix Ab = Matrix::Builder();
  for (int i = 0; i < m; i++) {
    for (int j = 0; j < n; j++) {
      Ab.addChildAtIndexInPlace(coefficients[i][j], Ab.numberOfChildren(), Ab.numberOfChildren());
    }
    Ab.addChildAtIndexInPlace(constants[i], Ab.numberOfChildren(), Ab.numberOfChildren());
  }
  Ab.setDimensions(m, n+1);

  // Compute the rank of (A | b)
  int rankAb = Ab.rank(context, updatedComplexFormat(context), angleUnit, unitFormat, true);

  // Initialize the number of solutions
  m_numberOfSolutions = INT_MAX;
  /* If the matrix has one null row except the last column, the system is
   * inconsistent (equivalent to 0 = x with x non-null */
  for (int j = m-1; j >= 0; j--) {
    bool rowWithNullCoefficients = true;
    for (int i = 0; i < n; i++) {
      if (Ab.matrixChild(j, i).nullStatus(context) != ExpressionNode::NullStatus::Null) {
        rowWithNullCoefficients = false;
        break;
      }
    }
    if (rowWithNullCoefficients && Ab.matrixChild(j, n).nullStatus(context) != ExpressionNode::NullStatus::Null) {
      // TODO: Handle ExpressionNode::NullStatus::Unknown
      m_numberOfSolutions = 0;
    }
  }
  if (m_numberOfSolutions > 0) {
    // if rank(A | b) < n, the system has an infinite number of solutions
    if (rankAb == n && n > 0) {
      // Otherwise, the system has n solutions which correspond to the last column
      m_numberOfSolutions = n;
      for (int i = 0; i < m_numberOfSolutions; i++) {
        exactSolutions[i] = Expression();
        Ab.matrixChild(i,n).simplifyAndApproximate(exactSolutions + i, exactSolutionsApproximations + i, context, updatedComplexFormat(context), angleUnit, unitFormat);
      }
    }
  }
  return Error::NoError;
}

EquationStore::Error EquationStore::oneDimensialPolynomialSolve(Expression exactSolutions[k_maxNumberOfExactSolutions], Expression exactSolutionsApproximations[k_maxNumberOfExactSolutions], Expression coefficients[Expression::k_maxNumberOfPolynomialCoefficients], Context * context) {
  /* Equation ax^2+bx+c = 0 */
  Expression delta;
  bool solutionsAreApproximated = false;
  if (m_degree == 2) {
    m_numberOfSolutions = Poincare::Polynomial::QuadraticPolynomialRoots(coefficients[2], coefficients[1], coefficients[0], exactSolutions, exactSolutions + 1, &delta, context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit());
  } else {
    assert(m_degree == 3);
    m_numberOfSolutions = Poincare::Polynomial::CubicPolynomialRoots(coefficients[3], coefficients[2], coefficients[1], coefficients[0], exactSolutions, exactSolutions + 1, exactSolutions + 2, &delta, context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit(), &solutionsAreApproximated);
  }
  exactSolutions[m_numberOfSolutions++] = delta;
  for (int i = 0; i < m_numberOfSolutions; i++) {
    Expression exactSolutionClone = exactSolutions[i].clone();
    if (solutionsAreApproximated) {
      exactSolutionsApproximations[i] = exactSolutionClone;
    } else {
      exactSolutions[i] = Expression();
      exactSolutionClone.simplifyAndApproximate(exactSolutions + i, exactSolutionsApproximations + i, context, updatedComplexFormat(context), Poincare::Preferences::sharedPreferences()->angleUnit(), GlobalPreferences::sharedGlobalPreferences()->unitFormat());
    }
  }
  return Error::NoError;
}

void EquationStore::tidySolution() {
  for (int i = 0; i < k_maxNumberOfExactSolutions; i++) {
    m_exactSolutionExactLayouts[i] = Layout();
    m_exactSolutionApproximateLayouts[i] = Layout();
  }
}

Preferences::ComplexFormat EquationStore::updatedComplexFormat(Context * context) {
  Preferences::ComplexFormat complexFormat = Preferences::sharedPreferences()->complexFormat();
  if (complexFormat == Preferences::ComplexFormat::Real && isExplictlyComplex(context)) {
    return Preferences::ComplexFormat::Cartesian;
  }
  return complexFormat;
}

bool EquationStore::isExplictlyComplex(Context * context) {
  for (int i = 0; i < numberOfDefinedModels(); i++) {
    // Ignore defined symbols if user variables are not used
    if (modelForRecord(definedRecordAtIndex(i))->containsIComplex(context, m_userVariablesUsed ? ExpressionNode::SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition : ExpressionNode::SymbolicComputation::ReplaceDefinedFunctionsWithDefinitions)) {
      return true;
    }
  }
  return false;
}

}
