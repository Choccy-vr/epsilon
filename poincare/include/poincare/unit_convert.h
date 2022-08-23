#ifndef POINCARE_UNIT_CONVERT_H
#define POINCARE_UNIT_CONVERT_H

#include <poincare/rightwards_arrow_expression.h>
#include <poincare/unit.h>
#include <poincare/evaluation.h>

namespace Poincare {

class UnitConvertNode /*final*/ : public RightwardsArrowExpressionNode {
public:
  // TreeNode
  size_t size() const override { return sizeof(UnitConvertNode); }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "UnitConvert";
  }
#endif
  // ExpressionNode
  Type type() const override { return Type::UnitConvert; }

private:
  Expression removeUnit(Expression * unit) override;
  // Simplification
  Expression shallowBeautify(ReductionContext * reductionContext) override;
  // Evalutation
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(approximationContext); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(approximationContext); }
  template<typename T> Evaluation<T> templatedApproximate(ApproximationContext approximationContext) const;
};

class UnitConvert final : public ExpressionTwoChildren<UnitConvert, UnitConvertNode> {
friend class UnitConvertNode;
public:
  using ExpressionBuilder::ExpressionBuilder;
  // Expression
  void deepReduceChildren(ExpressionNode::ReductionContext reductionContext);
  Expression deepBeautify(ExpressionNode::ReductionContext reductionContext);
  Expression shallowBeautify(ExpressionNode::ReductionContext * reductionContext);
private:
  UnitConvertNode * node() const { return static_cast<UnitConvertNode *>(Expression::node()); }
};

}

#endif
