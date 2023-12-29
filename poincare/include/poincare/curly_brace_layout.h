#ifndef POINCARE_CURLY_BRACE_LAYOUT_H
#define POINCARE_CURLY_BRACE_LAYOUT_H

#include <poincare/autocompleted_bracket_pair_layout.h>
#include <poincare/layout_helper.h>

namespace Poincare {

class CurlyBraceLayoutNode : public AutocompletedBracketPairLayoutNode {
 public:
  constexpr static KDCoordinate k_curveHeight = 6;
  constexpr static KDCoordinate k_curveWidth = 5;
  constexpr static KDCoordinate k_centerHeight = 3;
  constexpr static KDCoordinate k_centerWidth = 3;
  constexpr static KDCoordinate k_widthMargin = 1;
  constexpr static KDCoordinate k_minVerticalMargin = 1;
  constexpr static KDCoordinate k_curlyBraceWidth =
      2 * k_widthMargin + k_centerWidth + k_curveWidth - k_lineThickness;

  static void RenderWithChildHeight(bool left, KDCoordinate childHeight,
                                    KDContext* ctx, KDPoint p,
                                    KDColor expressionColor,
                                    KDColor backgroundColor);
  static KDCoordinate Height(KDCoordinate childHeight) {
    return BracketPairLayoutNode::Height(childHeight, k_minVerticalMargin);
  }
  static KDCoordinate Baseline(KDCoordinate childHeight,
                               KDCoordinate childBaseline) {
    return BracketPairLayoutNode::Baseline(childHeight, childBaseline,
                                           k_minVerticalMargin);
  }

  // LayoutNode
  Type type() const override { return Type::CurlyBraceLayout; }

  // TreeNode
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream& stream) const override {
    stream << "CurlyBraceLayout";
  }
#endif
  size_t serialize(char* buffer, size_t bufferSize,
                   Preferences::PrintFloatMode floatDisplayMode,
                   int numberOfSignificantDigits) const override {
    return serializeWithSymbol('{', '}', buffer, bufferSize, floatDisplayMode,
                               numberOfSignificantDigits);
  }

 private:
  // BracketPairLayoutNode
  KDCoordinate bracketWidth() const override { return k_curlyBraceWidth; }
  KDCoordinate minVerticalMargin() const override {
    return k_minVerticalMargin;
  }
  void renderOneBracket(bool left, KDContext* ctx, KDPoint p,
                        KDGlyph::Style style) override {
    RenderWithChildHeight(left, childLayout()->layoutSize(style.font).height(),
                          ctx, p,
                          bracketColor(left ? Side::Left : Side::Right,
                                       style.glyphColor, style.backgroundColor),
                          style.backgroundColor);
  }
};

class CurlyBraceLayout final
    : public LayoutOneChild<CurlyBraceLayout, CurlyBraceLayoutNode> {
 public:
  CurlyBraceLayout() = delete;
  static CurlyBraceLayout Builder() {
    return Builder(HorizontalLayout::Builder());
  }
  static CurlyBraceLayout Builder(Layout l) {
    // Ensure curly brace layout has always an horizontal child
    return LayoutOneChild<CurlyBraceLayout, CurlyBraceLayoutNode>::Builder(
        l.isHorizontal() ? l : HorizontalLayout::Builder(l));
  }
};

}  // namespace Poincare

#endif
