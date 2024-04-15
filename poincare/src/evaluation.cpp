#include <poincare/addition.h>
#include <poincare/complex.h>
#include <poincare/evaluation.h>
#include <poincare/expression.h>
#include <poincare/matrix_complex.h>
#include <poincare/multiplication.h>

namespace Poincare {

template <typename T>
bool EvaluationNode<T>::isDefinedScalar() const {
  return !std::isnan(toScalar());
}

template <typename T>
bool EvaluationNode<T>::isDefinedPoint() const {
  return type() == EvaluationNode<T>::Type::PointEvaluation && !isUndefined();
}

template <typename T>
bool EvaluationNode<T>::isListOfDefinedScalars() const {
  if (type() != EvaluationNode<T>::Type::ListComplex) {
    return false;
  }
  const int n = numberOfChildren();
  assert(n > 0);
  for (int i = 0; i < n; i++) {
    if (!childAtIndex(i)->isDefinedScalar()) {
      return false;
    }
  }
  return true;
}

template <typename T>
bool EvaluationNode<T>::isListOfDefinedPoints() const {
  if (type() != EvaluationNode<T>::Type::ListComplex) {
    return false;
  }
  const int n = numberOfChildren();
  assert(n > 0);
  for (int i = 0; i < n; i++) {
    if (!childAtIndex(i)->isDefinedPoint()) {
      return false;
    }
  }
  return true;
}

template <typename T>
Evaluation<T> Evaluation<T>::childAtIndex(int i) const {
  TreeHandle c = TreeHandle::childAtIndex(i);
  return static_cast<Evaluation<T> &>(c);
}

template <typename T>
Expression Evaluation<T>::complexToExpression(
    Preferences::ComplexFormat complexFormat) const {
  return node()->complexToExpression(complexFormat);
}

template bool EvaluationNode<float>::isDefinedScalar() const;
template bool EvaluationNode<double>::isDefinedScalar() const;
template bool EvaluationNode<float>::isDefinedPoint() const;
template bool EvaluationNode<double>::isDefinedPoint() const;
template bool EvaluationNode<float>::isListOfDefinedScalars() const;
template bool EvaluationNode<double>::isListOfDefinedScalars() const;
template bool EvaluationNode<float>::isListOfDefinedPoints() const;
template bool EvaluationNode<double>::isListOfDefinedPoints() const;
template Evaluation<float> Evaluation<float>::childAtIndex(int i) const;
template Evaluation<double> Evaluation<double>::childAtIndex(int i) const;
template Expression Evaluation<float>::complexToExpression(
    Preferences::ComplexFormat) const;
template Expression Evaluation<double>::complexToExpression(
    Preferences::ComplexFormat) const;

}  // namespace Poincare
