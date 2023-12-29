#include <assert.h>
#include <poincare/binomial_coefficient.h>
#include <poincare/binomial_coefficient_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/parenthesis_layout.h>
#include <poincare/serialization_helper.h>

#include <algorithm>

namespace Poincare {

int BinomialCoefficientLayoutNode::indexAfterHorizontalCursorMove(
    OMG::HorizontalDirection direction, int currentIndex,
    bool* shouldRedrawLayout) {
  if (currentIndex == k_outsideIndex) {
    /* When coming from the left, go to the n layout.
     * When coming from the right, go to the k layout. */
    return direction.isRight() ? k_nLayoutIndex : k_kLayoutIndex;
  }
  return k_outsideIndex;
}

int BinomialCoefficientLayoutNode::indexAfterVerticalCursorMove(
    OMG::VerticalDirection direction, int currentIndex,
    PositionInLayout positionAtCurrentIndex, bool* shouldRedrawLayout) {
  if (currentIndex == k_kLayoutIndex && direction.isUp()) {
    return k_nLayoutIndex;
  }
  if (currentIndex == k_nLayoutIndex && direction.isDown()) {
    return k_kLayoutIndex;
  }
  return k_cantMoveIndex;
}

LayoutNode::DeletionMethod
BinomialCoefficientLayoutNode::deletionMethodForCursorLeftOfChild(
    int childIndex) const {
  if (childIndex == k_nLayoutIndex && kLayout()->isEmpty()) {
    return DeletionMethod::DeleteParent;
  }
  if (childIndex == k_kLayoutIndex) {
    return DeletionMethod::BinomialCoefficientMoveFromKtoN;
  }
  return DeletionMethod::MoveLeft;
}

size_t BinomialCoefficientLayoutNode::serialize(
    char* buffer, size_t bufferSize,
    Preferences::PrintFloatMode floatDisplayMode,
    int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(
      this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits,
      BinomialCoefficient::s_functionHelper.aliasesList().mainAlias(),
      SerializationHelper::ParenthesisType::System);
}

KDSize BinomialCoefficientLayoutNode::computeSize(KDFont::Size font) {
  KDSize coefficientsSize =
      KDSize(std::max(nLayout()->layoutSize(font).width(),
                      kLayout()->layoutSize(font).width()),
             knHeight(font));
  KDCoordinate width =
      coefficientsSize.width() + 2 * ParenthesisLayoutNode::k_parenthesisWidth;
  return KDSize(width, coefficientsSize.height());
}

KDCoordinate BinomialCoefficientLayoutNode::computeBaseline(KDFont::Size font) {
  return (knHeight(font) + 1) / 2;
}

KDPoint BinomialCoefficientLayoutNode::positionOfChild(LayoutNode* child,
                                                       KDFont::Size font) {
  KDCoordinate horizontalCenter =
      ParenthesisLayoutNode::k_parenthesisWidth +
      std::max(nLayout()->layoutSize(font).width(),
               kLayout()->layoutSize(font).width()) /
          2;
  if (child == nLayout()) {
    return KDPoint(horizontalCenter - nLayout()->layoutSize(font).width() / 2,
                   0);
  }
  assert(child == kLayout());
  return KDPoint(horizontalCenter - kLayout()->layoutSize(font).width() / 2,
                 knHeight(font) - kLayout()->layoutSize(font).height());
}

void BinomialCoefficientLayoutNode::render(KDContext* ctx, KDPoint p,
                                           KDGlyph::Style style) {
  // Render the parentheses.
  KDCoordinate childHeight = knHeight(style.font);
  KDCoordinate rightParenthesisPointX =
      std::max(nLayout()->layoutSize(style.font).width(),
               kLayout()->layoutSize(style.font).width()) +
      ParenthesisLayoutNode::k_parenthesisWidth;
  ParenthesisLayoutNode::RenderWithChildHeight(
      true, childHeight, ctx, p, style.glyphColor, style.backgroundColor);
  ParenthesisLayoutNode::RenderWithChildHeight(
      false, childHeight, ctx,
      p.translatedBy(KDPoint(rightParenthesisPointX, 0)), style.glyphColor,
      style.backgroundColor);
}

}  // namespace Poincare
