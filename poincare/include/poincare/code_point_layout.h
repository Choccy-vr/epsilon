#ifndef POINCARE_CODEPOINT_LAYOUT_NODE_H
#define POINCARE_CODEPOINT_LAYOUT_NODE_H

#include <ion/unicode/code_point.h>
#include <poincare/layout.h>
#include <poincare/layout_cursor.h>
#include <poincare/serialization_helper.h>

namespace Poincare {

/* TODO: Make several code point classes depending on codepoint size?
 * (m_codePoint sometimes fits in a char, no need for a whole CodePoint */

class CodePointLayoutNode : public LayoutNode {
 public:
  static bool IsCodePoint(Layout l, CodePoint c);

  CodePointLayoutNode(CodePoint c = UCodePointNull)
      : LayoutNode(), m_codePoint(c) {}

  // Layout
  Type type() const override { return Type::CodePointLayout; }

  // CodePointLayout
  CodePoint codePoint() const { return m_codePoint; }

  // LayoutNode
  size_t serialize(char *buffer, size_t bufferSize,
                   Preferences::PrintFloatMode floatDisplayMode,
                   int numberOfSignificantDigits) const override;
  bool isCollapsable(int *numberOfOpenParenthesis,
                     OMG::HorizontalDirection direction) const override;

  // TreeNode
  size_t size() const override { return sizeof(CodePointLayoutNode); }
  int numberOfChildren() const override { return 0; }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream &stream) const override {
    stream << "CodePointLayout";
  }
  void logAttributes(std::ostream &stream) const override {
    constexpr int bufferSize = CodePoint::MaxCodePointCharLength + 1;
    char buffer[bufferSize];
    SerializationHelper::CodePoint(buffer, bufferSize, m_codePoint);
    stream << " CodePoint=\"" << buffer << "\"";
  }
#endif

  bool isMultiplicationCodePoint() const;

 protected:
  // LayoutNode
  KDSize computeSize(KDFont::Size font) override;
  KDCoordinate computeBaseline(KDFont::Size font) override;
  KDPoint positionOfChild(LayoutNode *child, KDFont::Size font) override {
    assert(false);
    return KDPointZero;
  }
  bool protectedIsIdenticalTo(Layout l) override;
  CodePoint m_codePoint;

 private:
  constexpr static const int k_middleDotWidth = 5;
  void render(KDContext *ctx, KDPoint p, KDGlyph::Style style) override;
};

class CodePointLayout : public Layout {
 public:
  CodePointLayout(const CodePointLayoutNode *n) : Layout(n) {}
  static CodePointLayout Builder(CodePoint c);
  CodePoint codePoint() const {
    return const_cast<CodePointLayout *>(this)->node()->codePoint();
  }

 private:
  using Layout::node;
  CodePointLayoutNode *node() {
    return static_cast<CodePointLayoutNode *>(Layout::node());
  }
};

}  // namespace Poincare

#endif
