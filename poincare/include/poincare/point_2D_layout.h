#ifndef POINCARE_POINT_2D_LAYOUT_NODE_H
#define POINCARE_POINT_2D_LAYOUT_NODE_H

#include <poincare/two_rows_layout.h>

namespace Poincare {

class Point2DLayoutNode final : public TwoRowsLayoutNode {
 public:
  using TwoRowsLayoutNode::TwoRowsLayoutNode;

  static KDSize SizeGivenChildSize(KDCoordinate width, KDCoordinate height) {
    return KDSize(width + 2 * k_parenthesisWidth,
                  2 * height + k_rowsSeparator +
                      2 * ParenthesisLayoutNode::VerticalMargin(height));
  }

  // Layout
  Type type() const override { return Type::Point2DLayout; }

  // SerializableNode
  size_t serialize(char* buffer, size_t bufferSize,
                   Preferences::PrintFloatMode floatDisplayMode,
                   int numberOfSignificantDigits) const override;

  // TreeNode
  size_t size() const override { return sizeof(Point2DLayoutNode); }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream& stream) const override {
    stream << "Point2DLayout";
  }
#endif

 private:
  constexpr static KDCoordinate k_rowsSeparator = 2;

  void render(KDContext* ctx, KDPoint p, KDGlyph::Style style) override;
  KDCoordinate upperMargin(KDFont::Size font) override {
    return ParenthesisLayoutNode::VerticalMargin(
        upperLayout()->layoutSize(font).height());
  }
  KDCoordinate lowerMargin(KDFont::Size font) override {
    return ParenthesisLayoutNode::VerticalMargin(
        lowerLayout()->layoutSize(font).height());
  }
  KDCoordinate rowsSeparator() override { return k_rowsSeparator; }
};

class Point2DLayout final
    : public LayoutTwoChildren<Point2DLayout, Point2DLayoutNode> {
 public:
  Point2DLayout() = delete;
};

}  // namespace Poincare

#endif
