#include "student_law.h"

#include <float.h>
#include <poincare/regularized_incomplete_beta_function.h>
#include <poincare/solver.h>

#include <cmath>

#include "law_helper.h"

namespace Probability {

template <typename T>
T StudentLaw::EvaluateAtAbscissa(T x, T k) {
  return std::exp(lnCoefficient(k) - (k + 1.0) / 2.0 * std::log(1.0 + x * x / k));
}

template <typename T>
T StudentLaw::CumulativeDistributiveFunctionAtAbscissa(T x, T k) {
  if (x == 0.0) {
    return (T)0.5;
  }
  if (std::isinf(x)) {
    return x > 0 ? (T)1.0 : (T)0.0;
  }
  /* TODO There are some computation errors, where the probability falsly jumps to 1.
   * k = 0.001 and P(x < 42000000) (for 41000000 it is around 0.5)
   * k = 0.01 and P(x < 8400000) (for 41000000 it is around 0.6) */
  const double sqrtXSquaredPlusK = std::sqrt(x * x + k);
  double t = (x + sqrtXSquaredPlusK) / (2.0 * sqrtXSquaredPlusK);
  return Poincare::RegularizedIncompleteBetaFunction(k / 2.0, k / 2.0, t);
}

template <typename T>
T StudentLaw::CumulativeDistributiveInverseForProbability(T probability, T k) {
  if (probability == 0.5) {
    return (T)0.0;
  } else if (probability > 1.0 - DBL_EPSILON) {
    return INFINITY;
  } else if (probability < DBL_EPSILON) {
    return -INFINITY;
  }

  struct Args {
    T proba;
    T k;
  };
  Args args{probability, k};
  Poincare::Solver::ValueAtAbscissa evaluation =
      [](double x, Poincare::Context * context, const void * auxiliary) {
        const Args * args = static_cast<const Args *>(auxiliary);
        return static_cast<double>(CumulativeDistributiveFunctionAtAbscissa<T>(x, args->k) -
                                   args->proba);
      };

  double xmin, xmax;
  findBoundsForBinarySearch(evaluation, nullptr, &args, xmin, xmax);
  assert((xmin < xmax) && std::isfinite(xmin) && std::isfinite(xmax));

  // Compute inverse using Solver::IncreasingFunctionRoot
  Poincare::Coordinate2D<double> result =
      Poincare::Solver::IncreasingFunctionRoot(xmin, xmax, DBL_EPSILON, evaluation, nullptr, &args);
  return result.x1();
}

template <typename T>
T StudentLaw::lnCoefficient(T k) {
  return std::lgamma((k + 1.f) / 2.f) - std::lgamma(k / 2.f) - std::log(std::sqrt(k * M_PI));
}

// Specialisations
template float StudentLaw::EvaluateAtAbscissa<float>(float, float);
template double StudentLaw::EvaluateAtAbscissa<double>(double, double);
template float StudentLaw::lnCoefficient<float>(float);
template double StudentLaw::lnCoefficient<double>(double);
template float StudentLaw::CumulativeDistributiveFunctionAtAbscissa<float>(float, float);
template double StudentLaw::CumulativeDistributiveFunctionAtAbscissa<double>(double, double);
template float StudentLaw::CumulativeDistributiveInverseForProbability<float>(float, float);
template double StudentLaw::CumulativeDistributiveInverseForProbability<double>(double, double);

}  // namespace Probability
