#include <assert.h>
#include <escher/metric.h>
#include <poincare/fraction_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>

#include <algorithm>

namespace Poincare {

int FractionLayoutNode::indexAfterHorizontalCursorMove(
    OMG::HorizontalDirection direction, int currentIndex,
    bool* shouldRedrawLayout) {
  if (currentIndex == k_outsideIndex) {
    /* When coming from the left, go to the numerator.
     * When coming from the right, go to the denominator. */
    return direction.isRight() ? k_numeratorIndex : k_denominatorIndex;
  }
  return k_outsideIndex;
}

int FractionLayoutNode::indexAfterVerticalCursorMove(
    OMG::VerticalDirection direction, int currentIndex,
    PositionInLayout positionAtCurrentIndex, bool* shouldRedrawLayout) {
  switch (currentIndex) {
    case k_outsideIndex:
      return direction.isUp() ? k_numeratorIndex : k_denominatorIndex;
    case k_numeratorIndex:
      return direction.isUp() ? k_cantMoveIndex : k_denominatorIndex;
    default:
      assert(currentIndex == k_denominatorIndex);
      return direction.isUp() ? k_numeratorIndex : k_cantMoveIndex;
  }
}

LayoutNode::DeletionMethod
FractionLayoutNode::deletionMethodForCursorLeftOfChild(int childIndex) const {
  return childIndex == k_denominatorIndex
             ? DeletionMethod::FractionDenominatorDeletion
             : DeletionMethod::MoveLeft;
}

size_t FractionLayoutNode::serialize(
    char* buffer, size_t bufferSize,
    Preferences::PrintFloatMode floatDisplayMode,
    int numberOfSignificantDigits) const {
  if (bufferSize == 0) {
    return bufferSize - 1;
  }
  buffer[bufferSize - 1] = 0;
  if (bufferSize == 1) {
    return bufferSize - 1;
  }

  /* Add System parenthesis to detect omitted multiplication:
   *   2
   *  --- i --> [2/3]i instead of 2/3i
   *   3
   */

  // Add system parenthesis
  size_t numberOfChar = SerializationHelper::CodePoint(
      buffer, bufferSize, UCodePointLeftSystemParenthesis);
  if (numberOfChar >= bufferSize - 1) {
    return bufferSize - 1;
  }

  // Write the content of the fraction
  numberOfChar += SerializationHelper::Infix(
      this, buffer + numberOfChar, bufferSize - numberOfChar, floatDisplayMode,
      numberOfSignificantDigits, "/");
  if (numberOfChar >= bufferSize - 1) {
    return bufferSize - 1;
  }

  // Add system parenthesis
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, bufferSize - numberOfChar,
      UCodePointRightSystemParenthesis);
  return numberOfChar;
}

int FractionLayoutNode::indexOfChildToPointToWhenInserting() {
  return numeratorLayout()->isEmpty() ? k_numeratorIndex : k_denominatorIndex;
}

bool FractionLayoutNode::isCollapsable(
    int* numberOfOpenParenthesis, OMG::HorizontalDirection direction) const {
  if (*numberOfOpenParenthesis > 0) {
    return true;
  }

  /* We do not want to absorb a fraction if something else is already being
   * absorbed. This way, the user can write a product of fractions without
   * typing the × sign. */
  Layout p = Layout(parent());
  assert(!p.isUninitialized() && p.isHorizontal() && p.numberOfChildren() > 1);
  int indexInParent = p.indexOfChild(Layout(this));
  int indexOfAbsorbingSibling = indexInParent + (direction.isLeft() ? 1 : -1);
  assert(indexOfAbsorbingSibling >= 0 &&
         indexOfAbsorbingSibling < p.numberOfChildren());
  Layout absorbingSibling = p.childAtIndex(indexOfAbsorbingSibling);
  if (absorbingSibling.numberOfChildren() > 0) {
    absorbingSibling = absorbingSibling.childAtIndex(
        direction.isLeft()
            ? absorbingSibling.leftCollapsingAbsorbingChildIndex()
            : absorbingSibling.rightCollapsingAbsorbingChildIndex());
  }
  return absorbingSibling.isHorizontal() && absorbingSibling.isEmpty();
}

KDSize FractionLayoutNode::computeSize(KDFont::Size font) {
  KDCoordinate width =
      std::max(numeratorLayout()->layoutSize(font).width(),
               denominatorLayout()->layoutSize(font).width()) +
      2 * Escher::Metric::FractionAndConjugateHorizontalOverflow +
      2 * Escher::Metric::FractionAndConjugateHorizontalMargin;
  KDCoordinate height = numeratorLayout()->layoutSize(font).height() +
                        k_fractionLineMargin + k_fractionLineHeight +
                        k_fractionLineMargin +
                        denominatorLayout()->layoutSize(font).height();
  return KDSize(width, height);
}

KDCoordinate FractionLayoutNode::computeBaseline(KDFont::Size font) {
  return numeratorLayout()->layoutSize(font).height() + k_fractionLineMargin +
         k_fractionLineHeight;
}

KDPoint FractionLayoutNode::positionOfChild(LayoutNode* child,
                                            KDFont::Size font) {
  KDCoordinate x = 0;
  KDCoordinate y = 0;
  if (child == numeratorLayout()) {
    x = (KDCoordinate)((layoutSize(font).width() -
                        numeratorLayout()->layoutSize(font).width()) /
                       2);
  } else if (child == denominatorLayout()) {
    x = (KDCoordinate)((layoutSize(font).width() -
                        denominatorLayout()->layoutSize(font).width()) /
                       2);
    y = (KDCoordinate)(numeratorLayout()->layoutSize(font).height() +
                       2 * k_fractionLineMargin + k_fractionLineHeight);
  } else {
    assert(false);
  }
  return KDPoint(x, y);
}

void FractionLayoutNode::render(KDContext* ctx, KDPoint p,
                                KDGlyph::Style style) {
  KDCoordinate fractionLineY =
      p.y() + numeratorLayout()->layoutSize(style.font).height() +
      k_fractionLineMargin;
  ctx->fillRect(
      KDRect(p.x() + Escher::Metric::FractionAndConjugateHorizontalMargin,
             fractionLineY,
             layoutSize(style.font).width() -
                 2 * Escher::Metric::FractionAndConjugateHorizontalMargin,
             k_fractionLineHeight),
      style.glyphColor);
}

}  // namespace Poincare
