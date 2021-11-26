#include <poincare/conic.h>
#include <poincare/polynomial.h>
#include <poincare/preferences.h>
#include <algorithm>

/* Double comparison is extensively used in Conic's methods for performances.
 * To limit the approximation errors that may rise from these comparisons, we
 * round the values of coefficients to 0 (or 1) before and after manipulating
 * them. */

namespace Poincare {

// Return smallest positive number between x and y, assuming at least one is > 0
double smallestPositive(double x, double y) {
  if (x > y) {
    return smallestPositive(y, x);
  }
  // We have x < y. Assert y is strictly positive.
  assert(y > 0.0);
  return (x > 0.0) ? x : y;
}

Conic::Conic(const Expression e, Context * context, const char * x, const char * y) :
    m_a(0.0),
    m_b(0.0),
    m_c(0.0),
    m_d(0.0),
    m_e(0.0),
    m_f(0.0),
    m_cx(0.0),
    m_cy(0.0),
    m_r(0.0),
    m_type(Type::Unknown) {
  /* Expression e represents an equation of the form :
   * A*x^2 + B*x*y + C*y^2 + D*x + E*y + F = 0
   * In this constructor, we extract the coefficients parameters.
   * We then compute the conic's type and canonize the coefficients. */
  Preferences::ComplexFormat complexFormat = Preferences::sharedPreferences()->complexFormat();
  Preferences::AngleUnit angleUnit = Preferences::sharedPreferences()->angleUnit();
  Preferences::UnitFormat unitFormat = Preferences::UnitFormat::Metric;
  ExpressionNode::SymbolicComputation symbolicComputation = ExpressionNode::SymbolicComputation::DoNotReplaceAnySymbol;
  // Reduce Conic for analysis
  Expression reducedExpression = e.cloneAndReduce(
      ExpressionNode::ReductionContext(
          context, complexFormat, angleUnit, unitFormat,
          ExpressionNode::ReductionTarget::SystemForAnalysis));
  // Extracting y parameters : C, B+E and A+D+F
  Expression coefficientsY[Expression::k_maxNumberOfPolynomialCoefficients];
  int dy = reducedExpression.getPolynomialReducedCoefficients(
      y, coefficientsY, context, complexFormat, angleUnit, unitFormat,
      symbolicComputation);
  if (dy > 2 || dy < 1) {
    m_type = Type::Undefined;
    return;
  }
  Expression coefficientsX[Expression::k_maxNumberOfPolynomialCoefficients];
  // Extract C term
  if (dy == 2) {
    // Ensure coefficient C does not depend on x
    int dx = coefficientsY[2].getPolynomialReducedCoefficients(
        x, coefficientsX, context, complexFormat, angleUnit, unitFormat,
        symbolicComputation);
    if (dx != 0) {
      m_type = Type::Undefined;
      return;
    }
    m_c = coefficientsY[2].approximateToScalar<double>(context, complexFormat,
                                                       angleUnit);
  }
  // Extract B and E terms
  int dx = coefficientsY[1].getPolynomialReducedCoefficients(
      x, coefficientsX, context, complexFormat, angleUnit, unitFormat,
      symbolicComputation);
  if (dx > 1 || dx < 0) {
    m_type = Type::Undefined;
    return;
  }
  if (dx == 1) {
    m_b = coefficientsX[1].approximateToScalar<double>(context, complexFormat,
                                                       angleUnit);
  }
  m_e = coefficientsX[0].approximateToScalar<double>(context, complexFormat,
                                                     angleUnit);
  // Extract A, D and F terms
  dx = coefficientsY[0].getPolynomialReducedCoefficients(
      x, coefficientsX, context, complexFormat, angleUnit, unitFormat,
      symbolicComputation);
  if (dx > 2 || dx < 0 || (dx < 2 && dy < 2 && m_b == 0.0) || (dx == 0 && m_b == 0.0)) {
    // A conic must have at least 1 squared term, 1 x term and 1 y term.
    m_type = Type::Undefined;
    return;
  }
  if (dx == 2) {
    m_a = coefficientsX[2].approximateToScalar<double>(context, complexFormat,
                                                       angleUnit);
  }
  if (dx >= 1) {
    m_d = coefficientsX[1].approximateToScalar<double>(context, complexFormat,
                                                       angleUnit);
  }
  m_f = coefficientsX[0].approximateToScalar<double>(context, complexFormat,
                                                     angleUnit);
  assert(std::isfinite(m_a) && std::isfinite(m_b) && std::isfinite(m_c) &&
         std::isfinite(m_d) && std::isfinite(m_e) && std::isfinite(m_f));
  // Round the coefficients to 0 if they are neglectable against the other ones
  roundCoefficientsIfNeglectable();
  // Setting type from a canonic conic is safer.
  canonize();
  updateConicType();
}

void Conic::updateConicType() {
  if (m_type == Type::Unknown) {
    if ((m_a == 0.0 && m_c == 0.0) ||
        (m_b == 0.0 &&
         ((m_a == 0.0 && m_d == 0.0) || (m_c == 0.0 && m_e == 0.0)))) {
      // A conic must have at least 1 squared term, 1 x term and 1 y term.
      m_type = Type::Undefined;
    } else {
      double determinant = getDeterminant();
      if (determinant < 0.0) {
        m_type = (m_b == 0.0 && m_a == m_c) ? Type::Circle : Type::Ellipse;
      } else {
        m_type = (determinant == 0.0) ? Type::Parabola : Type::Hyperbola;
      }
    }
  }
}

double Conic::roundIfNeglectable(double value, double target, double amplitude) const {
  return (std::abs(value-target) < k_tolerance * std::abs(amplitude)) ? target : value;
}

void Conic::roundCoefficientsIfNeglectable() {
  double amplitude = std::max(
      std::max(std::max(std::fabs(m_a), std::fabs(m_b)), std::fabs(m_c)),
      std::max(std::max(std::fabs(m_d), std::fabs(m_e)), std::fabs(m_f)));
  m_a = roundIfNeglectable(m_a, 0.0, amplitude);
  m_b = roundIfNeglectable(m_b, 0.0, amplitude);
  m_c = roundIfNeglectable(m_c, 0.0, amplitude);
  m_d = roundIfNeglectable(m_d, 0.0, amplitude);
  m_e = roundIfNeglectable(m_e, 0.0, amplitude);
  m_f = roundIfNeglectable(m_f, 0.0, amplitude);
}

void Conic::multiplyCoefficients(double factor) {
  m_a *= factor;
  m_b *= factor;
  m_c *= factor;
  m_d *= factor;
  m_e *= factor;
  m_f *= factor;
}

bool Conic::isCanonicallyRotated() const {
  /* For the conic to be rotated, coefficients must be of one of these forms :
   * A*x^2 + C*y^2 + D*x + E*y + F = 0 with A > 0 and either C <= 0 or C >= A */
  assert((m_a > 0.0 || m_c > 0.0));
  return m_b == 0.0 && m_a == smallestPositive(m_a, m_c);
}

void Conic::rotateConic() {
  /* Rotate the conic by first aligning it along the x and y axes. Then, rotate
   * it up to three times by π/2 to find a unique canonic orientation. */
  if (m_a <= 0.0 && m_c <= 0.0) {
    // This ensures that at least on of the two coefficients is always positive
    multiplyCoefficients(-1.0);
  }
  if ((m_a > 0.0 || m_c > 0.0) && isCanonicallyRotated()) {
    // Conic is already rotated in canonic form.
    return;
  }
  double a = m_a;
  double b = m_b;
  double c = m_c;
  double d = m_d;
  double e = m_e;
  double amplitude = std::max(std::fabs(a), std::max(std::fabs(b), std::fabs(c)));
  // If B is non null and A = C, the first rotation must be of π/4
  double r = (b == 0.0 ? 0.0
                       : -((a == c) ? M_PI / 4 : std::atan(b / (a - c)) / 2.0));
  double cr, sr;
  for (int i = 0; i < 4; i++) {
    cr = std::cos(r);
    sr = std::sin(r);
    // Replacing x with cr*x+sr*y and y with -sr*x+cr*y to cancel B coefficient
    m_a = roundIfNeglectable(a*cr*cr - b*cr*sr + c*sr*sr, 0.0, amplitude);
    m_b = roundIfNeglectable(2*a*cr*sr + b*cr*cr - b*sr*sr - 2*c*sr*cr, 0.0, amplitude);
    m_c = roundIfNeglectable(a*sr*sr + b*cr*sr + c*cr*cr, 0.0, amplitude);
    assert(m_b == 0.0);
    /* Looking at each π/2 rotations to find the most canonic form :
     * - A is strictly positive (y is the axis of symmetry)
     * - C is either :
     *     - null (Parabola)
     *     - negative (Hyperbola),
     *     - equal (Circle)
     *     - greater than A (Elipsis, major axis along x)
     */
    if ((m_a > 0.0 || m_c > 0.0) && isCanonicallyRotated()) {
      break;
    }
    assert(i < 3);
    r += M_PI / 2;
  }
  /* If rotated for the second time, the conic has been centered, and the
   * rotation might not be simple to compute anymore. Set it to NAN as we won't
   * use it anyway. */
  m_r = (m_r == 0.0) ? r : NAN;
  // Apply the rotation to D and E (F remains unchanged)
  amplitude = std::max(std::fabs(d), std::fabs(e));
  m_d = roundIfNeglectable(d * cr - e * sr, 0.0, amplitude);
  m_e = roundIfNeglectable(d * sr + e * cr, 0.0, amplitude);
  // Assert the conic is rotated in a canonic for.
  assert(isCanonicallyRotated());
}

bool Conic::isCanonicallyCentered() const {
  /* For the conic to be centered, coefficients must be of one of these forms :
   * Ax^2 + Ey = 0 with A > 0
   * Ax^2 + Cy^2 + F = 0 with A > 0 and either C <= 0 or (C >= A and F != 0)
   * Calling this method on already rotated conics ensures B is null, A > 0,
   * and either C <= 0 or C >= A. */
  assert(isCanonicallyRotated());
  return m_d == 0.0
         && ((m_c == 0.0 && m_e != 0.0 && m_f == 0.0)
             || (m_e == 0.0 && (m_c <= 0.0 || m_f != 0.0)));
}

void Conic::centerConic() {
  /* Rotate and center the conic so that here remains only one term in x and one
   * term in y. If A or C is null, F should be null as well. */
  rotateConic();
  if (isCanonicallyCentered()) {
    return;
  }
  assert(m_cx == 0.0 && m_cy == 0.0);
  double a = m_a;
  double c = m_c;
  double d = m_d;
  double e = m_e;
  double f = m_f;
  double h;
  double k;
  // Replacing x with x-h and y with y-k in order to cancel :
  // - D and F if C is null : Ax^2 + Ey = 0
  // - D and E otherwise    : Ax^2 + Cy^2 = F
  assert(a != 0.0);
  h = d / (2*a);
  if (c != 0.0) {
    k = e / (2*c);
  } else if (e != 0.0) {
    k = (f + a*h*h - d*h) / e;
  } else {
    assert(d == 0.0);
    k = 0.0;
  }
  // A and C remain unchanged
  m_d = roundIfNeglectable(d - 2*a*h, 0.0, std::max(std::fabs(d), std::fabs(2*a*h)));
  m_e = roundIfNeglectable(e - 2*c*k, 0.0, std::max(std::fabs(e), std::fabs(2*c*k)));
  double fAmplitude = std::max(std::max(std::max(std::fabs(f), std::fabs(a*h*h)), std::max(std::fabs(c*k*k), std::fabs(d*h))), std::fabs(e*k));
  m_f = roundIfNeglectable(f + a*h*h + c*k*k - d*h - e*k, 0.0, fAmplitude);
  // Update center (taking previous rotation into account)
  assert(!std::isnan(m_r));
  double cr = std::cos(m_r);
  double sr = std::sin(m_r);
  m_cx = -(h*cr + k*sr);
  m_cy = -(h*(-sr) + k*cr);
  assert(isCanonicallyCentered());
}

bool Conic::isCanonical() const {
  /* For the conic to be canonic, coefficients must be of one of these forms :
   * x^2 + Ey = 0 with E != 0
   * Ax^2 + Cy^2 - 1 = 0 with A > 0 and either C < 0 or C >= A
   * Calling this method on canonically centered conics to simplify check */
  assert(isCanonicallyCentered());
  return (m_f == -1.0 && m_c != 0.0) || (m_f == 0.0 && m_a == 1.0 && m_e != 0.0);
}

void Conic::canonize() {
  /* Rotate, center the conic and ensure that coefficients are unique :
   * - Circle, Ellipse, Hyperbola : Ax^2 + Cy^2 = 1, A > 0 and (C < 0 or C >= A)
   * - Parabola                   : x^2 + Ey = 0 with E != 0
   */
  centerConic();
  if (isCanonical()) {
    return;
  }
  if (m_e == 0.0 && (m_f == 0.0 || m_c == 0.0)) {
    /* The equation is of one of the forms :
     * Ax^2 + Cy^2 = 0 with A > 0 and C <= 0
     * Ax^2 = F with A > 0 and F >= 0
     * This is a set of two lines and not considered as a conic here. */
    m_type = Type::Undefined;
    return;
  }
  // Canonize either F or A
  if (m_f != 0.0) {
    multiplyCoefficients(-1/m_f);
    m_f = roundIfNeglectable(m_f, -1.0, std::abs(m_f));
    assert(m_f == -1.0);
  } else {
    multiplyCoefficients(1/m_a);
    m_a = roundIfNeglectable(m_a, 1.0, std::abs(m_a));
    assert(m_a == 1.0);
  }
  /* The sign of F may sometimes change after centering. In that case, the
   * conic needs to be rotated again to ensure the canonic form. */
  if ((m_a <= 0.0 && m_c <= 0.0)) {
    // A, B and F are all negative the equation has no solution.
    m_type = Type::Undefined;
    return;
  }

  if (!isCanonicallyRotated()) {
    /* We must rotate the conic once more. It isn't an issue for the center
     * metrics as it is already centered well. However, m_r is expected to only
     * rotate by  */
    rotateConic();
  }
  assert(isCanonical());
}

double Conic::getDeterminant() const {
  // Only the sign matters, there is no need for a canonic equation here.
  return m_b * m_b - 4.0 * m_a * m_c;
}

void Conic::getCenter(double * cx, double * cy) const {
  // Conic must be centered
  assert(isCanonicallyCentered());
  *cx = m_cx;
  *cy = m_cy;
}

double Conic::getEccentricity() const {
  // Conic must be canonically rotated
  assert(isCanonicallyRotated());
  assert(m_type != Type::Undefined);
  if (m_type == Type::Parabola) {
    assert(m_a == 0.0 || m_c == 0.0);
    return 1.0;
  }
  // A being the smallest positive number we can assert that
  assert(1 - m_a / m_c >= 0.0);
  double e = std::sqrt(1 - m_a / m_c);
  assert(m_type != Type::Circle || e == 0.0);
  return e;
}

double Conic::getSemiMajorAxis() const {
  // Conic must be rotated, F must be -1.0
  assert(isCanonical());
  assert(m_type == Type::Ellipse || m_type == Type::Hyperbola);
  return std::sqrt(1 / m_a);
}

double Conic::getSemiMinorAxis() const {
  // Conic must be rotated, F must be -1.0
  assert(isCanonical());
  assert(m_type == Type::Ellipse || m_type == Type::Hyperbola);
  return std::sqrt(1 / std::abs(m_c));
}

double Conic::getLinearEccentricity() const {
  // Conic must be rotated, F must be -1.0
  assert(isCanonical());
  assert(m_type == Type::Ellipse || m_type == Type::Hyperbola);
  return std::sqrt(std::abs(1 / m_a - 1 / m_c));
}

double Conic::getParameter() const {
  // Conic must be canonically rotated, and A must be 1.0
  assert(isCanonical());
  assert(m_type == Type::Parabola);
  return std::abs(m_e) / 2;
}

void Conic::getSummit(double * sx, double * sy) const {
  // Conic must be centered
  assert(isCanonicallyCentered());
  assert(m_type == Type::Parabola);
  // Parabola's summit is also it's center
  getCenter(sx, sy);
}

double Conic::getRadius() const {
  // F must be -1.0
  assert(isCanonical());
  assert(m_type == Type::Circle);
  return std::sqrt(1 / m_a);
}

}  // namespace Poincare
