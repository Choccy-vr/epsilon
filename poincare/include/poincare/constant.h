#ifndef POINCARE_CONSTANT_H
#define POINCARE_CONSTANT_H

#include <poincare/symbol_abstract.h>

namespace Poincare {

/* TODO: Also keep a m_codePoint ? Redundant with m_name, but faster constants
 * comparison */

class ConstantNode final : public SymbolAbstractNode {
public:
  ConstantNode(const char * newName, int length);

  const char * name() const override { return m_name; }

  // TreeNode
  int numberOfChildren() const override { return 0; }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream & stream) const override {
    stream << "Constant";
  }
#endif

  // Complex
  bool isReal() const;

  // Expression Properties
  Type type() const override { return constantInfo().unit() == nullptr ? Type::ConstantMaths : Type::ConstantPhysics; }
  Sign sign(Context * context) const override;
  NullStatus nullStatus(Context * context) const override { return NullStatus::NonNull; }

  /* Layout */
  int serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;
  Layout createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const override;

  /* Approximation */
  Evaluation<float> approximate(SinglePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<float>(); }
  Evaluation<double> approximate(DoublePrecision p, ApproximationContext approximationContext) const override { return templatedApproximate<double>(); }

  /* Constant properties */
  class ConstantInfo {
  public:
    constexpr ConstantInfo() : m_name(nullptr), m_unit(nullptr), m_value(NAN), m_comparisonRank(0) {}
    constexpr ConstantInfo(const char * name, int comparisonRank, double value = NAN, const char * unit = nullptr) : m_name(name), m_unit(unit), m_value(value), m_comparisonRank(comparisonRank) {}
    const char * name() const { return m_name; }
    const char * unit() const { return m_unit; }
    double value() const { return m_value; }
    int comparisonRank() const { return m_comparisonRank; }
  private:
    const char * m_name;
    const char * m_unit;
    double m_value;
    int m_comparisonRank;
  };
  ConstantInfo constantInfo() const;
  bool isConstant(const char * constant, ConstantInfo info = ConstantInfo()) const;

  // Comparison
  int simplificationOrderSameType(const ExpressionNode * e, bool ascending, bool ignoreParentheses) const override;

  // Simplification
  Expression shallowReduce(ReductionContext reductionContext) override;
  LayoutShape leftLayoutShape() const override { return LayoutShape::OneLetter; };

  /* Derivation */
  bool derivate(ReductionContext reductionContext, Symbol symbol, Expression symbolValue) override;
private:
  int rankOfConstant() const { return constantInfo().comparisonRank(); }
  size_t nodeSize() const override { return sizeof(ConstantNode); }
  template<typename T> Evaluation<T> templatedApproximate() const;

  char m_name[0]; // MUST be the last member variable
};

class Constant final : public SymbolAbstract {
public:
  Constant(const ConstantNode * node) : SymbolAbstract(node) {}
  static Constant Builder(const char * name, int length) {
    assert(Constant::IsConstant(name, length));
    return SymbolAbstract::Builder<Constant, ConstantNode>(name, length);
  }
  static Constant Builder(const char * name) {
    assert(Constant::IsConstant(name, strlen(name)));
    return Builder(name, strlen(name));
  }

  // Constant properties
  bool isConstant(const char * constantName, ConstantNode::ConstantInfo info = ConstantNode::ConstantInfo()) const { return node()->isConstant(constantName, info); }
  static bool IsConstant(const char * name, size_t length);

  // Simplification
  Expression shallowReduce(ExpressionNode::ReductionContext reductionContext);
  bool derivate(ExpressionNode::ReductionContext reductionContext, Symbol symbol, Expression symbolValue);

  static constexpr ConstantNode::ConstantInfo k_constants[] = {
    ConstantNode::ConstantInfo("i", 0),
    ConstantNode::ConstantInfo("π", 1, M_PI),
    ConstantNode::ConstantInfo("e", 2, M_E),
    ConstantNode::ConstantInfo("_c", 3, 299792458.0, "_m/_s"),
    ConstantNode::ConstantInfo("_e", 3, 1.602176634e-19, "_C"),
    ConstantNode::ConstantInfo("_G", 3, 6.67430e-11, "_m^3*_kg^-1*_s^-2"),
    ConstantNode::ConstantInfo("_g0", 3, 9.80665, "_m/_s^2"),
    ConstantNode::ConstantInfo("_k", 3, 1.380649e-23, "_J/_K"),
    ConstantNode::ConstantInfo("_ke", 3, 8.9875517923e9, "_N*_m^2/_C^2"),
    ConstantNode::ConstantInfo("_me", 3, 9.1093837015e-31, "_kg"),
    ConstantNode::ConstantInfo("_mn", 3, 1.67492749804e-27, "_kg"),
    ConstantNode::ConstantInfo("_mp", 3, 1.67262192369e-27, "_kg"),
    ConstantNode::ConstantInfo("_Na", 3, 6.02214076e23, "_mol^-1"),
    ConstantNode::ConstantInfo("_R", 3, 8.31446261815324, "_J*_mol^-1*_K^-1"),
    ConstantNode::ConstantInfo("_ε0", 3, 8.8541878128e-12, "_F/_m"),
    ConstantNode::ConstantInfo("_μ0", 3, 1.25663706212e-6, "_N*_A^-2")
  };

private:
  ConstantNode::ConstantInfo constantInfo() const { return node()->constantInfo(); }
  ConstantNode * node() const { return static_cast<ConstantNode *>(Expression::node()); }
};

}

#endif
