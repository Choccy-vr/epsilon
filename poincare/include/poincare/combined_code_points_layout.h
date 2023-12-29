#ifndef POINCARE_COMBINED_CODEPOINT_LAYOUT_NODE_H
#define POINCARE_COMBINED_CODEPOINT_LAYOUT_NODE_H

#include <poincare/code_point_layout.h>
#include <poincare/print_float.h>

namespace Poincare {

class CombinedCodePointsLayoutNode final : public CodePointLayoutNode {
 public:
  CombinedCodePointsLayoutNode(CodePoint mainCodePoint,
                               CodePoint combinedCodePoint)
      : CodePointLayoutNode(mainCodePoint),
        m_combinedCodePoint(combinedCodePoint) {}

  // Layout
  Type type() const override { return Type::CombinedCodePointsLayout; }

  // CodePointLayout
  CodePoint combinedCodePoint() const { return m_combinedCodePoint; }

  // LayoutNode
  size_t serialize(char *buffer, size_t bufferSize,
                   Preferences::PrintFloatMode floatDisplayMode,
                   int numberOfSignificantDigits) const override;

  // TreeNode
  size_t size() const override { return sizeof(CombinedCodePointsLayoutNode); }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream &stream) const override {
    stream << "CombinedCodePointsLayout";
  }
  void logAttributes(std::ostream &stream) const override {
    constexpr size_t bufferSize = 2 * CodePoint::MaxCodePointCharLength + 1;
    char buffer[bufferSize];
    serialize(buffer, bufferSize, Preferences::PrintFloatMode::Decimal,
              PrintFloat::k_floatNumberOfSignificantDigits);
    stream << " CombinedCodePoints=\"" << buffer << "\"";
  }
#endif

  /*
  bool isNotEqualOperator() const {
    return m_codePoint == '=' &&
           m_combinedCodePoint == UCodePointCombiningLongSolidusOverlay;
  }
  */

 private:
  void render(KDContext *ctx, KDPoint p, KDGlyph::Style style) override;
  bool protectedIsIdenticalTo(Layout l) override;

  CodePoint m_combinedCodePoint;
};

class CombinedCodePointsLayout final : public CodePointLayout {
 public:
  CombinedCodePointsLayout(const CodePointLayoutNode *n) : CodePointLayout(n) {}
  static CombinedCodePointsLayout Builder(CodePoint mainCodePoint,
                                          CodePoint CombinedCodePoints);
  CodePoint combinedCodePoint() const {
    return const_cast<CombinedCodePointsLayout *>(this)
        ->node()
        ->combinedCodePoint();
  }
  CombinedCodePointsLayoutNode *node() {
    return static_cast<CombinedCodePointsLayoutNode *>(Layout::node());
  }
};

}  // namespace Poincare

#endif
