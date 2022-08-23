#include <poincare/logarithm.h>
#include <poincare/multiplication.h>
#include <poincare/percent.h>
#include <poincare/power.h>
#include <poincare/unit_convert.h>

namespace Poincare {

/* Devirtualisation: methods here could be virtual but are overriden only a few
 * times so it's not worth introducing a vtable entry for them */

bool ExpressionNode::isNumber() const { return isOfType({Type::BasedInteger, Type::Decimal, Type::Double, Type::Float, Type::Infinity, Type::Nonreal, Type::Rational, Type::Undefined}); }

bool ExpressionNode::isRandom() const { return isOfType({Type::Random, Type::Randint}); }

bool ExpressionNode::isParameteredExpression() const { return isOfType({Type::Derivative, Type::Integral, Type::ListSequence, Type::Sum, Type::Product}); }

Expression ExpressionNode::denominator(const ReductionContext& reductionContext) const {
  if (type() == Type::Multiplication) {
    return Expression(this).convert<Multiplication>().denominator(reductionContext);
  }
  if (type() == Type::Power) {
    return Expression(this).convert<Power>().denominator(reductionContext);
  }
  if (type() == Type::Rational) {
    return Expression(this).convert<Rational>().denominator();
  }
  return Expression();
}

Expression ExpressionNode::deepBeautify(const ExpressionNode::ReductionContext& reductionContext) {
  if (type() == Type::UnitConvert) {
    return Expression(this).convert<UnitConvert>().deepBeautify(reductionContext);
  } else if (type() == Type::PercentAddition) {
    return Expression(this).convert<PercentAddition>().deepBeautify(reductionContext);
  } else {
    ExpressionNode::ReductionContext childContext = reductionContext;
    Expression e = shallowBeautify(childContext);
    SimplificationHelper::deepBeautifyChildren(e, childContext);
    return e;
  }
}

void ExpressionNode::deepReduceChildren(const ExpressionNode::ReductionContext& reductionContext) {
  if (type() == Type::Store) {
    return;
  }
  if (type() == Type::Logarithm && numberOfChildren()==2) {
    Expression(this).convert<Logarithm>().deepReduceChildren(reductionContext);
  }
  if (type() == Type::UnitConvert) {
    Expression(this).convert<UnitConvert>().deepReduceChildren(reductionContext);
  } else {
    SimplificationHelper::defaultDeepReduceChildren(Expression(this), reductionContext);
  }
}


}
