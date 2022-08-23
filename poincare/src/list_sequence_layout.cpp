#include <poincare/list_sequence_layout.h>
#include <poincare/code_point_layout.h>
#include <poincare/curly_brace_layout.h>
#include <poincare/list_sequence.h>
#include <poincare/serialization_helper.h>
#include <escher/metric.h>
#include <algorithm>

namespace Poincare {

int ListSequenceLayoutNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, ListSequence::s_functionHelper.name(), true);
}

void ListSequenceLayoutNode::moveCursorLeft(LayoutCursor * cursor, bool * shouldRecomputeLayout, bool forSelection) {
  if (cursor->layoutNode() == variableLayout()) {
    assert(cursor->position() == LayoutCursor::Position::Left);
    cursor->setLayoutNode(functionLayout());
    cursor->setPosition(LayoutCursor::Position::Right);
    return;
  }
  if (cursor->layoutNode() == functionLayout()) {
    assert(cursor->position() == LayoutCursor::Position::Left);
    cursor->setLayoutNode(this);
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  if (cursor->layoutNode() == upperBoundLayout()) {
    assert(cursor->position() == LayoutCursor::Position::Left);
    cursor->setLayoutNode(variableLayout());
    cursor->setPosition(LayoutCursor::Position::Right);
    return;
  }
  assert(cursor->layoutNode() == this);
  if (cursor->position() == LayoutCursor::Position::Right) {
    cursor->setLayoutNode(upperBoundLayout());
    cursor->setPosition(LayoutCursor::Position::Right);
    return;
  }
  assert(cursor->position() == LayoutCursor::Position::Left);
  LayoutNode * parentNode = parent();
  if (parentNode) {
    parentNode->moveCursorLeft(cursor, shouldRecomputeLayout);
  }
}

void ListSequenceLayoutNode::moveCursorRight(LayoutCursor * cursor, bool * shouldRecomputeLayout, bool forSelection) {
  if (cursor->layoutNode() == variableLayout()) {
    assert(cursor->position() == LayoutCursor::Position::Right);
    cursor->setLayoutNode(upperBoundLayout());
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  if (cursor->layoutNode() == functionLayout()) {
    assert(cursor->position() == LayoutCursor::Position::Right);
    cursor->setLayoutNode(variableLayout());
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  if (cursor->layoutNode() == upperBoundLayout()) {
    assert(cursor->position() == LayoutCursor::Position::Right);
    cursor->setLayoutNode(this);
    cursor->setPosition(LayoutCursor::Position::Right);
    return;
  }
  assert(cursor->layoutNode() == this);
  if (cursor->position() == LayoutCursor::Position::Left) {
    cursor->setLayoutNode(functionLayout());
    cursor->setPosition(LayoutCursor::Position::Left);
    return;
  }
  assert(cursor->position() == LayoutCursor::Position::Right);
  LayoutNode * parentNode = parent();
  if (parentNode) {
    parentNode->moveCursorRight(cursor, shouldRecomputeLayout);
  }
}

Layout ListSequenceLayoutNode::XNTLayout(int childIndex) const {
  if (childIndex == k_functionLayoutIndex) {
    return Layout(childAtIndex(k_variableLayoutIndex)).clone();
  }
  if (childIndex == k_variableLayoutIndex) {
    return CodePointLayout::Builder(CodePoint(ListSequence::k_defaultXNTChar));
  }
  return LayoutNode::XNTLayout();
}

KDSize ListSequenceLayoutNode::computeSize() {
  KDPoint upperBoundPosition = positionOfChild(upperBoundLayout());
  KDSize upperBoundSize = upperBoundLayout()->layoutSize();
  return KDSize(
      upperBoundPosition.x() + upperBoundSize.width(),
      std::max(upperBoundPosition.y() + upperBoundSize.height(), positionOfVariable().y() + variableLayout()->layoutSize().height()));
}

KDPoint ListSequenceLayoutNode::positionOfChild(LayoutNode * child) {
  if (child == variableLayout()) {
    return positionOfVariable();
  }
  if (child == functionLayout()) {
    return KDPoint(CurlyBraceLayoutNode::k_curlyBraceWidth, baseline() - functionLayout()->baseline());
  }
  assert(child == upperBoundLayout());
  return KDPoint(
      positionOfVariable().x() + variableLayout()->layoutSize().width() + KDFont::LargeFont->stringSize("≤").width(),
      variableSlotBaseline() - upperBoundLayout()->baseline());
}

KDPoint ListSequenceLayoutNode::positionOfVariable() {
  return KDPoint(
     k_variableHorizontalMargin + bracesWidth(),
     variableSlotBaseline() - variableLayout()->baseline());
}

KDCoordinate ListSequenceLayoutNode::variableSlotBaseline() {
  return std::max({static_cast<int>(functionLayout()->layoutSize().height() + CurlyBraceLayoutNode::k_verticalInternalMargin + k_variableBaselineOffset),
                  static_cast<int>(upperBoundLayout()->baseline()),
                  static_cast<int>(variableLayout()->baseline())});
}

KDCoordinate ListSequenceLayoutNode::computeBaseline() {
  return variableSlotBaseline() - k_variableBaselineOffset - functionLayout()->layoutSize().height() - CurlyBraceLayoutNode::k_verticalInternalMargin / 2 + functionLayout()->baseline();
}

KDCoordinate ListSequenceLayoutNode::bracesWidth() {
  return 2 * CurlyBraceLayoutNode::k_curlyBraceWidth + functionLayout()->layoutSize().width();
}

void ListSequenceLayoutNode::render(KDContext * ctx, KDPoint p, KDColor expressionColor, KDColor backgroundColor, Layout * selectionStart, Layout * selectionEnd, KDColor selectionColor) {
  KDSize functionSize = functionLayout()->layoutSize();

  // Draw {  }
  KDPoint leftBracePosition(p.x(), p.y() + baseline() - functionLayout()->baseline() - CurlyBraceLayoutNode::k_verticalInternalMargin / 2);
  LeftCurlyBraceLayoutNode::RenderWithChildHeight(functionSize.height(), ctx, leftBracePosition, expressionColor, backgroundColor);

  KDPoint rightBracePosition(leftBracePosition.x() + CurlyBraceLayoutNode::k_curlyBraceWidth + functionSize.width(), leftBracePosition.y());
  RightCurlyBraceLayoutNode::RenderWithChildHeight(functionSize.height(), ctx, rightBracePosition, expressionColor, backgroundColor);

  // Draw k=...
  ctx->drawString("≤", p.translatedBy(KDPoint(positionOfVariable().x() + variableLayout()->layoutSize().width(), variableSlotBaseline() - KDFont::LargeFont->glyphSize().height() / 2)), KDFont::LargeFont, expressionColor, backgroundColor);

}

}
