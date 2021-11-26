#include <poincare/polynomial.h>
#include <poincare/addition.h>
#include <poincare/arithmetic.h>
#include <poincare/complex_argument.h>
#include <poincare/complex_cartesian.h>
#include <poincare/division.h>
#include <poincare/exception_checkpoint.h>
#include <poincare/float.h>
#include <poincare/imaginary_part.h>
#include <poincare/least_common_multiple.h>
#include <poincare/nth_root.h>
#include <poincare/opposite.h>
#include <poincare/power.h>
#include <poincare/real_part.h>
#include <poincare/square_root.h>
#include <poincare/subtraction.h>
#include <poincare/undefined.h>

namespace Poincare {

int Polynomial::QuadraticPolynomialRoots(Expression a, Expression b, Expression c, Expression * root1, Expression * root2, Expression * delta, Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit, bool approximateSolutions) {
  assert(root1 && root2 && delta);
  assert(!(a.isUninitialized() || b.isUninitialized() || c.isUninitialized()));

  ExpressionNode::ReductionContext reductionContext(context, complexFormat, angleUnit, Preferences::UnitFormat::Metric, ExpressionNode::ReductionTarget::User);

  *delta = Subtraction::Builder(Power::Builder(b.clone(), Rational::Builder(2)), Multiplication::Builder(Rational::Builder(4), a.clone(), c.clone()));
  *delta = delta->simplify(reductionContext);
  assert(!delta->isUninitialized());
  if (delta->isUndefined()) {
    *root1 = Undefined::Builder();
    *root2 = Undefined::Builder();
    return 0;
  }

  bool multipleRoot = false;
  ExpressionNode::NullStatus deltaNull = delta->nullStatus(context);
  if (deltaNull == ExpressionNode::NullStatus::Null
   || (deltaNull == ExpressionNode::NullStatus::Unknown && delta->approximateToScalar<double>(context, complexFormat, angleUnit) == 0.))
  {
    *root1 = Division::Builder(Opposite::Builder(b.clone()), Multiplication::Builder(Rational::Builder(2), a.clone()));
    *root2 = Undefined::Builder();
    multipleRoot = true;
  } else {
    *root1 = Division::Builder(
        Subtraction::Builder(Opposite::Builder(b.clone()), SquareRoot::Builder(delta->clone())),
        Multiplication::Builder(Rational::Builder(2), a.clone()));
    *root2 = Division::Builder(
        Addition::Builder(Opposite::Builder(b.clone()), SquareRoot::Builder(delta->clone())),
        Multiplication::Builder(Rational::Builder(2), a.clone()));
  }

  if (!approximateSolutions) {
    *root1 = root1->simplify(reductionContext);
    *root2 = root2->simplify(reductionContext);
    if (root1->isUninitialized()
     || root2->isUninitialized()
     || root1->type() == ExpressionNode::Type::Undefined
     || (!multipleRoot && root2->type() == ExpressionNode::Type::Undefined))
    {
      // Simplification has been interrupted, recompute approximated roots.
      return QuadraticPolynomialRoots(a, b, c, root1, root2, delta, context, complexFormat, angleUnit, true);
    }
  } else {
    *root1 = root1->approximate<double>(context, complexFormat, angleUnit);
    *root2 = root2->approximate<double>(context, complexFormat, angleUnit);
  }
  assert(!(root1->isUninitialized() || root2->isUninitialized()));

  if (root1->isUndefined()) {
    *root1 = *root2;
    *root2 = Undefined::Builder();
  }

  return !root1->isUndefined() + !root2->isUndefined();
}

static bool rootSmallerThan(const Expression * root1, const Expression * root2, Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit) {
  if (root2->type() == ExpressionNode::Type::Undefined || root2->type() == ExpressionNode::Type::Unreal) {
    return true;
  }
  if (root1->type() == ExpressionNode::Type::Undefined || root1->type() == ExpressionNode::Type::Unreal) {
    return false;
  }
  float r1 = root1->approximateToScalar<float>(context, complexFormat, angleUnit);
  float r2 = root2->approximateToScalar<float>(context, complexFormat, angleUnit);
  if (std::isfinite(r2)) {
    return std::isfinite(r1) && r1 <= r2;
  }
  return std::isfinite(r1)
      || ImaginaryPart::Builder(root1->clone()).approximateToScalar<float>(context, complexFormat, angleUnit) <= ImaginaryPart::Builder(root2->clone()).approximateToScalar<float>(context, complexFormat, angleUnit);
}

int Polynomial::CubicPolynomialRoots(Expression a, Expression b, Expression c, Expression d, Expression * root1, Expression * root2, Expression * root3, Expression * delta, Context * context, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit, bool * approximateSolutions) {
  assert(root1 && root2 && root3 && delta);
  assert(!(a.isUninitialized() || b.isUninitialized() || c.isUninitialized() || d.isUninitialized()));
  const Expression coefficients[] = { d, c, b, a };
  constexpr int degree = 3;
  static_assert(Expression::k_maxPolynomialDegree >= degree, "The maximal polynomial degree is too low to handle cubic equations.");

  ExpressionNode::ReductionContext reductionContext(context, complexFormat, angleUnit, Preferences::UnitFormat::Metric, ExpressionNode::ReductionTarget::User);
  bool approximate = approximateSolutions ? *approximateSolutions : false;
  const bool equationIsReal = a.isReal(context) && b.isReal(context) && c.isReal(context) && d.isReal(context);

  // Cube roots of unity.
  Expression roots[3] = {
    Rational::Builder(1),
    Division::Builder(ComplexCartesian::Builder(Rational::Builder(-1), SquareRoot::Builder(Rational::Builder(3))), Rational::Builder(2)),
    Division::Builder(ComplexCartesian::Builder(Rational::Builder(1), SquareRoot::Builder(Rational::Builder(3))), Rational::Builder(-2))
  };

  // b^2*c^2 + 18abcd - 27a^2*d^2 - 4ac^3 - 4db^3
  *delta = Addition::Builder({
      Power::Builder(Multiplication::Builder(b.clone(), c.clone()), Rational::Builder(2)),
      Multiplication::Builder({Rational::Builder(18), a.clone(), b.clone(), c.clone(), d.clone()}),
      Multiplication::Builder(Rational::Builder(-27), Power::Builder(Multiplication::Builder(a.clone(), d.clone()), Rational::Builder(2))),
      Multiplication::Builder(Rational::Builder(-4), a.clone(), Power::Builder(c.clone(), Rational::Builder(3))),
      Multiplication::Builder(Rational::Builder(-4), d.clone(), Power::Builder(b.clone(), Rational::Builder(3)))});
  if (!approximate) {
    *delta = delta->simplify(reductionContext);
    if (delta->isUninitialized() || delta->type() == ExpressionNode::Type::Undefined) {
      // Simplification has been interrupted, recompute approximated roots.
      approximate = true;
      if (approximateSolutions != nullptr) {
        *approximateSolutions = approximate;
      }
      return CubicPolynomialRoots(a, b, c, d, root1, root2, root3, delta, context, complexFormat, angleUnit, &approximate);
    }
    if (delta->numberOfDescendants(true) > k_maxNumberOfNodesBeforeApproximatingDelta) {
      // Delta is too complex anyway, approximate it.
      *delta = delta->approximate<double>(context, complexFormat, angleUnit);
    }
  } else {
    *delta = delta->approximate<double>(context, complexFormat, angleUnit);
  }
  assert(!delta->isUninitialized());

  /* To avoid applying Cardano's formula right away, we use techniques to find
   * a simple solution, based on some particularly common forms of cubic
   * equations in school problems. */
  *root1 = Expression();
  /* If d is null, the polynom can easily be factored by X. We handle it here
   * (even though in most case it would be caught by the following case) in
   * case c is null. */
  if (d.nullStatus(context) == ExpressionNode::NullStatus::Null || d.approximateToScalar<double>(context, complexFormat, angleUnit) == 0.) {
    *root1 = Rational::Builder(0);
  }
  /* Polynoms of the form "ax^3+d=0" have a simple solutions : x1 = sqrt(-d/a,3)
   * x2 = roots[1] * x1 and x3 = roots[2] * x1. */
  if (root1->isUninitialized()
    && (b.nullStatus(context) == ExpressionNode::NullStatus::Null || b.approximateToScalar<double>(context, complexFormat, angleUnit) == 0.)
    && (c.nullStatus(context) == ExpressionNode::NullStatus::Null || c.approximateToScalar<double>(context, complexFormat, angleUnit) == 0.)) {
    *root1 = NthRoot::Builder(Division::Builder(Opposite::Builder(d.clone()), a.clone()), Rational::Builder(3));
    *root1 = root1->simplify(reductionContext);
    if (root1->isUninitialized()
      || root1->numberOfDescendants(true) * 2 > k_maxNumberOfNodesBeforeApproximatingDelta
      || (complexFormat == Preferences::ComplexFormat::Polar && !equationIsReal)) {
      /* Approximate roots if root1 is uninitialized, too big (roots 2 and 3
       * might be twice root1's size), or if complex format is Polar, which can
       * serverly complexify roots when beautifying.
       * TODO : Improve simplification on Polar complex format. */
      approximate = true;
      *root1 = NthRoot::Builder(Division::Builder(Opposite::Builder(d.clone()), a.clone()), Rational::Builder(3));
    }
    /* We compute the three solutions here because they are quite simple, and
     * to avoid generating very complex coefficients when creating the remaining
     * quadratic equation. */
    *root2 = Multiplication::Builder(root1->clone(), roots[1]);
    *root3 = Multiplication::Builder(root1->clone(), roots[2]);
  }
  /* Polynoms of the forms "kx^2(cx+d)+cx+d" and "kx(bx^2+d)+bx^2+d" have a
   * simple solution x1 = -d/c. */
  Expression r = Division::Builder(Opposite::Builder(d.clone()), c.clone());
  if (root1->isUninitialized() && IsRoot(coefficients, degree, r, reductionContext)) {
    *root1 = r;
  }
  if (root1->isUninitialized() && a.type() == ExpressionNode::Type::Rational && b.type() == ExpressionNode::Type::Rational && c.type() == ExpressionNode::Type::Rational && d.type() == ExpressionNode::Type::Rational) {
    /* The equation can be written with integer coefficients. Under that form,
     * since d/a = x1*x2*x3, a rational root p/q must be so that p divides d
     * and q divides a. */
    *root1 = RationalRootSearch(coefficients, degree, reductionContext);
  }
  if (root1->isUninitialized()) {
    /* b is the opposite of the sum of all roots counted with their
     * multiplicity. As additions containing roots or powers are in general not
     * reducible, if there exists an irrational root, it might still be
     * explicit in the expression for b. */
    *root1 = SumRootSearch(coefficients, degree, 2, reductionContext);
  }

  if (!root1->isUninitialized() && root2->isUninitialized()) {
    /* We have found one simple solution, we can factor and solve the quadratic
     * equation. */
    Expression beta = Addition::Builder({b.clone(), Multiplication::Builder(a.clone(), root1->clone())}).simplify(reductionContext);
    Expression gamma = root1->nullStatus(context) == ExpressionNode::NullStatus::Null ? c.clone() : Opposite::Builder(Division::Builder(d.clone(), root1->clone())).simplify(reductionContext);
    Expression delta2;
    QuadraticPolynomialRoots(a.clone(), beta, gamma, root2, root3, &delta2, context, complexFormat, angleUnit);
    assert(!root2->isUninitialized() && !root3->isUninitialized());
  } else if (root1->isUninitialized()) {
    /* We did not manage to find any simple root : we resort to using Cardano's
     * formula. */
    int deltaSign;
    if (delta->nullStatus(context) == ExpressionNode::NullStatus::Null) {
      deltaSign = 0;
    } else {
      double deltaValue = delta->approximateToScalar<double>(context, complexFormat, angleUnit);
      /* A complex delta (NAN deltaValue) must be handled like a negative delta
       * This ternary operator's condition order is important here. */
      deltaSign = deltaValue == 0. ? 0 : deltaValue > 0. ? 1 : -1;
      assert(!std::isnan(deltaValue) || deltaSign == -1);
    }
    // b^2 - 3ac
    Expression delta0 = Subtraction::Builder(Power::Builder(b.clone(), Rational::Builder(2)), Multiplication::Builder(Rational::Builder(3), a.clone(), c.clone())).simplify(reductionContext);
    if (deltaSign == 0) {
      if (delta0.nullStatus(context) == ExpressionNode::NullStatus::Null || delta0.approximateToScalar<double>(context, complexFormat, angleUnit) == 0.) {
        // -b / 3a
        *root1 = Division::Builder(b.clone(), Multiplication::Builder(Rational::Builder(-3), a.clone()));
        *root2 = Undefined::Builder();
        *root3 = Undefined::Builder();
      } else {
        // (9ad - bc) / (2*delta0)
        *root1 = Division::Builder(Subtraction::Builder(Multiplication::Builder(Rational::Builder(9), a.clone(), d.clone()), Multiplication::Builder(b.clone(), c.clone())), Multiplication::Builder(Rational::Builder(2), delta0.clone()));
        // (4abc - 9da^2 - b^3) / (a*delta0)
        *root2 = Division::Builder(Addition::Builder({Multiplication::Builder(Rational::Builder(4), a.clone(), b.clone(), c.clone()), Multiplication::Builder(Rational::Builder(-9), Power::Builder(a.clone(), Rational::Builder(2)), d.clone()), Opposite::Builder(Power::Builder(b.clone(), Rational::Builder(3)))}), Multiplication::Builder(a.clone(), delta0.clone()));
        *root3 = Undefined::Builder();
      }
    } else {
      // 2b^3 - 9abc + 27da^2
      Expression delta1 = Addition::Builder({
          Multiplication::Builder(Rational::Builder(2), Power::Builder(b.clone(), Rational::Builder(3))),
          Multiplication::Builder(Rational::Builder(-9), a.clone(), b.clone(), c.clone()),
          Multiplication::Builder(Rational::Builder(27), Power::Builder(a.clone(), Rational::Builder(2)), d.clone())
          }).simplify(reductionContext);
      /* Cardano's formula is famous for introducing complex numbers in the
       * resolution of some real equations. As such, we temporarily set the
       * complex format to Cartesian. */
      ExpressionNode::ReductionContext complexContext(context, Preferences::ComplexFormat::Cartesian, angleUnit, Preferences::UnitFormat::Metric, ExpressionNode::ReductionTarget::SystemForApproximation);
      Expression cardano = CardanoNumber(delta0, delta1, &approximate, complexContext);
      /* cardano is only null when there is a triple root. */
      assert(cardano.nullStatus(context) != ExpressionNode::NullStatus::Null);
      int loneRealRootIndex = -1;
      float minimalImaginaryPart = static_cast<float>(INFINITY);
      for (int i = 0; i < 3; i++) {
        /* The roots can be computed from Cardano's number using the cube roots
         * of unity. */
        Expression cz = Multiplication::Builder(cardano.clone(), roots[i]);
        roots[i] = Division::Builder(
            Addition::Builder({b.clone(), cz.clone(), Division::Builder(delta0, cz.clone())}),
            Multiplication::Builder(Rational::Builder(-3), a.clone()));
        if (approximate) {
          if (equationIsReal && deltaSign > 0) {
            /* delta > 0, the three solutions are real. We need to get rid of
             * the imaginary part that might have appeared. */
            roots[i]= RealPart::Builder(roots[i]);
          }
          roots[i] = roots[i].approximate<double>(context, complexContext.complexFormat(), complexContext.angleUnit());
          if (equationIsReal && deltaSign < 0) {
            /* We know there is exactly one real root (and two complex
             * conjugate). Because of approximation errors, this real root can
             * have an infinitesimal imaginary size. As such, we strip the
             * imaginary part from the root with the smallest imaginary part. */
            float im = std::fabs(ImaginaryPart::Builder(roots[i]).approximateToScalar<float>(context, complexContext.complexFormat(), complexContext.angleUnit()));
            if (im < minimalImaginaryPart) {
              minimalImaginaryPart = im;
              loneRealRootIndex = i;
            }
          }
        } else {
          roots[i] = roots[i].simplify(complexContext);
        }
      }
      if (loneRealRootIndex >= 0) {
        roots[loneRealRootIndex] = RealPart::Builder(roots[loneRealRootIndex]).approximate<double>(context, complexContext.complexFormat(), complexContext.angleUnit());
      }
      *root1 = roots[0];
      *root2 = roots[1];
      *root3 = roots[2];
    }
  }

  /* Simplify the results with the correct complexFormat */
  if (!approximate) {
    *root1 = root1->simplify(reductionContext);
    bool simplificationInterrupted = root1->isUninitialized() || root1->type() == ExpressionNode::Type::Undefined;
    if (root2->isUninitialized() || root2->type() != ExpressionNode::Type::Undefined) {
      *root2 = root2->simplify(reductionContext);
      simplificationInterrupted |= root2->isUninitialized() || root2->type() == ExpressionNode::Type::Undefined;
    }
    if (root3->isUninitialized() || root3->type() != ExpressionNode::Type::Undefined) {
      *root3 = root3->simplify(reductionContext);
      simplificationInterrupted |= root3->isUninitialized() || root3->type() == ExpressionNode::Type::Undefined;
    }
    if (simplificationInterrupted) {
      // Simplification has been interrupted, recompute approximated roots.
      approximate = true;
      if (approximateSolutions != nullptr) {
        *approximateSolutions = approximate;
      }
      return CubicPolynomialRoots(a, b, c, d, root1, root2, root3, delta, context, complexFormat, angleUnit, &approximate);
    }
  } else {
    *root1 = root1->approximate<double>(context, complexFormat, angleUnit);
    *root2 = root2->approximate<double>(context, complexFormat, angleUnit);
    *root3 = root3->approximate<double>(context, complexFormat, angleUnit);
  }
  assert(!(root1->isUninitialized() || root2->isUninitialized() || root3->isUninitialized()));

  /* Remove duplicates */
  if (root3->isIdenticalTo(*root1) || root3->isIdenticalTo(*root2)) {
    *root3 = Undefined::Builder();
  }
  if (root2->isIdenticalTo(*root1) || root2->isUndefined()) {
    *root2 = *root3;
    *root3 = Undefined::Builder();
  }
  if (root1->isUndefined()) {
    *root1 = *root2;
    *root2 = *root3;
    *root3 = Undefined::Builder();
  }

  /* Sort the roots. The real roots go first, in ascending order, then the
   * complex roots in order of ascending imaginary part. */
  void * pack[] = { root1, root2, root3, context, &complexFormat, &angleUnit };
  Helpers::Sort([](int i, int j, void * ctx, int n) { // Swap method
        assert(i < n && j < n);
        Expression ** tab = reinterpret_cast<Expression **>(reinterpret_cast<void **>(ctx));
        Expression t = *tab[i];
        *tab[i] = *tab[j];
        *tab[j] = t;
      },
      [](int i, int j, void * ctx, int n) { // Comparison method
        assert(i < n && j < n);
        void ** pack = reinterpret_cast<void **>(ctx);
        Expression ** tab = reinterpret_cast<Expression **>(pack);
        Context * context = reinterpret_cast<Context *>(pack[3]);
        Preferences::ComplexFormat complexFormat = *reinterpret_cast<Preferences::ComplexFormat *>(pack[4]);
        Preferences::AngleUnit angleUnit = *reinterpret_cast<Preferences::AngleUnit *>(pack[5]);
        return rootSmallerThan(tab[j], tab[i], context, complexFormat, angleUnit);
      }, pack, degree);

  if (approximateSolutions != nullptr) {
    *approximateSolutions = approximate;
  }
  return !root1->isUndefined() + !root2->isUndefined() + !root3->isUndefined();
}

Expression Polynomial::ReducePolynomial(const Expression * coefficients, int degree, Expression parameter, ExpressionNode::ReductionContext reductionContext) {
  Addition polynomial = Addition::Builder();
  polynomial.addChildAtIndexInPlace(coefficients[0].clone(), 0, 0);
  for (int i = 1; i <= degree; i++) {
    polynomial.addChildAtIndexInPlace(Multiplication::Builder(coefficients[i].clone(), Power::Builder(parameter.clone(), Rational::Builder(i))), i, i);
  }
  // Try to simplify polynomial
  Expression simplifiedReducedPolynomial = polynomial.clone().simplify(reductionContext);
  return simplifiedReducedPolynomial.isUninitialized() ? polynomial : simplifiedReducedPolynomial;
}

Rational Polynomial::ReduceRationalPolynomial(const Rational * coefficients, int degree, Rational parameter) {
  Rational result = coefficients[degree];
  for (int i = degree - 1; i <= 0; i--) {
    result = Rational::Addition(Rational::Multiplication(result, parameter), coefficients[i]);
  }
  return result;
}

Expression Polynomial::RationalRootSearch(const Expression * coefficients, int degree, ExpressionNode::ReductionContext reductionContext) {
  assert(degree <= Expression::k_maxPolynomialDegree);

  const Rational * rationalCoefficients = static_cast<const Rational *>(coefficients);
  LeastCommonMultiple lcm = LeastCommonMultiple::Builder();
  for (int i = 0; i <= degree; i++) {
    assert(coefficients[i].type() == ExpressionNode::Type::Rational);
    lcm.addChildAtIndexInPlace(Rational::Builder(rationalCoefficients[i].integerDenominator()), i, i);
  }
  Expression lcmResult = lcm.shallowReduce(reductionContext);
  assert(lcmResult.type() == ExpressionNode::Type::Rational);
  Rational rationalLCM = static_cast<Rational &>(lcmResult);

  Integer a0Int = Rational::Multiplication(static_cast<const Rational &>(coefficients[0]), rationalLCM).unsignedIntegerNumerator();
  Integer aNInt = Rational::Multiplication(static_cast<const Rational &>(coefficients[degree]), rationalLCM).unsignedIntegerNumerator();

  Integer a0Divisors[Arithmetic::k_maxNumberOfDivisors];
  int a0NumberOfDivisors, aNNumberOfDivisors;
  {
    ExceptionCheckpoint ecp;
    if (ExceptionRun(ecp)) {
      Arithmetic arithmetic;
      /* We need to compare two lists of divisors, but Arithmetic only allows
       * access to one list of factors. We thus need to store the first list in
       * its own buffer. */
      a0NumberOfDivisors = arithmetic.PositiveDivisors(a0Int);
      for (int i = 0; i < a0NumberOfDivisors; i++) {
        a0Divisors[i] = *arithmetic.divisorAtIndex(i);
      }
    } else {
      Arithmetic::resetLock();
      return Expression();
    }
  }

  {
    ExceptionCheckpoint ecp;
    if (ExceptionRun(ecp)) {
      Arithmetic arithmetic;
      aNNumberOfDivisors = arithmetic.PositiveDivisors(aNInt);
      for (int i = 0; i < a0NumberOfDivisors; i++) {
        for (int j = 0; j < aNNumberOfDivisors; j++) {
          /* If i and j are not coprime, i/j has already been tested. */
          Integer p = a0Divisors[i], q = *arithmetic.divisorAtIndex(j);
          if (Arithmetic::GCD(p, q).isOne()) {
            Rational r = Rational::Builder(p, q);
            if (ReduceRationalPolynomial(rationalCoefficients, degree, r).isZero()) {
              return std::move(r);
            }
            r = Rational::Multiplication(Rational::Builder(-1), r);
            if (ReduceRationalPolynomial(rationalCoefficients, degree, r).isZero()) {
              return std::move(r);
            }
          }
        }
      }
    } else {
      Arithmetic::resetLock();
      return Expression();
    }
  }

  return Expression();
}

Expression Polynomial::SumRootSearch(const Expression * coefficients, int degree, int relevantCoefficient, ExpressionNode::ReductionContext reductionContext) {
  Expression a = coefficients[degree];
  Expression b = coefficients[relevantCoefficient].clone();

  if (b.type() != ExpressionNode::Type::Addition) {
    Expression r = Opposite::Builder(Division::Builder(b, a.clone()));
    return IsRoot(coefficients, degree, r, reductionContext) ? r : Expression();
  }
  int n = b.numberOfChildren();
  for (int i = 0; i < n; i++) {
    Expression r = Opposite::Builder(Division::Builder(b.childAtIndex(i), a.clone()));
    if (IsRoot(coefficients, degree, r, reductionContext)) {
      return r;
    }
  }

  return Expression();
}

Expression Polynomial::CardanoNumber(Expression delta0, Expression delta1, bool * approximate, ExpressionNode::ReductionContext reductionContext) {
  assert(approximate != nullptr);

  /* C = root((delta1 ± sqrt(delta1^2 - 4*delta0^3)) / 2, 3)
   * The sign of ± must be chosen so that C is not null:
   *   - if delta0 is null, we enforce C = root(delta1, 3).
   *   - otherwise, ± takes the sign of delta1. This way, we do not run the
   *     risk of subtracting two very close numbers when delta0 << delta1. */

  Expression C;
  if (delta0.nullStatus(reductionContext.context()) == ExpressionNode::NullStatus::Null) {
    C = delta1.clone();
  } else {
    Expression rootDeltaDifference = SquareRoot::Builder(Subtraction::Builder(
          Power::Builder(delta1.clone(), Rational::Builder(2)),
          Multiplication::Builder(Rational::Builder(4), Power::Builder(delta0.clone(), Rational::Builder(3)))
          ));
    Expression diff;
    if (delta1.sign(reductionContext.context()) == ExpressionNode::Sign::Negative) {
      diff = Subtraction::Builder(delta1.clone(), rootDeltaDifference);
    } else {
      diff = Addition::Builder(delta1.clone(), rootDeltaDifference);
    }
    C = Division::Builder(diff, Rational::Builder(2));
  }
  C = NthRoot::Builder(C, Rational::Builder(3)).simplify(reductionContext);

  if (C.type() == ExpressionNode::Type::NthRoot || C.nullStatus(reductionContext.context()) == ExpressionNode::NullStatus::Unknown) {
    C = C.approximate<double>(reductionContext.context(), reductionContext.complexFormat(), reductionContext.angleUnit());
    *approximate = true;
  } else {
    *approximate = false;
  }
  assert(C.nullStatus(reductionContext.context()) != ExpressionNode::NullStatus::Null);
  return C;
}
}
