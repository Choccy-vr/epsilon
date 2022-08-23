#ifndef POINCARE_ADDITION_H
#define POINCARE_ADDITION_H

#include <poincare/approximation_helper.h>
#include <poincare/n_ary_infix_expression.h>
#include <poincare/rational.h>

namespace Poincare {

class AdditionNode final : public NAryInfixExpressionNode {
  friend class Addition;
public:
  using NAryInfixExpressionNode::NAryInfixExpressionNode;

  // Tree
  size_t size() const override { return sizeof(AdditionNode); }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "Addition";
  }
#endif

  // ExpressionNode

  // Properties
  Type type() const override { return Type::Addition; }
  Sign sign(Context * context) const override;
  int polynomialDegree(Context * context, const char * symbolName) const override;
  int getPolynomialCoefficients(Context * context, const char * symbolName, Expression coefficients[]) const override;

  // Evaluation
  template<typename T> static Complex<T> computeOnComplex(const std::complex<T> c, const std::complex<T> d, Preferences::ComplexFormat complexFormat) { return Complex<T>::Builder(c+d); }
  template<typename T> static MatrixComplex<T> computeOnMatrices(const MatrixComplex<T> m, const MatrixComplex<T> n, Preferences::ComplexFormat complexFormat) {
    return ApproximationHelper::ElementWiseOnMatrices(m, n, complexFormat, computeOnComplex<T>);
  }
  template<typename T> static Evaluation<T> Compute(Evaluation<T> eval1, Evaluation<T> eval2, Preferences::ComplexFormat complexFormat) {
    return ApproximationHelper::Reduce<T>(
        eval1,
        eval2,
        complexFormat,
        computeOnComplex<T>,
        ApproximationHelper::UndefinedOnComplexAndMatrix<T>,
        ApproximationHelper::UndefinedOnMatrixAndComplex<T>,
        computeOnMatrices<T>);
  }

  // Simplification
  LayoutShape leftLayoutShape() const override {
    /* When beautifying a Multiplication of Additions, Parentheses are added
     * around Additions. As leftLayoutShape is called after beautifying, we
     * should never call it on an Addition. */
    assert(false);
    return NAryExpressionNode::leftLayoutShape();
  }
  LayoutShape rightLayoutShape() const override {
    assert(false);
    return NAryExpressionNode::rightLayoutShape();
  }
private:
  // Layout
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;

  // Simplification
  Expression shallowBeautify(ReductionContext * reductionContext) override;
  Expression shallowReduce(const ReductionContext& reductionContext) override;

  // Derivation
  bool derivate(const ReductionContext& reductionContext, Symbol symbol, Expression symbolValue) override;

  /* Evaluation */
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::MapReduce<float>(this, approximationContext, Compute<float>);
   }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override {
    return ApproximationHelper::MapReduce<double>(this, approximationContext, Compute<double>);
  }
};

class Addition final : public NAryExpression {
public:
  Addition(const AdditionNode * n) : NAryExpression(n) {}
  static Addition Builder(const Tuple & children = {}) {
    return TreeHandle::NAryBuilder<Addition, AdditionNode>(convert(children));
  }
  // TODO: Get rid of these two helper functions
  static Addition Builder(Expression e1) { return Addition::Builder({e1}); }
  static Addition Builder(Expression e1, Expression e2) { return Addition::Builder({e1, e2}); }
  // Expression
  Expression shallowBeautify(ExpressionNode::ReductionContext * reductionContext);
  Expression shallowReduce(const ExpressionNode::ReductionContext& reductionContext);
  bool derivate(const ExpressionNode::ReductionContext& reductionContext, Symbol symbol, Expression symbolValue);
  int getPolynomialCoefficients(Context * context, const char * symbolName, Expression coefficients[]) const;
  void sortChildrenInPlace(NAryExpressionNode::ExpressionOrder order, Context * context) {
    NAryExpression::sortChildrenInPlace(order, context, true);
  }

private:
  static const Number NumeralFactor(const Expression & e);
  static inline int NumberOfNonNumeralFactors(const Expression & e);
  static inline const Expression FirstNonNumeralFactor(const Expression & e);

  static bool TermsHaveIdenticalNonNumeralFactors(const Expression & e1, const Expression & e2, Context * context);
  Expression factorizeOnCommonDenominator(const ExpressionNode::ReductionContext& reductionContext);
  void factorizeChildrenAtIndexesInPlace(int index1, int index2, const ExpressionNode::ReductionContext& reductionContext);
  AdditionNode * node() const { return static_cast<AdditionNode *>(Expression::node()); }
};

}

#endif
