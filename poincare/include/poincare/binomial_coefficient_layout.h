#ifndef POINCARE_BINOMIAL_COEFFICIENT_LAYOUT_NODE_H
#define POINCARE_BINOMIAL_COEFFICIENT_LAYOUT_NODE_H

#include <poincare/two_rows_layout.h>

namespace Poincare {

class BinomialCoefficientLayoutNode final : public TwoRowsLayoutNode {
 public:
  using TwoRowsLayoutNode::TwoRowsLayoutNode;

  // Layout
  Type type() const override { return Type::BinomialCoefficientLayout; }

  // SerializableNode
  size_t serialize(char* buffer, size_t bufferSize,
                   Preferences::PrintFloatMode floatDisplayMode,
                   int numberOfSignificantDigits) const override;

  // TreeNode
  size_t size() const override { return sizeof(BinomialCoefficientLayoutNode); }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream& stream) const override {
    stream << "BinomialCoefficientLayout";
  }
#endif

 private:
  void render(KDContext* ctx, KDPoint p, KDGlyph::Style style) override;
};

class BinomialCoefficientLayout final
    : public LayoutTwoChildren<BinomialCoefficientLayout,
                               BinomialCoefficientLayoutNode> {
 public:
  BinomialCoefficientLayout() = delete;
};

}  // namespace Poincare

#endif
