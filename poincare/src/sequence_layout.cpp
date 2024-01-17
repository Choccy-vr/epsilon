#include <assert.h>
#include <poincare/code_point_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/parenthesis_layout.h>
#include <poincare/sequence_layout.h>
#include <poincare/sum_and_product.h>

#include <algorithm>
#include <array>

namespace Poincare {

int SequenceLayoutNode::indexAfterHorizontalCursorMove(
    OMG::HorizontalDirection direction, int currentIndex,
    bool *shouldRedrawLayout) {
  switch (currentIndex) {
    case k_outsideIndex:
      return direction.isRight() ? k_upperBoundLayoutIndex
                                 : k_argumentLayoutIndex;
    case k_upperBoundLayoutIndex:
      return direction.isRight() ? k_argumentLayoutIndex : k_outsideIndex;
    case k_variableLayoutIndex:
      return direction.isRight() ? k_lowerBoundLayoutIndex : k_outsideIndex;
    case k_lowerBoundLayoutIndex:
      return direction.isRight() ? k_argumentLayoutIndex
                                 : k_variableLayoutIndex;
    default:
      assert(currentIndex == k_argumentLayoutIndex);
      return direction.isRight() ? k_outsideIndex : k_lowerBoundLayoutIndex;
  }
}

int SequenceLayoutNode::indexAfterVerticalCursorMove(
    OMG::VerticalDirection direction, int currentIndex,
    PositionInLayout positionAtCurrentIndex, bool *shouldRedrawLayout) {
  if (direction.isUp() && ((currentIndex == k_variableLayoutIndex ||
                            currentIndex == k_lowerBoundLayoutIndex) ||
                           (positionAtCurrentIndex == PositionInLayout::Left &&
                            (currentIndex == k_outsideIndex ||
                             currentIndex == k_argumentLayoutIndex)))) {
    return k_upperBoundLayoutIndex;
  }

  if (direction.isDown() &&
      ((currentIndex == k_upperBoundLayoutIndex) ||
       (positionAtCurrentIndex == PositionInLayout::Left &&
        (currentIndex == k_outsideIndex ||
         currentIndex == k_argumentLayoutIndex)))) {
    return k_lowerBoundLayoutIndex;
  }
  return k_cantMoveIndex;
}

LayoutNode::DeletionMethod
SequenceLayoutNode::deletionMethodForCursorLeftOfChild(int childIndex) const {
  return StandardDeletionMethodForLayoutContainingArgument(
      childIndex, k_argumentLayoutIndex);
}

// Protected

KDSize SequenceLayoutNode::lowerBoundSizeWithVariableEquals(KDFont::Size font) {
  KDSize variableSize = variableLayout()->layoutSize(font);
  KDSize lowerBoundSize = lowerBoundLayout()->layoutSize(font);
  KDSize equalSize = KDFont::Font(font)->stringSize(k_equal);
  return KDSize(
      variableSize.width() + equalSize.width() + lowerBoundSize.width(),
      subscriptBaseline(font) +
          std::max(
              {variableSize.height() - variableLayout()->baseline(font),
               lowerBoundSize.height() - lowerBoundLayout()->baseline(font),
               equalSize.height() / 2}));
}

KDSize SequenceLayoutNode::computeSize(KDFont::Size font) {
  KDSize totalLowerBoundSize = lowerBoundSizeWithVariableEquals(font);
  KDSize argumentSize = argumentLayout()->layoutSize(font);
  KDSize argumentSizeWithParentheses = KDSize(
      argumentSize.width() + 2 * ParenthesisLayoutNode::k_parenthesisWidth,
      ParenthesisLayoutNode::Height(argumentSize.height()));
  KDSize result = KDSize(
      std::max({SymbolWidth(font), totalLowerBoundSize.width(),
                upperBoundWidth(font)}) +
          ArgumentHorizontalMargin(font) + argumentSizeWithParentheses.width(),
      baseline(font) +
          std::max(SymbolHeight(font) / 2 + LowerBoundVerticalMargin(font) +
                       totalLowerBoundSize.height(),
                   argumentSizeWithParentheses.height() -
                       argumentLayout()->baseline(font)));
  return result;
}

KDCoordinate SequenceLayoutNode::computeBaseline(KDFont::Size font) {
  return std::max<KDCoordinate>(upperBoundLayout()->layoutSize(font).height() +
                                    UpperBoundVerticalMargin(font) +
                                    (SymbolHeight(font) + 1) / 2,
                                argumentLayout()->baseline(font));
}

KDPoint SequenceLayoutNode::positionOfChild(LayoutNode *l, KDFont::Size font) {
  KDSize variableSize = variableLayout()->layoutSize(font);
  KDSize equalSize = KDFont::Font(font)->stringSize(k_equal);
  KDSize upperBoundSize = upperBoundLayout()->layoutSize(font);
  KDCoordinate x = 0;
  KDCoordinate y = 0;
  if (l == variableLayout()) {
    x = completeLowerBoundX(font);
    y = baseline(font) + SymbolHeight(font) / 2 +
        LowerBoundVerticalMargin(font) + subscriptBaseline(font) -
        variableLayout()->baseline(font);
  } else if (l == lowerBoundLayout()) {
    x = completeLowerBoundX(font) + equalSize.width() + variableSize.width();
    y = baseline(font) + SymbolHeight(font) / 2 +
        LowerBoundVerticalMargin(font) + subscriptBaseline(font) -
        lowerBoundLayout()->baseline(font);
  } else if (l == upperBoundLayout()) {
    x = std::max({0, (SymbolWidth(font) - upperBoundSize.width()) / 2,
                  (lowerBoundSizeWithVariableEquals(font).width() -
                   upperBoundSize.width()) /
                      2});
    y = baseline(font) - (SymbolHeight(font) + 1) / 2 -
        UpperBoundVerticalMargin(font) - upperBoundSize.height();
  } else if (l == argumentLayout()) {
    x = leftParenthesisPosition(font).x() +
        ParenthesisLayoutNode::k_parenthesisWidth;
    y = baseline(font) - argumentLayout()->baseline(font);
  } else {
    assert(false);
  }
  return KDPoint(x, y);
}

size_t SequenceLayoutNode::writeDerivedClassInBuffer(
    const char *operatorName, char *buffer, size_t bufferSize,
    Preferences::PrintFloatMode floatDisplayMode,
    int numberOfSignificantDigits) const {
  assert(operatorName != nullptr);
  if (bufferSize == 0) {
    return -1;
  }
  buffer[bufferSize - 1] = 0;

  // Write the operator name
  size_t numberOfChar = strlcpy(buffer, operatorName, bufferSize);
  if (numberOfChar >= bufferSize - 1) {
    return bufferSize - 1;
  }

  /* Add system parentheses to avoid serializing:
   *   2)+(1           2),1
   *    ∑     (5)  or   π    (5)
   *   n=1             n=1+binomial(3
   */
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, bufferSize - numberOfChar,
      UCodePointLeftSystemParenthesis);
  if (numberOfChar >= bufferSize - 1) {
    return bufferSize - 1;
  }

  LayoutNode *argLayouts[] = {
      const_cast<SequenceLayoutNode *>(this)->argumentLayout(),
      const_cast<SequenceLayoutNode *>(this)->variableLayout(),
      const_cast<SequenceLayoutNode *>(this)->lowerBoundLayout(),
      const_cast<SequenceLayoutNode *>(this)->upperBoundLayout()};
  for (uint8_t i = 0; i < std::size(argLayouts); i++) {
    if (i != 0) {
      // Write the comma
      numberOfChar += SerializationHelper::CodePoint(
          buffer + numberOfChar, bufferSize - numberOfChar, ',');
      if (numberOfChar >= bufferSize - 1) {
        return bufferSize - 1;
      }
    }
    // Write the child with system parentheses
    numberOfChar += SerializationHelper::CodePoint(
        buffer + numberOfChar, bufferSize - numberOfChar,
        UCodePointLeftSystemParenthesis);
    if (numberOfChar >= bufferSize - 1) {
      return bufferSize - 1;
    }
    numberOfChar += argLayouts[i]->serialize(
        buffer + numberOfChar, bufferSize - numberOfChar, floatDisplayMode,
        numberOfSignificantDigits);
    if (numberOfChar >= bufferSize - 1) {
      return bufferSize - 1;
    }
    numberOfChar += SerializationHelper::CodePoint(
        buffer + numberOfChar, bufferSize - numberOfChar,
        UCodePointRightSystemParenthesis);
    if (numberOfChar >= bufferSize - 1) {
      return bufferSize - 1;
    }
  }

  // Write the closing system parenthesis
  numberOfChar += SerializationHelper::CodePoint(
      buffer + numberOfChar, bufferSize - numberOfChar,
      UCodePointRightSystemParenthesis);
  return numberOfChar;
}

void SequenceLayoutNode::render(KDContext *ctx, KDPoint p,
                                KDGlyph::Style style) {
  KDFont::Size font = style.font;
  // Render the "="
  KDSize variableSize = variableLayout()->layoutSize(font);
  KDPoint equalPosition =
      positionOfChild(variableLayout(), font)
          .translatedBy(KDPoint(
              variableSize.width(),
              variableLayout()->baseline(font) -
                  KDFont::Font(font)->stringSize(k_equal).height() / 2));
  ctx->drawString(k_equal, equalPosition.translatedBy(p), style);

  // Render the parentheses
  KDSize argumentSize = argumentLayout()->layoutSize(font);
  ParenthesisLayoutNode::RenderWithChildHeight(
      true, argumentSize.height(), ctx,
      leftParenthesisPosition(font).translatedBy(p), style.glyphColor,
      style.backgroundColor);
  ParenthesisLayoutNode::RenderWithChildHeight(
      false, argumentSize.height(), ctx,
      rightParenthesisPosition(font, argumentSize).translatedBy(p),
      style.glyphColor, style.backgroundColor);
}

KDPoint SequenceLayoutNode::leftParenthesisPosition(KDFont::Size font) {
  KDSize argumentSize = argumentLayout()->layoutSize(font);
  KDCoordinate argumentBaseline = argumentLayout()->baseline(font);
  KDCoordinate lowerboundWidth = lowerBoundSizeWithVariableEquals(font).width();

  KDCoordinate x =
      std::max({SymbolWidth(font), lowerboundWidth, upperBoundWidth(font)}) +
      ArgumentHorizontalMargin(font);
  KDCoordinate y =
      baseline(font) -
      ParenthesisLayoutNode::Baseline(argumentSize.height(), argumentBaseline);
  return {x, y};
}

KDPoint SequenceLayoutNode::rightParenthesisPosition(KDFont::Size font,
                                                     KDSize argumentSize) {
  return leftParenthesisPosition(font).translatedBy(KDPoint(
      ParenthesisLayoutNode::k_parenthesisWidth + argumentSize.width(), 0));
}

KDCoordinate SequenceLayoutNode::completeLowerBoundX(KDFont::Size font) {
  return std::max(
      {0,
       (SymbolWidth(font) - lowerBoundSizeWithVariableEquals(font).width()) / 2,
       (upperBoundWidth(font) -
        lowerBoundSizeWithVariableEquals(font).width()) /
           2});
}

KDCoordinate SequenceLayoutNode::subscriptBaseline(KDFont::Size font) {
  return std::max<KDCoordinate>(
      std::max(variableLayout()->baseline(font),
               lowerBoundLayout()->baseline(font)),
      KDFont::Font(font)->stringSize(k_equal).height() / 2);
}

}  // namespace Poincare
