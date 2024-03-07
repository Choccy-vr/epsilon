#include "continuous_function.h"

#include <apps/apps_container_helper.h>
#include <escher/palette.h>
#include <poincare/based_integer.h>
#include <poincare/cosine.h>
#include <poincare/derivative.h>
#include <poincare/division.h>
#include <poincare/float.h>
#include <poincare/function.h>
#include <poincare/helpers.h>
#include <poincare/integral.h>
#include <poincare/list_sort.h>
#include <poincare/matrix.h>
#include <poincare/multiplication.h>
#include <poincare/point_evaluation.h>
#include <poincare/polynomial.h>
#include <poincare/print.h>
#include <poincare/serialization_helper.h>
#include <poincare/sine.h>
#include <poincare/string_layout.h>
#include <poincare/subtraction.h>
#include <poincare/symbol.h>
#include <poincare/symbol_abstract.h>
#include <poincare/trigonometry.h>
#include <poincare/undefined.h>
#include <poincare/zoom.h>

#include <algorithm>

#include "global_context.h"
#include "poincare_helpers.h"

using namespace Poincare;

namespace Shared {

/* ContinuousFunction - Public */

ContinuousFunction ContinuousFunction::NewModel(
    Ion::Storage::Record::ErrorStatus *error, const char *baseName,
    KDColor color) {
  assert(baseName != nullptr);
  // Create the record
  /* WARNING: We create an empty record with the baseName and extension right
   * away because the baseName pointer might be corrupted later on.
   * Indeed, the baseName can point in the Pool, which can be altered during
   * the record creation, because the FunctionStore will have its memoization
   * reset.
   * Creating an empty record with the base name computes and saves the CRC32
   * of the name, so that the baseName variable is not needed anymore after
   * calling the method "createRecordWithExtension". */
  Ion::Storage::Record record =
      Ion::Storage::Record(baseName, Ion::Storage::functionExtension);
  RecordDataBuffer data(color);
  *error =
      Ion::Storage::FileSystem::sharedFileSystem->createRecordWithExtension(
          baseName, Ion::Storage::functionExtension, &data, sizeof(data));
  // Return the ContinuousFunction with the new record
  return ContinuousFunction(
      *error == Ion::Storage::Record::ErrorStatus::None ? record : Record());
}

ContinuousFunction::ContinuousFunction(Ion::Storage::Record record)
    : Function(record), m_cache(nullptr) {
  /* The name of the record might need an update after another expression
   * on which this one depended was updated.
   * Since each time a record is updated, all ContinuousFunctions are tidied
   * and will be re-built later, it's safe to do it in this constructor.
   * Example:
   *  Expression 1: f(x)=x^2
   *  Expression 2: f(x)=x
   *  Here f(x)=x is treated as the equation x^2=x and its record name is ?1.
   *  If Expression 1 is modified into: g(x)=x^2, the Expression 2 should
   *  now be updated as the function f(x), so the record needs a renaming.
   * */
  if (!record.isNull()) {
    Ion::Storage::Record::ErrorStatus error = updateNameIfNeeded(
        AppsContainerHelper::sharedAppsContainerGlobalContext());
    assert(error == Ion::Storage::Record::ErrorStatus::None);
    (void)error;
  }
}

ContinuousFunctionProperties ContinuousFunction::properties() const {
  if (!m_model.properties().isInitialized()) {
    // Computing the expression equation will update the function properties
    expressionReducedForAnalysis(
        AppsContainerHelper::sharedAppsContainerGlobalContext());
  }
  assert(m_model.properties().isInitialized());
  return m_model.properties();
}

Ion::Storage::Record::ErrorStatus ContinuousFunction::updateNameIfNeeded(
    Context *context) {
  return m_model.renameRecordIfNeeded(this, context);
}

size_t ContinuousFunction::nameWithoutArgument(char *buffer, size_t bufferSize,
                                               int derivationOrder) {
  assert(0 <= derivationOrder && derivationOrder <= 2);
  size_t length = 0;
  if (isNamed()) {
    length += Function::name(buffer, bufferSize);
  } else {
    length += SerializationHelper::CodePoint(
        buffer, bufferSize,
        properties().isPolar()
            ? k_radiusSymbol
            : (properties().isInversePolar() ? k_polarSymbol
                                             : k_ordinateSymbol));
  }
  if (derivationOrder > 0) {
    const CodePoint derivative = derivationOrder == 1
                                     ? DerivativeNode::k_firstDerivativeSymbol
                                     : DerivativeNode::k_secondDerivativeSymbol;
    length += SerializationHelper::CodePoint(buffer + length,
                                             bufferSize - length, derivative);
  }
  return length;
}

size_t ContinuousFunction::nameWithArgument(char *buffer, size_t bufferSize,
                                            int derivationOrder) {
  assert(0 <= derivationOrder && derivationOrder <= 2);
  size_t length = nameWithoutArgument(buffer, bufferSize, derivationOrder);
  if (isNamed()) {
    length += withArgument(buffer + length, bufferSize - length);
  }
  return length;
}

size_t ContinuousFunction::printAbscissaValue(double cursorT, double cursorX,
                                              char *buffer, size_t bufferSize,
                                              int precision) {
  ContinuousFunctionProperties thisProperties = properties();
  /* With Vertical curves, cursorT != cursorX .
   * We need the value for symbol=... */
  return PoincareHelpers::ConvertFloatToText<double>(
      thisProperties.isCartesian() || thisProperties.isScatterPlot() ? cursorX
                                                                     : cursorT,
      buffer, bufferSize, precision);
}

size_t ContinuousFunction::printFunctionValue(double cursorT, double cursorX,
                                              double cursorY, char *buffer,
                                              size_t bufferSize, int precision,
                                              Context *context) {
  ContinuousFunctionProperties thisProperties = properties();
  if (thisProperties.isParametric()) {
    Preferences::PrintFloatMode mode =
        Poincare::Preferences::sharedPreferences->displayMode();
    return Poincare::Print::CustomPrintf(buffer, bufferSize, "(%*.*ed;%*.*ed)",
                                         cursorX, mode, precision, cursorY,
                                         mode, precision);
  }
  double value = thisProperties.isPolar() || thisProperties.isInversePolar()
                     ? evaluate2DAtParameter(cursorT, context).y()
                     : cursorY;
  return PoincareHelpers::ConvertFloatToText<double>(value, buffer, bufferSize,
                                                     precision);
}

Ion::Storage::Record::ErrorStatus ContinuousFunction::setContent(
    const char *c, Context *context) {
  setCache(nullptr);
  bool wasCartesian = properties().isCartesian();
  /* About to set the content, the symbol does not matter here yet. We don't use
   * ExpressionModelHandle::setContent implementation to avoid calling symbol()
   * and any unnecessary plot type update at this point. See comment in
   * ContinuousFunction::Model::buildExpressionFromText. */
  Ion::Storage::Record::ErrorStatus error =
      m_model.setContent(this, c, context, k_unnamedExpressionSymbol);
  if (error == Ion::Storage::Record::ErrorStatus::None && !isNull()) {
    // Set proper name
    error = updateNameIfNeeded(context);
    // Update model
    updateModel(context, wasCartesian);
  }
  return error;
}

void ContinuousFunction::tidyDownstreamPoolFrom(
    TreeNode *treePoolCursor) const {
  ExpressionModelHandle::tidyDownstreamPoolFrom(treePoolCursor);
  m_cache = nullptr;
}

bool ContinuousFunction::isNamed() const {
  // Unnamed functions have a fullname starting with k_unnamedRecordFirstChar
  const char *recordFullName = fullName();
  return recordFullName != nullptr &&
         recordFullName[0] != k_unnamedRecordFirstChar;
}

bool ContinuousFunction::isDiscontinuousBetweenFloatValues(
    float x1, float x2, Poincare::Context *context) const {
  Expression equation = expressionReduced(context);
  ApproximationContext approximationContext(context, complexFormat(context));
  return equation.isDiscontinuousBetweenValuesForSymbol(k_unknownName, x1, x2,
                                                        approximationContext);
}

void ContinuousFunction::getLineParameters(double *slope, double *intercept,
                                           Context *context) const {
  assert(properties().isLine());
  Expression equation = expressionReduced(context);
  // Compute metrics for details view of Line
  Expression coefficients[Expression::k_maxNumberOfPolynomialCoefficients];
  // Separate the two line coefficients for approximation.
  int d = equation.getPolynomialReducedCoefficients(
      k_unknownName, coefficients, context, complexFormat(context),
      Poincare::Preferences::sharedPreferences->angleUnit(),
      ContinuousFunctionProperties::k_defaultUnitFormat,
      SymbolicComputation::ReplaceAllSymbolsWithDefinitionsOrUndefined);
  assert(d <= 1);
  /* Degree might vary depending on symbols definition and complex format.
   * Approximate and return the two line coefficients */
  if (d < 0) {
    *slope = NAN;
    *intercept = NAN;
  } else {
    ApproximationContext approximationContext(context, complexFormat(context));
    *intercept =
        coefficients[0].approximateToScalar<double>(approximationContext);
    if (d == 0) {
      *slope = 0.0;
    } else {
      *slope =
          coefficients[1].approximateToScalar<double>(approximationContext);
    }
  }
}

CartesianConic ContinuousFunction::cartesianConicParameters(
    Context *context) const {
  assert(properties().isConic() && properties().isCartesian());
  return CartesianConic(expressionReducedForAnalysis(context), context,
                        complexFormat(context), k_unknownName);
}

double ContinuousFunction::evaluateCurveParameter(int index, double cursorT,
                                                  double cursorX,
                                                  double cursorY,
                                                  Context *context) const {
  switch (properties().symbolType()) {
    case ContinuousFunctionProperties::SymbolType::T:
      return index == 0   ? cursorT
             : index == 1 ? evaluateXYAtParameter(cursorT, context).x()
                          : evaluateXYAtParameter(cursorT, context).y();
    case ContinuousFunctionProperties::SymbolType::Theta:
    case ContinuousFunctionProperties::SymbolType::Radius:
      return index == 0 ? cursorT : evaluate2DAtParameter(cursorT, context).y();
    default:
      return index == 0 ? cursorX : cursorY;
  }
}

void ContinuousFunction::updateModel(Context *context, bool wasCartesian) {
  setCache(nullptr);
  m_model.resetProperties();  // Reset model's properties.
  properties();               // update properties.
  assert(m_model.properties().isInitialized());
  if (wasCartesian != properties().isCartesian() ||
      !properties().canHaveCustomDomain()) {
    // The definition's domain must be reset.
    setTAuto(true);
  }
}

template <typename T>
Evaluation<T> ContinuousFunction::approximateDerivative(T t, Context *context,
                                                        int derivationOrder,
                                                        bool useDomain) const {
  assert(canDisplayDerivative());
  assert(!isAlongY());
  assert(numberOfSubCurves() == 1);
  if (useDomain && (t < tMin() || t > tMax())) {
    if (properties().isParametric()) {
      return PointEvaluation<T>::Builder(NAN, NAN);
    }
    return Complex<T>::RealUndefined();
  }
  // Derivative is simplified once and for all
  Expression derivate = expressionDerivateReduced(context, derivationOrder);
  ApproximationContext approximationContext(context, complexFormat(context));
  Evaluation<T> result = derivate.approximateWithValueForSymbol(
      k_unknownName, t, approximationContext);
  return result;
}

double ContinuousFunction::approximateSlope(double t,
                                            Poincare::Context *context) const {
  if (t < tMin() || t > tMax()) {
    return NAN;
  }
  // Slope is simplified once and for all
  Expression slope = expressionSlopeReduced(context);
  ApproximationContext approximationContext(context, complexFormat(context));
  Evaluation<double> result = slope.approximateWithValueForSymbol(
      k_unknownName, t, approximationContext);
  assert(result.type() == EvaluationNode<double>::Type::Complex);
  return result.toScalar();
}

void ContinuousFunction::setTMin(float tMin) {
  assert(!recordData()->tAuto());
  recordData()->setTMin(tMin);
  setCache(nullptr);
}

void ContinuousFunction::setTMax(float tMax) {
  assert(!recordData()->tAuto());
  recordData()->setTMax(tMax);
  setCache(nullptr);
}

void ContinuousFunction::setTAuto(bool tAuto) {
  if (!tAuto && !recordData()->tAuto()) {
    return;
  }
  /* Domain either was or will be auto. Reset values anyway in case model has
   * been updated or angle unit changed. */
  recordData()->setTAuto(tAuto);
  setCache(nullptr);
  if (tAuto) {
    // No need to update Tmin or Tmax since the auto value will be returned
    return;
  }
  recordData()->setTMin(autoTMin());
  recordData()->setTMax(autoTMax());
}

float ContinuousFunction::autoTMax() const {
  return properties().isCartesian() || properties().isScatterPlot()
             ? INFINITY
             : (properties().isInversePolar()
                    ? Range1D<float>::k_defaultHalfLength
                    : 2.f * Trigonometry::PiInAngleUnit(
                                Preferences::sharedPreferences->angleUnit()));
}

float ContinuousFunction::autoTMin() const {
  return properties().isCartesian()
             ? -INFINITY
             : (properties().isInversePolar()
                    ? -Range1D<float>::k_defaultHalfLength
                    : 0.f);
}

bool ContinuousFunction::approximationBasedOnCostlyAlgorithms(
    Context *context) const {
  return expressionApproximated(context).recursivelyMatches(
      [](const Expression e) {
        return !e.isUninitialized() &&
               e.isOfType({ExpressionNode::Type::Sequence,
                           ExpressionNode::Type::Integral,
                           ExpressionNode::Type::Derivative});
      });
}

void ContinuousFunction::trimResolutionInterval(double *start,
                                                double *end) const {
  double tmin = tMin(), tmax = tMax();
  *start = *start < tmin ? tmin : tmax < *start ? tmax : *start;
  *end = *end < tmin ? tmin : tmax < *end ? tmax : *end;
}

Expression ContinuousFunction::sumBetweenBounds(double start, double end,
                                                Context *context) const {
  assert(properties().isCartesian());
  start = std::max<double>(start, tMin());
  end = std::min<double>(end, tMax());
  // Integral takes ownership of args
  return Integral::Builder(
      expressionReduced(context).clone(), Symbol::SystemSymbol(),
      Float<double>::Builder(start), Float<double>::Builder(end));
  /* TODO: when we approximate integral, we might want to simplify the integral
   * here. However, we might want to do it once for all x (to avoid lagging in
   * the derivative table. */
}

/* ContinuousFunction - Private */

float ContinuousFunction::rangeStep() const {
  return properties().isCartesian()
             ? NAN
             : (tMax() - tMin()) / k_polarParamRangeSearchNumberOfPoints;
}

template <typename T>
Coordinate2D<T> ContinuousFunction::privateEvaluateXYAtParameter(
    T t, Context *context, int subCurveIndex) const {
  ContinuousFunctionProperties thisProperties = properties();
  Coordinate2D<T> x1x2 =
      templatedApproximateAtParameter(t, context, subCurveIndex);
  if (thisProperties.isParametric() || thisProperties.isCartesian() ||
      thisProperties.isScatterPlot()) {
    return x1x2;
  }
  assert(thisProperties.isPolar() || thisProperties.isInversePolar());
  const T r = thisProperties.isPolar() ? x1x2.y() : x1x2.x();
  const T angle = (thisProperties.isPolar() ? x1x2.x() : x1x2.y()) * M_PI /
                  Trigonometry::PiInAngleUnit(
                      Poincare::Preferences::sharedPreferences->angleUnit());
  return Coordinate2D<T>(r * std::cos(angle), r * std::sin(angle));
}

template <typename T>
Coordinate2D<T> ContinuousFunction::templatedApproximateAtParameter(
    T t, Context *context, int subCurveIndex) const {
  if (t < tMin() || t > tMax()) {
    return Coordinate2D<T>(properties().isCartesian() ? t : NAN, NAN);
  }
  int derivationOrder =
      subCurveIndex >= numberOfSubCurves() ? subCurveIndex : 0;
  Expression e = expressionApproximated(context, derivationOrder);
  ApproximationContext approximationContext(context, complexFormat(context));

  if (properties().isScatterPlot()) {
    Expression point;
    if (Expression::IsPoint(e)) {
      if (t != static_cast<T>(0.)) {
        return Coordinate2D<T>();
      }
      point = e;
    } else {
      assert(e.type() == ExpressionNode::Type::List);
      int tInt = t;
      if (static_cast<T>(tInt) != t || tInt < 0 ||
          tInt >= e.numberOfChildren()) {
        return Coordinate2D<T>();
      }
      point = point = e.childAtIndex(tInt);
    }
    assert(!point.isUninitialized() && Expression::IsPoint(point));
    if (point.isUndefined()) {
      return Coordinate2D<T>();
    }
    return static_cast<Point &>(point).approximate2D<T>(approximationContext);
  }

  if (!properties().isParametric()) {
    if (numberOfSubCurves() >= 2) {
      assert(derivationOrder == 0);
      assert(e.numberOfChildren() > subCurveIndex);
      e = e.childAtIndex(subCurveIndex);
    }
    T value = e.approximateToScalarWithValueForSymbol(k_unknownName, t,
                                                      approximationContext);
    if (isAlongY()) {
      // Invert x and y with vertical lines so it can be scrolled vertically
      return Coordinate2D<T>(value, t);
    }
    return Coordinate2D<T>(t, value);
  }
  if (e.type() == ExpressionNode::Type::Dependency) {
    e = e.childAtIndex(0);
  }
  if (e.isUndefined()) {
    return Coordinate2D<T>(NAN, NAN);
  }
  assert(e.type() == ExpressionNode::Type::Point);
  assert(e.numberOfChildren() == 2);
  return Coordinate2D<T>(
      e.childAtIndex(0).approximateToScalarWithValueForSymbol(
          k_unknownName, t, approximationContext),
      e.childAtIndex(1).approximateToScalarWithValueForSymbol(
          k_unknownName, t, approximationContext));
}

ContinuousFunction::RecordDataBuffer::RecordDataBuffer(KDColor color)
    : Shared::Function::RecordDataBuffer(color),
      m_domain(-INFINITY, INFINITY),
      m_derivativesOptions{},
      m_tAuto(true) {
  m_derivativesOptions.colorFirstDerivative = color;
  m_derivativesOptions.colorSecondDerivative = color;
}

KDColor ContinuousFunction::RecordDataBuffer::color(int derivationOrder) const {
  switch (derivationOrder) {
    case 0:
      return Function::RecordDataBuffer::color();
    case 1:
      return KDColor::RGB16(m_derivativesOptions.colorFirstDerivative);
    default:
      assert(derivationOrder == 2);
      return KDColor::RGB16(m_derivativesOptions.colorSecondDerivative);
  }
}

void ContinuousFunction::RecordDataBuffer::setColor(KDColor color,
                                                    int derivationOrder) {
  switch (derivationOrder) {
    case 0:
      Function::RecordDataBuffer::setColor(color);
      break;
    case 1:
      m_derivativesOptions.colorFirstDerivative = color;
      break;
    default:
      assert(derivationOrder == 2);
      m_derivativesOptions.colorSecondDerivative = color;
      break;
  }
}

/* ContinuousFunction::Model */

Expression ContinuousFunction::Model::expressionReduced(
    const Ion::Storage::Record *record, Context *context) const {
  // m_expression might already be memmoized.
  if (m_expression.isUninitialized()) {
    // Retrieve the expression equation's expression.
    m_expression = expressionReducedForAnalysis(record, context);
    ContinuousFunctionProperties thisProperties = properties();
    if (!thisProperties.isEnabled()) {
      m_expression = Undefined::Builder();
      return m_expression;
    }
    Preferences::ComplexFormat complexFormat =
        this->complexFormat(record, context);
    Preferences::AngleUnit angleUnit =
        Preferences::sharedPreferences->angleUnit();
    if (thisProperties.isScatterPlot()) {
      /* Scatter plots do not depend on any variable, so they can be
       * approximated in advance.
       * In addition, they are sorted to be travelled from left to right (i.e.
       * in order of ascending x). */
      if (m_expression.type() == ExpressionNode::Type::List ||
          (m_expression.type() == ExpressionNode::Type::Dependency &&
           m_expression.childAtIndex(0).type() == ExpressionNode::Type::List)) {
        Expression list = m_expression.type() == ExpressionNode::Type::List
                              ? m_expression
                              : m_expression.childAtIndex(0);
        m_expression =
            static_cast<List &>(list).approximateAndRemoveUndefAndSort<double>(
                ApproximationContext(context, complexFormat));
      } else {
        assert(m_expression.type() == ExpressionNode::Type::Point ||
               (m_expression.type() == ExpressionNode::Type::Dependency &&
                m_expression.childAtIndex(0).type() ==
                    ExpressionNode::Type::Point));
        m_expression = PoincareHelpers::Approximate<double>(
            m_expression, context,
            {.complexFormat = complexFormat, .angleUnit = angleUnit});
      }
    } else if (!thisProperties.isPolar() && !thisProperties.isInversePolar() &&
               (record->fullName() == nullptr ||
                record->fullName()[0] == k_unnamedRecordFirstChar)) {
      /* Polar, inversePolar and cartesian equations are unnamed. Here
       * only cartesian equations are processed. */
      /* Function isn't named, m_expression currently is an expression in y or x
       * such as y = x. We extract the solution by solving in y or x. */
      int yDegree = m_expression.polynomialDegree(
          context, ContinuousFunctionProperties::k_ordinateName);
      bool willBeAlongX = true;
      if (yDegree < 1 || yDegree > 2) {
        int xDegree = m_expression.polynomialDegree(context, k_unknownName);
        if (xDegree < 1 || xDegree > 2) {
          // Such degrees of equation in y and x are not handled.
          m_expression = Undefined::Builder();
          return m_expression;
        }
        // Equation can be plotted along y. For example : x=cos(y) or x^2=1
        willBeAlongX = false;
      }
      /* Solve the equation in y (or x if not willBeAlongX)
       * Symbols are replaced to simplify roots. */
      Expression coefficients[Expression::k_maxNumberOfPolynomialCoefficients];
      int degree = m_expression.getPolynomialReducedCoefficients(
          willBeAlongX ? ContinuousFunctionProperties::k_ordinateName
                       : k_unknownName,
          coefficients, context, complexFormat, angleUnit,
          ContinuousFunctionProperties::k_defaultUnitFormat,
          SymbolicComputation::ReplaceAllDefinedSymbolsWithDefinition, true);

      if (degree == -1) {
        /* The reduction failed, so the expression is not reduced and
         * getPolynomialReducedCoefficients returned -1. */
        return m_expression;
      }
      assert(!willBeAlongX || degree == yDegree);
      ReductionContext reductionContext(context, complexFormat, angleUnit,
                                        Preferences::UnitFormat::Metric,
                                        ReductionTarget::SystemForAnalysis);
      if (degree == 1) {
        Polynomial::LinearPolynomialRoots(coefficients[1], coefficients[0],
                                          &m_expression, reductionContext,
                                          false);
      } else if (degree == 2) {
        // Equation is of degree 2, each root is a subcurve to plot.
        assert(m_properties.isOfDegreeTwo());
        Expression root1, root2, delta;
        int solutions = Polynomial::QuadraticPolynomialRoots(
            coefficients[2], coefficients[1], coefficients[0], &root1, &root2,
            &delta, reductionContext, nullptr, false);
        if (solutions <= 1) {
          m_expression = root1;
        } else {
          // SubCurves are stored in a list
          // Roots are ordered so that the first one is superior to the second
          List newExpr = List::Builder();
          newExpr.addChildAtIndexInPlace(root2, newExpr.numberOfChildren(),
                                         newExpr.numberOfChildren());
          newExpr.addChildAtIndexInPlace(root1, newExpr.numberOfChildren(),
                                         newExpr.numberOfChildren());
          /* Shallow reduce in case the equation could approximate to a list.
           * Example: x^2={}(a) */
          m_expression = newExpr.shallowReduce(reductionContext);
        }
      } else {
        /* TODO : We could handle simple equations of any degree by solving the
         * equation within the graph view bounds, to plot as many vertical or
         * horizontal lines as needed. */
        m_expression = Undefined::Builder();
        return m_expression;
      }
      if (!willBeAlongX && yDegree != 0) {
        // No need to replace anything if yDegree is 0
        m_expression.replaceSymbolWithExpression(
            Symbol::Builder(k_ordinateSymbol), Symbol::SystemSymbol());
      }
    } else {
      /* m_expression is resulting of a simplification with the target
       * SystemForAnalysis. But SystemForApproximation can be sometimes way
       * simpler than the one from SystemForAnalysis. For example (x+9)^6 is
       * fully developped in SystemForAnalysis, which results in approximation
       * inaccuracy. On the other hand, the expression (x+1)^2-x^2-2x-1 should
       * be developped so that we understand that it's equal to zero, and is
       * better handled by SystemForAnalysis. To solve this problem, we try to
       * simplify both ways and compare the number of nodes of each expression.
       * We take the one that has the less node. This is not ideal because an
       * expression with less node does not always mean a simpler expression,
       * but it's a good compromise for now.       */
      Expression resultForApproximation = expressionEquation(record, context);
      if (!resultForApproximation.isUninitialized()) {
        PoincareHelpers::CloneAndReduce(
            &resultForApproximation, context,
            {.complexFormat = complexFormat,
             .updateComplexFormatWithExpression = false,
             .target = ReductionTarget::SystemForApproximation,
             .symbolicComputation =
                 SymbolicComputation::DoNotReplaceAnySymbol});
        if (resultForApproximation.numberOfDescendants(true) <
            m_expression.numberOfDescendants(true)) {
          m_expression = resultForApproximation;
        }
      }
    }
  }
  return m_expression;
}

Poincare::Expression ContinuousFunction::Model::expressionApproximated(
    const Ion::Storage::Record *record, Poincare::Context *context,
    int derivationOrder) const {
  assert(0 <= derivationOrder && derivationOrder <= 2);
  Expression *approximated;
  switch (derivationOrder) {
    case 0:
      approximated = &m_expressionApproximated;
      break;
    case 1:
      approximated = &m_expressionFirstDerivateApproximated;
      break;
    default:
      assert(derivationOrder == 2);
      approximated = &m_expressionSecondDerivateApproximated;
      break;
  }
  if (approximated->isUninitialized()) {
    Expression e = derivationOrder == 0 ? expressionReduced(record, context)
                                        : expressionDerivateReduced(
                                              record, context, derivationOrder);
    PoincareHelpers::CloneAndApproximateKeepingSymbols(
        &e, context,
        {.complexFormat = complexFormat(record, context),
         .updateComplexFormatWithExpression = false,
         .target = ReductionTarget::SystemForApproximation,
         .symbolicComputation = SymbolicComputation::DoNotReplaceAnySymbol});
    *approximated = e;
  }
  return *approximated;
}

Poincare::Expression ContinuousFunction::Model::expressionReducedForAnalysis(
    const Ion::Storage::Record *record, Poincare::Context *context) const {
  ContinuousFunctionProperties::SymbolType computedFunctionSymbol =
      ContinuousFunctionProperties::k_defaultSymbolType;
  ComparisonNode::OperatorType computedEquationType =
      ContinuousFunctionProperties::k_defaultEquationType;
  bool isCartesianEquation = false;
  Expression result =
      expressionEquation(record, context, &computedEquationType,
                         &computedFunctionSymbol, &isCartesianEquation);
  Preferences::ComplexFormat complexFormat =
      this->complexFormat(record, context);
  if (!result.isUndefined()) {
    PoincareHelpers::CloneAndReduce(
        &result, context,
        {.complexFormat = complexFormat,
         .updateComplexFormatWithExpression = false,
         .target = ReductionTarget::SystemForAnalysis,
         // Symbols have already been replaced.
         .symbolicComputation = SymbolicComputation::DoNotReplaceAnySymbol});
  }
  if (!m_properties.isInitialized()) {
    // Use the computed equation to update the plot type.
    m_properties.update(result, originalEquation(record, UCodePointUnknown),
                        context, complexFormat, computedEquationType,
                        computedFunctionSymbol, isCartesianEquation);
  }
  return result;
}

Expression ContinuousFunction::Model::expressionClone(
    const Ion::Storage::Record *record) const {
  assert(record->fullName() != nullptr &&
         record->fullName()[0] != k_unnamedRecordFirstChar);
  Expression e = ExpressionModel::expressionClone(record);
  if (e.isUninitialized()) {
    return e;
  }
  return e.childAtIndex(1);
}

Expression ContinuousFunction::Model::originalEquation(
    const Ion::Storage::Record *record, CodePoint symbol) const {
  Expression unknownSymbolEquation = ExpressionModel::expressionClone(record);
  if (unknownSymbolEquation.isUninitialized() || symbol == UCodePointUnknown) {
    return unknownSymbolEquation;
  }
  return unknownSymbolEquation.replaceSymbolWithExpression(
      Symbol::SystemSymbol(), Symbol::Builder(symbol));
}

bool ContinuousFunction::IsFunctionAssignment(const Expression e) {
  if (!ComparisonNode::IsBinaryEquality(e)) {
    return false;
  }
  Expression leftExpression = e.childAtIndex(0);
  if (leftExpression.type() != ExpressionNode::Type::Function) {
    return false;
  }
  Expression functionSymbol = leftExpression.childAtIndex(0);
  return functionSymbol.isIdenticalTo(
             Symbol::Builder(ContinuousFunction::k_cartesianSymbol)) ||
         functionSymbol.isIdenticalTo(
             Symbol::Builder(ContinuousFunction::k_parametricSymbol)) ||
         functionSymbol.isIdenticalTo(
             Symbol::Builder(ContinuousFunction::k_polarSymbol));
}

Expression ContinuousFunction::Model::expressionEquation(
    const Ion::Storage::Record *record, Context *context,
    ComparisonNode::OperatorType *computedEquationType,
    ContinuousFunctionProperties::SymbolType *computedFunctionSymbol,
    bool *isCartesianEquation) const {
  Expression result = ExpressionModel::expressionClone(record);
  if (result.isUninitialized()) {
    return Undefined::Builder();
  }
  ContinuousFunctionProperties::SymbolType tempFunctionSymbol =
      ContinuousFunctionProperties::k_defaultSymbolType;
  ComparisonNode::OperatorType equationType;
  if (!ComparisonNode::IsBinaryComparison(result, &equationType)) {
    if (result.type() == ExpressionNode::Type::Point ||
        (result.type() == ExpressionNode::Type::List &&
         static_cast<List &>(result).isListOfPoints(context)) ||
        (result.type() == ExpressionNode::Type::ListSequence &&
         result.childAtIndex(0).type() == ExpressionNode::Type::Point)) {
      if (computedFunctionSymbol) {
        *computedFunctionSymbol =
            ContinuousFunctionProperties::SymbolType::NoSymbol;
      }
      result = Expression::ExpressionWithoutSymbols(result, context);
      if (!result.isUninitialized()) {
        // Result is not circularly defined.
        return result;
      }
    }
    /* Happens when:
     * - The input text is too long and "f(x)=" can't be inserted.
     * - When inputting amiguous equations like "x+y>2>y".
     * - When result is uninitialized because of circular definition.  */
    return Undefined::Builder();
  }
  if (equationType == ComparisonNode::OperatorType::NotEqual) {
    return Undefined::Builder();
  }
  if (computedEquationType) {
    *computedEquationType = equationType;
  }
  bool isUnnamedFunction = true;
  Expression leftExpression = result.childAtIndex(0);

  if (IsFunctionAssignment(result)) {
    // Ensure that function name is either record's name, or free
    assert(record->fullName() != nullptr);
    assert(leftExpression.type() == Poincare::ExpressionNode::Type::Function);
    const char *functionName =
        static_cast<Poincare::Function &>(leftExpression).name();
    const size_t functionNameLength = strlen(functionName);
    if (Shared::GlobalContext::SymbolAbstractNameIsFree(functionName) ||
        strncmp(record->fullName(), functionName, functionNameLength) == 0) {
      Expression functionSymbol = leftExpression.childAtIndex(0);
      // Set the model's plot type.
      if (functionSymbol.isIdenticalTo(Symbol::Builder(k_parametricSymbol))) {
        tempFunctionSymbol = ContinuousFunctionProperties::SymbolType::T;
      } else if (functionSymbol.isIdenticalTo(Symbol::Builder(k_polarSymbol))) {
        tempFunctionSymbol = ContinuousFunctionProperties::SymbolType::Theta;
      } else {
        assert(
            functionSymbol.isIdenticalTo(Symbol::Builder(k_cartesianSymbol)));
        tempFunctionSymbol = ContinuousFunctionProperties::SymbolType::X;
      }
      result = result.childAtIndex(1);
      isUnnamedFunction = false;
    } else {
      /* Function in first half of the equation refer to an already defined one.
       * Replace the symbol. */
      leftExpression.replaceChildAtIndexInPlace(0, Symbol::SystemSymbol());
    }
  } else if (leftExpression.isIdenticalTo(Symbol::Builder(k_radiusSymbol)) ||
             leftExpression.isIdenticalTo(Symbol::Builder(k_polarSymbol))) {
    result = result.childAtIndex(1);
    tempFunctionSymbol =
        leftExpression.isIdenticalTo(Symbol::Builder(k_polarSymbol))
            ? ContinuousFunctionProperties::SymbolType::Radius
            : ContinuousFunctionProperties::SymbolType::Theta;
    isUnnamedFunction = false;
  }
  if (computedFunctionSymbol) {
    *computedFunctionSymbol = tempFunctionSymbol;
  }
  if (isCartesianEquation) {
    *isCartesianEquation = isUnnamedFunction;
  }
  if (isUnnamedFunction) {
    result = Subtraction::Builder(leftExpression, result.childAtIndex(1));
    /* Replace all y symbols with UCodePointTemporaryUnknown so that they are
     * not replaced if they had a predefined value. This will not replace the y
     * symbols nested in function, which is not a supported behavior anyway.
     * TODO: Make a consistent behavior calculation/additional_results using a
     *       VariableContext to temporary disable y's predefinition. */
    result = result.replaceSymbolWithExpression(
        Symbol::Builder(k_ordinateSymbol),
        Symbol::Builder(UCodePointTemporaryUnknown));
  }
  // Replace all defined symbols and functions to extract symbols
  result = Expression::ExpressionWithoutSymbols(result, context);
  if (result.isUninitialized()) {
    // result was Circularly defined
    return Undefined::Builder();
  }
  if (isUnnamedFunction) {
    result = result.replaceSymbolWithExpression(
        Symbol::Builder(UCodePointTemporaryUnknown),
        Symbol::Builder(k_ordinateSymbol));
  }
  assert(!result.isUninitialized());
  return result;
}

Expression ContinuousFunction::Model::expressionDerivateReduced(
    const Ion::Storage::Record *record, Context *context,
    int derivationOrder) const {
  // Derivative isn't available on curves with multiple subcurves
  assert(numberOfSubCurves(record) == 1);
  assert(1 <= derivationOrder && derivationOrder <= 2);
  Expression *derivative = derivationOrder == 1 ? &m_expressionFirstDerivate
                                                : &m_expressionSecondDerivate;
  if (derivative->isUninitialized()) {
    Expression expression = expressionReduced(record, context).clone();
    *derivative = Derivative::Builder(expression, Symbol::SystemSymbol(),
                                      Symbol::SystemSymbol(),
                                      BasedInteger::Builder(derivationOrder));
    /* On complex functions, this step can take a significant time.
     * A workaround could be to identify big functions to skip simplification
     * at the cost of possible inaccurate evaluations (such as
     * diff(abs(x),x,0) not being undefined). */
    PoincareHelpers::CloneAndSimplify(
        derivative, context,
        {.complexFormat = complexFormat(record, context),
         .updateComplexFormatWithExpression = false,
         .target = ReductionTarget::SystemForApproximation});
  }
  return *derivative;
}

Expression ContinuousFunction::Model::expressionSlopeReduced(
    const Ion::Storage::Record *record, Context *context) const {
  /* Slope is only needed for parametric and polar functions.
   * For cartesian function, it is the same as the derivative.
   * For curves with multiple subcurves and for inverse polar,
   * it is not available. */
  // TODO: assert(canComputeTangent());
  if (m_expressionSlope.isUninitialized()) {
    if (properties().isCartesian()) {
      m_expressionSlope = expressionDerivateReduced(record, context, 1);
    } else {
      assert(properties().isParametric() || properties().isPolar());
      Expression expression = parametricForm(record, context);
      assert(expression.type() == ExpressionNode::Type::Point);
      assert(expression.numberOfChildren() == 2);
      m_expressionSlope = Division::Builder(
          Derivative::Builder(expression.childAtIndex(1),
                              Symbol::SystemSymbol(), Symbol::SystemSymbol()),
          Derivative::Builder(expression.childAtIndex(0),
                              Symbol::SystemSymbol(), Symbol::SystemSymbol()));
    }
    /* On complex functions, this step can take a significant time.
     * A workaround could be to identify big functions to skip simplification
     * at the cost of possible inaccurate evaluations (such as
     * diff(abs(x),x,0) not being undefined). */
    PoincareHelpers::CloneAndSimplify(
        &m_expressionSlope, context,
        {.complexFormat = complexFormat(record, context),
         .updateComplexFormatWithExpression = false,
         .target = ReductionTarget::SystemForApproximation});
  }
  return m_expressionSlope;
}

Ion::Storage::Record::ErrorStatus
ContinuousFunction::Model::renameRecordIfNeeded(Ion::Storage::Record *record,
                                                Context *context) const {
  /* Use ExpressionModel::expressionClone because it does not alter
   * the left-hand side of "f(x)=" and "f(t)=", which allows the name
   * of the function to be found. */
  Expression newExpression = ExpressionModel::expressionClone(record);
  Ion::Storage::Record::ErrorStatus error =
      Ion::Storage::Record::ErrorStatus::None;
  if (newExpression.isUninitialized()) {
    /* The expression is not set, the record will be named later or is already
     * correctly named by GlobalContext. */
    return error;
  }
  if (record->hasExtension(Ion::Storage::functionExtension)) {
    if (IsFunctionAssignment(newExpression)) {
      Expression function = newExpression.childAtIndex(0);
      error = Ion::Storage::Record::SetBaseNameWithExtension(
          record, static_cast<SymbolAbstract &>(function).name(),
          Ion::Storage::functionExtension);
      if (error != Ion::Storage::Record::ErrorStatus::NameTaken) {
        return error;
      }
      // Function's name is already taken, reset records name if needed.
      error = Ion::Storage::Record::ErrorStatus::None;
    }
    if (record->fullName()[0] == k_unnamedRecordFirstChar) {
      // Record is already unnamed (and hidden).
      return error;
    }
    // Rename record with a hidden record name.
    char name[k_maxDefaultNameSize];
    size_t length = SerializationHelper::CodePoint(name, k_maxDefaultNameSize,
                                                   k_unnamedRecordFirstChar);
    Ion::Storage::FileSystem::sharedFileSystem->firstAvailableNameFromPrefix(
        name, length, k_maxDefaultNameSize, Ion::Storage::functionExtension);
    error = Ion::Storage::Record::SetBaseNameWithExtension(
        record, name, Ion::Storage::functionExtension);
  }
  return error;
}

Poincare::Expression ContinuousFunction::Model::buildExpressionFromText(
    const char *c, CodePoint symbol, Poincare::Context *context) const {
  /* The symbol parameter is discarded in this implementation. Either there is a
   * valid named left expression and the symbol will be extracted, either the
   * symbol should be the default symbol used in unnamed expressions. */
  assert(symbol == k_unnamedExpressionSymbol);
  // if c = "", we want to reinit the Expression
  if (!c || c[0] == 0) {
    return Expression();
  }
  /* Parse the expression to store as possible function assignment. */
  Expression expressionToStore = Expression::Parse(c, context, true, true);
  if (expressionToStore.isUninitialized()) {
    return expressionToStore;
  }
  // Check if the equation is of the form f(x)=...
  if (IsFunctionAssignment(expressionToStore)) {
    Expression functionSymbol =
        expressionToStore.childAtIndex(0).childAtIndex(0);
    // Extract the CodePoint function's symbol. We know it is either x, t or θ
    assert(functionSymbol.type() == ExpressionNode::Type::Symbol);
    // Override the symbol so that it can be replaced in the right expression
    if (functionSymbol.isIdenticalTo(Symbol::Builder(k_cartesianSymbol))) {
      symbol = k_cartesianSymbol;
    } else if (functionSymbol.isIdenticalTo(Symbol::Builder(k_polarSymbol))) {
      symbol = k_polarSymbol;
    } else {
      assert(functionSymbol.isIdenticalTo(Symbol::Builder(k_parametricSymbol)));
      symbol = k_parametricSymbol;
    }
    // Do not replace symbol in f(x)=
    ExpressionModel::ReplaceSymbolWithUnknown(expressionToStore.childAtIndex(1),
                                              symbol);
  } else {
    if (expressionToStore.recursivelyMatches([](const Expression e) {
          return e.type() == ExpressionNode::Type::Symbol &&
                 AliasesLists::k_thetaAliases.contains(
                     static_cast<const Symbol &>(e).name());
        })) {
      symbol = expressionToStore.childAtIndex(0).isIdenticalTo(
                   Symbol::Builder(k_polarSymbol))
                   ? k_radiusSymbol
                   : k_polarSymbol;
    }
    // Fallback on normal parsing (replace symbol with unknown)
    expressionToStore =
        ExpressionModel::buildExpressionFromText(c, symbol, context);
  }

  return expressionToStore;
}

void ContinuousFunction::Model::tidyDownstreamPoolFrom(
    TreeNode *treePoolCursor) const {
  if (treePoolCursor == nullptr ||
      m_expressionFirstDerivate.isDownstreamOf(treePoolCursor)) {
    m_expressionFirstDerivate = Expression();
  }
  if (treePoolCursor == nullptr ||
      m_expressionFirstDerivateApproximated.isDownstreamOf(treePoolCursor)) {
    m_expressionFirstDerivateApproximated = Expression();
  }
  if (treePoolCursor == nullptr ||
      m_expressionSecondDerivate.isDownstreamOf(treePoolCursor)) {
    m_expressionSecondDerivate = Expression();
  }
  if (treePoolCursor == nullptr ||
      m_expressionSecondDerivateApproximated.isDownstreamOf(treePoolCursor)) {
    m_expressionSecondDerivateApproximated = Expression();
  }
  if (treePoolCursor == nullptr ||
      m_expressionSlope.isDownstreamOf(treePoolCursor)) {
    m_expressionSlope = Expression();
  }
  if (treePoolCursor == nullptr ||
      m_expressionApproximated.isDownstreamOf(treePoolCursor)) {
    m_expressionApproximated = Expression();
  }
  ExpressionModel::tidyDownstreamPoolFrom(treePoolCursor);
}

int ContinuousFunction::Model::numberOfSubCurves(
    const Ion::Storage::Record *record) const {
  if (properties().isCartesian()) {
    Expression e = expressionReduced(
        record, AppsContainerHelper::sharedAppsContainerGlobalContext());
    if (e.type() == ExpressionNode::Type::List) {
      assert(properties().isOfDegreeTwo());
      return e.numberOfChildren();
    }
  }
  return 1;
}

Expression ContinuousFunction::Model::parametricForm(
    const Ion::Storage::Record *record, Poincare::Context *context,
    bool approximated) const {
  assert(properties().isPolar() || properties().isInversePolar() ||
         properties().isParametric());
  Expression e = approximated ? expressionApproximated(record, context)
                              : expressionReduced(record, context);
  if (properties().isPolar() || properties().isInversePolar()) {
    Expression x, y;
    Expression unknown = Symbol::SystemSymbol();
    if (properties().isPolar()) {
      /* Turn r(θ) into f(θ) = (x(θ), y(θ)) with
       * - x(θ) = r(θ) * cos(θ)
       * - y(θ) = r(θ) * sin(θ) */
      x = Multiplication::Builder(e.clone(), Cosine::Builder(unknown.clone()));
      y = Multiplication::Builder(e.clone(), Sine::Builder(unknown.clone()));
    } else {
      /* Turn θ(r) into f(r) = (x(r), y(r)) with
       * - x(r) = r * cos(θ(r))
       * - y(r) = r * sin(θ(r)) */
      x = Multiplication::Builder(unknown.clone(), Cosine::Builder(e.clone()));
      y = Multiplication::Builder(unknown.clone(), Sine::Builder(e.clone()));
    }
    e = Point::Builder(x, y);
  } else {
    assert(properties().isParametric());
    e = e.clone();
  }
  assert(e.type() == ExpressionNode::Type::Point);
  return e;
}

void *ContinuousFunction::Model::expressionAddress(
    const Ion::Storage::Record *record) const {
  return (char *)record->value().buffer + sizeof(RecordDataBuffer);
}

size_t ContinuousFunction::Model::expressionSize(
    const Ion::Storage::Record *record) const {
  return record->value().size - sizeof(RecordDataBuffer);
}

void ContinuousFunction::Model::setStorageChangeFlag() const {
  GlobalContext::continuousFunctionStore->setStorageChangeFlag(true);
}

template Coordinate2D<float>
ContinuousFunction::templatedApproximateAtParameter<float>(float, Context *,
                                                           int) const;
template Coordinate2D<double>
ContinuousFunction::templatedApproximateAtParameter<double>(double, Context *,
                                                            int) const;

template Coordinate2D<float>
ContinuousFunction::privateEvaluateXYAtParameter<float>(float, Context *,
                                                        int) const;
template Coordinate2D<double>
ContinuousFunction::privateEvaluateXYAtParameter<double>(double, Context *,
                                                         int) const;
template Evaluation<double> ContinuousFunction::approximateDerivative(
    double t, Context *context, int derivationOrder, bool useDomain) const;

}  // namespace Shared
