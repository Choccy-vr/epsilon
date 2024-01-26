#include <ion/unicode/utf8_decoder.h>
#include <poincare/binomial_coefficient_layout.h>
#include <poincare/code_point_layout.h>
#include <poincare/combined_code_points_layout.h>
#include <poincare/curly_brace_layout.h>
#include <poincare/fraction_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/input_beautification.h>
#include <poincare/layout.h>
#include <poincare/layout_cursor.h>
#include <poincare/matrix_layout.h>
#include <poincare/nth_root_layout.h>
#include <poincare/parenthesis_layout.h>
#include <poincare/vertical_offset_layout.h>

#include <algorithm>

namespace Poincare {

void LayoutCursor::safeSetLayout(Layout layout,
                                 OMG::HorizontalDirection sideOfLayout) {
  LayoutCursor previousCursor = *this;
  setLayout(layout, sideOfLayout);
  didEnterCurrentPosition(previousCursor);
}

void LayoutCursor::safeSetPosition(int position) {
  assert(position >= 0);
  assert(position <= RightmostPossibleCursorPosition(m_layout));
  assert(!isSelecting());
  LayoutCursor previousCursor = *this;
  m_position = position;
  didEnterCurrentPosition(previousCursor);
}

KDCoordinate LayoutCursor::cursorHeight(KDFont::Size font) {
  LayoutSelection currentSelection = selection();
  if (currentSelection.isEmpty()) {
    return layoutToFit(font).layoutSize(font).height();
  }

  if (m_layout.isHorizontal()) {
    return static_cast<HorizontalLayoutNode *>(m_layout.node())
        ->layoutSizeBetweenIndexes(currentSelection.leftPosition(),
                                   currentSelection.rightPosition(), font)
        .height();
  }

  return m_layout.layoutSize(font).height();
}

KDPoint LayoutCursor::cursorAbsoluteOrigin(KDFont::Size font) {
  KDCoordinate cursorBaseline = 0;
  LayoutSelection currentSelection = selection();
  if (!currentSelection.isEmpty() && m_layout.isHorizontal()) {
    cursorBaseline =
        static_cast<HorizontalLayout &>(m_layout).baselineBetweenIndexes(
            currentSelection.leftPosition(), currentSelection.rightPosition(),
            font);
  } else {
    cursorBaseline = layoutToFit(font).baseline(font);
  }
  KDCoordinate cursorYOriginInLayout = m_layout.baseline(font) - cursorBaseline;
  KDCoordinate cursorXOffset = 0;
  if (m_layout.isHorizontal()) {
    cursorXOffset = static_cast<HorizontalLayout &>(m_layout)
                        .layoutSizeBetweenIndexes(0, m_position, font)
                        .width();
  } else {
    cursorXOffset = m_position == 1 ? m_layout.layoutSize(font).width() : 0;
  }
  return m_layout.absoluteOrigin(font).translatedBy(
      KDPoint(cursorXOffset, cursorYOriginInLayout));
}

KDPoint LayoutCursor::middleLeftPoint(KDFont::Size font) {
  KDPoint origin = cursorAbsoluteOrigin(font);
  return KDPoint(origin.x(), origin.y() + cursorHeight(font) / 2);
}

/* Move */
bool LayoutCursor::move(OMG::Direction direction, bool selecting,
                        bool *shouldRedrawLayout, Context *context) {
  *shouldRedrawLayout = false;
  if (!selecting && isSelecting()) {
    resetSelection();
    *shouldRedrawLayout = true;
    return true;
  }
  if (selecting && !isSelecting()) {
    privateStartSelecting();
  }
  LayoutCursor cloneCursor = *this;
  bool moved = false;
  if (direction.isVertical()) {
    moved = verticalMove(direction, shouldRedrawLayout);
  } else {
    moved = horizontalMove(direction, shouldRedrawLayout);
  }
  assert(!*shouldRedrawLayout || moved);
  if (moved) {
    *shouldRedrawLayout = selecting || *shouldRedrawLayout;
    if (cloneCursor.layout() != m_layout) {
      // Beautify the layout that was just left
      LayoutCursor rightmostPositionOfPreviousLayout(cloneCursor.layout(),
                                                     OMG::Direction::Right());
      *shouldRedrawLayout =
          InputBeautification::BeautifyLeftOfCursorBeforeCursorMove(
              &rightmostPositionOfPreviousLayout, context) ||
          *shouldRedrawLayout;
    }
    // Ensure that didEnterCurrentPosition is always called by being left of ||
    *shouldRedrawLayout =
        didEnterCurrentPosition(cloneCursor) || *shouldRedrawLayout;
  }

  if (isSelecting() && selection().isEmpty()) {
    resetSelection();
  }

  if (*shouldRedrawLayout) {
    invalidateSizesAndPositions();
  }
  return moved;
}

bool LayoutCursor::moveMultipleSteps(OMG::Direction direction, int step,
                                     bool selecting, bool *shouldRedrawLayout,
                                     Context *context) {
  assert(step > 0);
  for (int i = 0; i < step; i++) {
    if (!move(direction, selecting, shouldRedrawLayout, context)) {
      return i > 0;
    }
  }
  return true;
}

static bool IsEmptyChildOfGridLayout(Layout l) {
  Layout parent = l.parent();
  return l.isEmpty() && !parent.isUninitialized() &&
         GridLayoutNode::IsGridLayoutType(parent.type());
}

static Layout LeftOrRightmostLayout(Layout l,
                                    OMG::HorizontalDirection direction) {
  return l.isHorizontal()
             ? (l.childAtIndex(direction.isLeft() ? 0
                                                  : l.numberOfChildren() - 1))
             : l;
}

static bool IsTemporaryAutocompletedBracketPair(
    Layout l, AutocompletedBracketPairLayoutNode::Side tempSide) {
  return AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
             l.type()) &&
         static_cast<AutocompletedBracketPairLayoutNode *>(l.node())
             ->isTemporary(tempSide);
}

// Return leftParenthesisIndex
static int ReplaceCollapsableLayoutsLeftOfIndexWithParenthesis(
    HorizontalLayout l, int index) {
  int dummy = 0;
  int leftParenthesisIndex = index + 1;
  while (leftParenthesisIndex > 0 &&
         l.childAtIndex(leftParenthesisIndex - 1)
             .isCollapsable(&dummy, OMG::Direction::Left())) {
    leftParenthesisIndex--;
  }
  HorizontalLayout h = HorizontalLayout::Builder();
  int i = index;
  while (i >= leftParenthesisIndex) {
    Layout child = l.childAtIndex(i);
    l.removeChildAtIndexInPlace(i);
    h.addOrMergeChildAtIndex(child, 0);
    i--;
  }
  ParenthesisLayout p = ParenthesisLayout::Builder(h);
  l.addOrMergeChildAtIndex(p, leftParenthesisIndex);
  return leftParenthesisIndex;
}

/* Layout insertion */
void LayoutCursor::insertLayout(Layout layout, Context *context,
                                bool forceRight, bool forceLeft) {
  assert(!isUninitialized() && isValid());
  if (layout.isEmpty()) {
    return;
  }
  assert(!forceRight || !forceLeft);
  // - Step 1 - Delete selection
  deleteAndResetSelection();

  // - Step 2 - Beautify the current layout if needed.
  InputBeautification::BeautificationMethod beautificationMethod =
      InputBeautification::BeautificationMethodWhenInsertingLayout(layout);
  if (beautificationMethod.beautifyIdentifiersBeforeInserting) {
    InputBeautification::BeautifyLeftOfCursorBeforeCursorMove(this, context);
  }

  /* - Step 3 - Add empty row to grid layout if needed
   * When an empty child at the bottom or right of the grid is filled,
   * an empty row/column is added below/on the right.
   */
  if (IsEmptyChildOfGridLayout(m_layout)) {
    static_cast<GridLayoutNode *>(m_layout.parent().node())
        ->willFillEmptyChildAtIndex(m_layout.parent().indexOfChild(m_layout));
  }

  /* - Step 4 - Close brackets on the left/right
   *
   * For example, if the current layout is "(3+4]|" (where "|"" is the cursor
   * and "]" is a temporary parenthesis), inserting something on the right
   * should make the parenthesis permanent.
   * "(3+4]|" -> insert "x" -> "(3+4)x|"
   *
   * There is an exception to this: If a new bracket of the same type, temporary
   * on the other side, is inserted, you only want to make the inner brackets
   * permanent.
   *
   * Examples:
   * "(3+4]|" -> insert "[)" -> "(3+4][)|"
   * The newly inserted one is temporary on its other side, so the current
   * bracket is not made permanent.
   * Later at Step 9, balanceAutocompletedBrackets will make it so:
   * "(3+4][)|" -> "(3+4)|"
   *
   * "(1+(3+4]]|" -> insert "[)" -> "(1+(3+4)][)|"
   * The newly inserted one is temporary on its other side, so the current
   * bracket is not made permanent, but its inner bracket is made permanent.
   * Later at Step 9, balanceAutocompletedBrackets will make it so:
   * "(1+(3+4)][)|" -> "(1+3(3+4))|"
   * */
  Layout leftL = leftLayout();
  Layout rightL = rightLayout();
  if (!leftL.isUninitialized() &&
      AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
          leftL.type())) {
    Layout leftMostInsertedLayout =
        LeftOrRightmostLayout(layout, OMG::Direction::Left());
    bool makeLeftLPerma = leftMostInsertedLayout.type() != leftL.type() ||
                          !IsTemporaryAutocompletedBracketPair(
                              leftMostInsertedLayout,
                              AutocompletedBracketPairLayoutNode::Side::Left);
    static_cast<AutocompletedBracketPairLayoutNode *>(leftL.node())
        ->makeChildrenPermanent(AutocompletedBracketPairLayoutNode::Side::Right,
                                makeLeftLPerma);
  }
  if (!rightL.isUninitialized() &&
      AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
          rightL.type())) {
    Layout rightMostInsertedLayout =
        LeftOrRightmostLayout(layout, OMG::Direction::Right());
    bool makeRightLPerma = rightMostInsertedLayout.type() != rightL.type() ||
                           !IsTemporaryAutocompletedBracketPair(
                               rightMostInsertedLayout,
                               AutocompletedBracketPairLayoutNode::Side::Right);
    static_cast<AutocompletedBracketPairLayoutNode *>(rightL.node())
        ->makeChildrenPermanent(AutocompletedBracketPairLayoutNode::Side::Left,
                                makeRightLPerma);
  }

  /* - Step 5 - Add parenthesis around vertical offset
   * To avoid ambiguity between a^(b^c) and (a^b)^c when representing a^b^c,
   * add parentheses to make (a^b)^c. */
  if (m_layout.isHorizontal() &&
      layout.type() == LayoutNode::Type::VerticalOffsetLayout &&
      static_cast<VerticalOffsetLayout &>(layout).isSuffixSuperscript()) {
    if (!leftL.isUninitialized() &&
        leftL.type() == LayoutNode::Type::VerticalOffsetLayout &&
        static_cast<VerticalOffsetLayout &>(leftL).isSuffixSuperscript()) {
      // Insert ^c left of a^b -> turn a^b into (a^b)
      int leftParenthesisIndex =
          ReplaceCollapsableLayoutsLeftOfIndexWithParenthesis(
              static_cast<HorizontalLayout &>(m_layout),
              m_layout.indexOfChild(leftL));
      m_position = leftParenthesisIndex + 1;
    }

    if (!rightL.isUninitialized() &&
        rightL.type() == LayoutNode::Type::VerticalOffsetLayout &&
        static_cast<VerticalOffsetLayout &>(rightL).isSuffixSuperscript() &&
        m_layout.indexOfChild(rightL) > 0) {
      // Insert ^b right of a in a^c -> turn a^c into (a)^c
      int leftParenthesisIndex =
          ReplaceCollapsableLayoutsLeftOfIndexWithParenthesis(
              static_cast<HorizontalLayout &>(m_layout),
              m_layout.indexOfChild(rightL) - 1);
      m_layout = m_layout.childAtIndex(leftParenthesisIndex).childAtIndex(0);
      m_position = m_layout.numberOfChildren();
    }
  }

  // - Step 6 - Find position to point to if layout will me merged
  LayoutCursor previousCursor = *this;
  Layout layoutToPoint;
  OMG::HorizontalDirection directionOfLayoutToPoint = OMG::Direction::Left();
  bool layoutToInsertIsHorizontal = layout.isHorizontal();
  if (layoutToInsertIsHorizontal) {
    layoutToPoint = (forceRight || forceLeft)
                        ? Layout()
                        : static_cast<HorizontalLayout &>(layout)
                              .deepChildToPointToWhenInserting();
    if (!layoutToPoint.isUninitialized() &&
        AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
            layoutToPoint.type())) {
      layoutToPoint = layoutToPoint.childAtIndex(0);
    }
  }

  // - Step 7 - Insert layout
  int numberOfInsertedChildren =
      layout.isHorizontal() ? layout.numberOfChildren() : 1;

  if (!m_layout.isHorizontal()) {
    /* Replace the current layout with an HorizontalLayout so that a sibling
     * can be added to it. */
    assert(m_layout.parent().isUninitialized() ||
           !m_layout.parent().isHorizontal());
    HorizontalLayout newParent = HorizontalLayout::Builder();
    m_layout.replaceWithInPlace(newParent);
    newParent.addOrMergeChildAtIndex(m_layout, 0);
    m_layout = newParent;
  }
  assert(m_layout.isHorizontal());
  static_cast<HorizontalLayout &>(m_layout).addOrMergeChildAtIndex(layout,
                                                                   m_position);

  if (!forceLeft) {
    // Move cursor right of inserted children
    m_position += numberOfInsertedChildren;
  }

  /* - Step 8 - Collapse siblings and find position to point to if layout was
   * not merged */
  if (!layoutToInsertIsHorizontal) {
    collapseSiblingsOfLayout(layout);
    if (forceRight || forceLeft) {
      layoutToPoint = layout;
      directionOfLayoutToPoint =
          forceLeft ? OMG::Direction::Left() : OMG::Direction::Right();
    } else {
      layoutToPoint =
          layout.childAtIndex(layout.indexOfChildToPointToWhenInserting());
    }
  }

  // - Step 9 - Point to required position
  if (!layoutToPoint.isUninitialized()) {
    *this = LayoutCursor(layoutToPoint, directionOfLayoutToPoint);
    didEnterCurrentPosition(previousCursor);
  }

  // - Step 10 - Balance brackets
  balanceAutocompletedBracketsAndKeepAValidCursor();

  // - Step 11 - Beautify after insertion if needed
  if (beautificationMethod.beautifyAfterInserting) {
    InputBeautification::BeautifyLeftOfCursorAfterInsertion(this, context);
  }

  // - Step 12 - Invalidate layout sizes and positions
  invalidateSizesAndPositions();
}

void LayoutCursor::addEmptyExponentialLayout(Context *context) {
  insertLayout(
      HorizontalLayout::Builder(
          CodePointLayout::Builder('e'),
          VerticalOffsetLayout::Builder(
              HorizontalLayout::Builder(),
              VerticalOffsetLayoutNode::VerticalPosition::Superscript)),
      context);
}

void LayoutCursor::addEmptyMatrixLayout(Context *context) {
  insertLayout(MatrixLayout::EmptyMatrixBuilder(), context);
}

void LayoutCursor::addEmptySquareRootLayout(Context *context) {
  insertLayout(NthRootLayout::Builder(HorizontalLayout::Builder()), context);
}

void LayoutCursor::addEmptyPowerLayout(Context *context) {
  insertLayout(VerticalOffsetLayout::Builder(
                   HorizontalLayout::Builder(),
                   VerticalOffsetLayoutNode::VerticalPosition::Superscript),
               context);
}

void LayoutCursor::addEmptySquarePowerLayout(Context *context) {
  /* Force the cursor right of the layout. */
  insertLayout(VerticalOffsetLayout::Builder(
                   CodePointLayout::Builder('2'),
                   VerticalOffsetLayoutNode::VerticalPosition::Superscript),
               context, true);
}

void LayoutCursor::addEmptyTenPowerLayout(Context *context) {
  insertLayout(
      HorizontalLayout::Builder(
          {CodePointLayout::Builder(UCodePointMultiplicationSign),
           CodePointLayout::Builder('1'), CodePointLayout::Builder('0'),
           VerticalOffsetLayout::Builder(
               HorizontalLayout::Builder(),
               VerticalOffsetLayoutNode::VerticalPosition::Superscript)}),
      context);
}

void LayoutCursor::addFractionLayoutAndCollapseSiblings(Context *context) {
  insertLayout(FractionLayout::Builder(HorizontalLayout::Builder(),
                                       HorizontalLayout::Builder()),
               context);
}

void LayoutCursor::insertText(const char *text, Context *context,
                              bool forceCursorRightOfText,
                              bool forceCursorLeftOfText, bool linearMode) {
  UTF8Decoder decoder(text);

  CodePoint codePoint = decoder.nextCodePoint();
  if (codePoint == UCodePointNull) {
    return;
  }

  /* - Step 1 -
   * Read the text from left to right and create an Horizontal layout
   * containing the layouts corresponding to each code point. */
  HorizontalLayout layoutToInsert = HorizontalLayout::Builder();
  HorizontalLayout currentLayout = layoutToInsert;

  bool setCursorToFirstEmptyCodePoint =
      linearMode && !forceCursorLeftOfText && !forceCursorRightOfText;

  while (codePoint != UCodePointNull) {
    assert(!codePoint.isCombining());
    CodePoint nextCodePoint = decoder.nextCodePoint();
    if (codePoint == UCodePointEmpty) {
      codePoint = nextCodePoint;
      if (setCursorToFirstEmptyCodePoint) {
        /* To force cursor at first empty code point in linear mode, insert
         * the first half of text now, and then insert the end of the text
         * and force the cursor left of it. */
        insertLayout(layoutToInsert, context, forceCursorRightOfText,
                     forceCursorLeftOfText);
        layoutToInsert = HorizontalLayout::Builder();
        currentLayout = layoutToInsert;
        forceCursorLeftOfText = true;
        setCursorToFirstEmptyCodePoint = false;
      }
      assert(!codePoint.isCombining());
      continue;
    }

    /* TODO: The insertion of subscripts should be replaced with a parser
     * that creates layout. This is a draft of this. */

    /* - Step 1.1 - Handle subscripts
     * Subscripts are serialized as "\x14{...\x14}". When the code points
     * "\x14{" are encountered by the decoder, create a subscript layout
     * and continue insertion in it. When the code points "\x14}" are
     * encountered, leave the subscript and continue the insertion in its
     * parent. */
    if (codePoint == UCodePointSystem) {
      /* UCodePointSystem should be inserted only for system braces.
       * Sometimes though, a code point system can be alone at the end of the
       * inserted text. This happens for example when copy-pasting a very long
       * text containing subscripts. The '{' or '}' codepoint can be cropped
       * because the clipboard buffer is too short. */
      if (nextCodePoint == UCodePointNull) {
        break;
      }
      assert(nextCodePoint == '{' || nextCodePoint == '}');
      if (linearMode) {
        // Convert u\x14{n\x14} into u(n)
        codePoint = nextCodePoint == '{' ? '(' : ')';
        nextCodePoint = decoder.nextCodePoint();
      } else {
        if (nextCodePoint == '{') {
          // Enter a subscript
          Layout newChild = VerticalOffsetLayout::Builder(
              HorizontalLayout::Builder(),
              VerticalOffsetLayoutNode::VerticalPosition::Subscript);
          currentLayout.addOrMergeChildAtIndex(
              newChild, currentLayout.numberOfChildren());
          Layout horizontalChildOfSubscript = newChild.childAtIndex(0);
          assert(horizontalChildOfSubscript.isEmpty());
          currentLayout =
              static_cast<HorizontalLayout &>(horizontalChildOfSubscript);
          codePoint = decoder.nextCodePoint();
          continue;
        }
        // UCodePointSystem should be inserted only for system braces
        assert(nextCodePoint == '}');
        // Leave the subscript
        Layout subscript = currentLayout;
        while (subscript.type() != LayoutNode::Type::VerticalOffsetLayout) {
          subscript = subscript.parent();
          assert(!subscript.isUninitialized());
        }
        Layout parentOfSubscript = subscript.parent();
        assert(!parentOfSubscript.isUninitialized() &&
               parentOfSubscript.isHorizontal());
        currentLayout = static_cast<HorizontalLayout &>(parentOfSubscript);
        codePoint = decoder.nextCodePoint();
        continue;
      }
    }

    // - Step 1.2 - Handle code points and brackets
    Layout newChild;
    LayoutNode::Type bracketType;
    AutocompletedBracketPairLayoutNode::Side bracketSide;
    if (!linearMode &&
        AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairCodePoint(
            codePoint, &bracketType, &bracketSide)) {
      // Brackets will be balanced later in insertLayout
      newChild =
          AutocompletedBracketPairLayoutNode::BuildFromBracketType(bracketType);
      static_cast<AutocompletedBracketPairLayoutNode *>(newChild.node())
          ->setTemporary(
              AutocompletedBracketPairLayoutNode::OtherSide(bracketSide), true);
    } else if (nextCodePoint.isCombining()) {
      newChild = CombinedCodePointsLayout::Builder(codePoint, nextCodePoint);
      nextCodePoint = decoder.nextCodePoint();
    } else {
      if (codePoint == UCodePointLeftSystemParenthesis ||
          codePoint == UCodePointRightSystemParenthesis) {
        assert(linearMode);  // Handled earlier if not in linear
        codePoint = codePoint == UCodePointLeftSystemParenthesis ? '(' : ')';
      }
      newChild = CodePointLayout::Builder(codePoint);
    }
    currentLayout.addOrMergeChildAtIndex(newChild,
                                         currentLayout.numberOfChildren());
    codePoint = nextCodePoint;
  }

  // - Step 2 - Insert the created layout
  insertLayout(layoutToInsert, context, forceCursorRightOfText,
               forceCursorLeftOfText);
}

void LayoutCursor::performBackspace() {
  if (isSelecting()) {
    deleteAndResetSelection();
    return;
  }

  LayoutCursor previousCursor = *this;
  Layout leftL = leftLayout();
  if (!leftL.isUninitialized()) {
    LayoutNode::DeletionMethod deletionMethod =
        leftL.deletionMethodForCursorLeftOfChild(LayoutNode::k_outsideIndex);
    privateDelete(deletionMethod, false);
  } else {
    assert(m_position == leftMostPosition());
    Layout p = m_layout.parent();
    if (p.isUninitialized()) {
      return;
    }
    LayoutNode::DeletionMethod deletionMethod =
        p.deletionMethodForCursorLeftOfChild(p.indexOfChild(m_layout));
    privateDelete(deletionMethod, true);
  }
  balanceAutocompletedBracketsAndKeepAValidCursor();
  removeEmptyRowOrColumnOfGridParentIfNeeded();
  didEnterCurrentPosition(previousCursor), invalidateSizesAndPositions();
}

void LayoutCursor::deleteAndResetSelection() {
  LayoutSelection selec = selection();
  if (selec.isEmpty()) {
    return;
  }
  int selectionLeftBound = selec.leftPosition();
  int selectionRightBound = selec.rightPosition();
  if (m_layout.isHorizontal()) {
    for (int i = selectionLeftBound; i < selectionRightBound; i++) {
      static_cast<HorizontalLayout &>(m_layout).removeChildAtIndexInPlace(
          selectionLeftBound);
    }
  } else {
    assert(m_layout.parent().isUninitialized() ||
           !m_layout.parent().isHorizontal());
    Layout hLayout = HorizontalLayout::Builder();
    m_layout.replaceWithInPlace(hLayout);
    m_layout = hLayout;
  }
  m_position = selectionLeftBound;
  resetSelection();
  balanceAutocompletedBracketsAndKeepAValidCursor();
  removeEmptyRowOrColumnOfGridParentIfNeeded();
  didEnterCurrentPosition();
  invalidateSizesAndPositions();
}

bool LayoutCursor::didEnterCurrentPosition(LayoutCursor previousPosition) {
  bool changed = false;
  if (!previousPosition.isUninitialized() && previousPosition.isValid()) {
    /* Order matters: First show the empty rectangle at position, because when
     * leaving a piecewise operator layout, empty rectangles can be set back
     * to Hidden. */
    changed = previousPosition.setEmptyRectangleVisibilityAtCurrentPosition(
                  EmptyRectangle::State::Visible) ||
              changed;
    changed = previousPosition.layout().deleteGraySquaresBeforeLeavingGrid(
                  m_layout) ||
              changed;
    if (changed) {
      previousPosition.invalidateSizesAndPositions();
    }
  }
  if (isUninitialized()) {
    return changed;
  }
  assert(isValid());
  /* Order matters: First enter the grid, because when entering a piecewise
   * operator layout, empty rectangles can be set back to Visible. */
  changed =
      m_layout.createGraySquaresAfterEnteringGrid(previousPosition.layout()) ||
      changed;
  changed = setEmptyRectangleVisibilityAtCurrentPosition(
                EmptyRectangle::State::Hidden) ||
            changed;
  if (changed) {
    invalidateSizesAndPositions();
  }
  return changed;
}

void LayoutCursor::prepareForExitingPosition() {
  if (IsEmptyChildOfGridLayout(m_layout)) {
    /* When exiting a grid, the gray columns and rows will disappear, so
     * before leaving the grid, set the cursor position to a layout that will
     * stay valid when the grid will be re-entered. */
    GridLayoutNode *parentGrid =
        static_cast<GridLayoutNode *>(m_layout.parent().node());
    setLayout(Layout(parentGrid->childAtIndex(parentGrid->closestNonGrayIndex(
                  parentGrid->indexOfChild(m_layout.node())))),
              OMG::Direction::Right());
  }
}

bool LayoutCursor::didExitPosition() {
  prepareForExitingPosition();
  LayoutCursor lc;
  return lc.didEnterCurrentPosition(*this);
}

bool LayoutCursor::isAtNumeratorOfEmptyFraction() const {
  return m_layout.numberOfChildren() == 0 &&
         !m_layout.parent().isUninitialized() &&
         m_layout.parent().type() == LayoutNode::Type::FractionLayout &&
         m_layout.parent().indexOfChild(m_layout) == 0 &&
         m_layout.parent().childAtIndex(1).numberOfChildren() == 0;
}

int LayoutCursor::RightmostPossibleCursorPosition(Layout l) {
  return l.isHorizontal() ? l.numberOfChildren() : 1;
}

void LayoutCursor::beautifyLeft(Context *context) {
  if (InputBeautification::BeautifyLeftOfCursorBeforeCursorMove(this,
                                                                context)) {
    invalidateSizesAndPositions();
  }
}

/* Private */

void LayoutCursor::setLayout(Layout l, OMG::HorizontalDirection sideOfLayout) {
  if (!l.isHorizontal() && !l.parent().isUninitialized() &&
      l.parent().isHorizontal()) {
    m_layout = l.parent();
    m_position = m_layout.indexOfChild(l) + (sideOfLayout.isRight());
    return;
  }
  m_layout = l;
  m_position = sideOfLayout.isLeft() ? leftMostPosition() : rightmostPosition();
}

Layout LayoutCursor::leftLayout() {
  assert(!isUninitialized());
  if (!m_layout.isHorizontal()) {
    return m_position == 1 ? m_layout : Layout();
  }
  if (m_layout.numberOfChildren() == 0 || m_position == 0) {
    return Layout();
  }
  return m_layout.childAtIndex(m_position - 1);
}

Layout LayoutCursor::rightLayout() {
  assert(!isUninitialized());
  if (!m_layout.isHorizontal()) {
    return m_position == 0 ? m_layout : Layout();
  }
  if (m_layout.numberOfChildren() == 0 ||
      m_position == m_layout.numberOfChildren()) {
    return Layout();
  }
  return m_layout.childAtIndex(m_position);
}

Layout LayoutCursor::layoutToFit(KDFont::Size font) {
  assert(!isUninitialized());
  Layout leftL = leftLayout();
  Layout rightL = rightLayout();
  if (leftL.isUninitialized() && rightL.isUninitialized()) {
    return m_layout;
  }
  return leftL.isUninitialized() || (!rightL.isUninitialized() &&
                                     leftL.layoutSize(font).height() <
                                         rightL.layoutSize(font).height())
             ? rightL
             : leftL;
}

bool LayoutCursor::horizontalMove(OMG::HorizontalDirection direction,
                                  bool *shouldRedrawLayout) {
  Layout nextLayout = Layout();
  /* Search the nextLayout on the left/right to ask it where
   * the cursor should go when entering from outside.
   *
   * Example in the layout of 3+4/5 :
   * § is the cursor and -> the direction
   *
   *       4
   * 3+§->---
   *       5
   *
   * Here the cursor must move right but should not "jump" over the fraction,
   * so will ask its rightLayout (the fraction), where it should enter
   * (numerator or denominator).
   *
   * Example in the layout of 12+34:
   * § is the cursor and -> the direction
   *
   * 12+§->34
   *
   * Here the cursor will ask its rightLayout (the "3"), where it should go.
   * This will result in the "3" answering "outside", so that the cursor
   * jumps over it.
   * */
  int currentIndexInNextLayout = LayoutNode::k_outsideIndex;
  if (direction.isRight()) {
    nextLayout = rightLayout();
  } else {
    nextLayout = leftLayout();
  }

  if (nextLayout.isUninitialized()) {
    /* If nextLayout is uninitialized, the cursor is at the left-most or
     * right-most position. It should move to the parent.
     *
     * Example in an integral layout:
     * § is the cursor and -> the direction
     *
     *   / 10§->
     *  /
     *  |
     *  |     1+ln(x) dx
     *  |
     *  /
     * / -10
     *
     * Here the cursor must move right but has no rightLayout. So it will
     * ask its parent what it should do when leaving its upper bound child
     * from the right (go to integrand).
     *
     * Example in a square root layout:
     * § is the cursor and -> the direction
     *  _______
     * √1234§->
     *
     * Here the cursor must move right but has no rightLayout. So it will
     * ask its parent what it should do when leaving its only child from
     * from the right (leave the square root).
     * */
    if (m_layout.parent().isUninitialized()) {
      return false;
    }
    nextLayout = m_layout.parent();
    currentIndexInNextLayout = nextLayout.indexOfChild(m_layout);
  }
  assert(!nextLayout.isUninitialized());
  assert(!nextLayout.isHorizontal());

  /* If the cursor is selecting, it should not enter a new layout
   * but select all of it. */
  int newIndex = isSelecting() ? LayoutNode::k_outsideIndex
                               : nextLayout.indexAfterHorizontalCursorMove(
                                     direction, currentIndexInNextLayout,
                                     shouldRedrawLayout);
  assert(newIndex != LayoutNode::k_cantMoveIndex);

  if (newIndex != LayoutNode::k_outsideIndex) {
    /* Enter the next layout child
     *
     *       4                                        §4
     * 3+§->---          : newIndex = numerator ==> 3+---
     *       5                                         5
     *
     *
     *   / 10§->                                     / 10
     *  /                                           /
     *  |                                           |
     *  |     1+ln(x) dx : newIndex = integrand ==> |     §1+ln(x) dx
     *  |                                           |
     *  /                                           /
     * / -10                                       / -10
     *
     * */
    m_layout = nextLayout.childAtIndex(newIndex);
    m_position = direction.isRight() ? leftMostPosition() : rightmostPosition();
    return true;
  }

  /* The new index is outside.
   * If it's not selecting, it can be because there is no child to go into:
   *
   * 12+§->34  : newIndex = outside of the 3    ==> 12+3§4
   *
   *  _______                                        ____ §
   * √1234§->  : newIndex = outside of the sqrt ==> √1234 §
   *
   * If it's selecting, the cursor should always leave the layout and all of
   * it will be selected.
   *
   *   / 10§->                                   / 10          §
   *  /                                         /              §
   *  |                                         |              §
   *  |     1+ln(x) dx : newIndex = outside ==> |   1+ln(x) dx §
   *  |                                         |              §
   *  /                                         /              §
   * / -10                                     / -10           §
   *
   * */
  Layout parent = nextLayout.parent();
  Layout previousLayout = m_layout;
  if (!parent.isUninitialized() && parent.isHorizontal()) {
    m_layout = parent;
    m_position = m_layout.indexOfChild(nextLayout) + (direction.isRight());
  } else {
    m_layout = nextLayout;
    m_position = direction.isRight();
  }

  if (isSelecting() && m_layout != previousLayout) {
    /* If the cursor went into the parent, start the selection before
     * the layout that was just left (or after depending on the direction
     * of the selection). */
    m_startOfSelection = m_position + (direction.isRight() ? -1 : 1);
  }
  return true;
}

bool LayoutCursor::verticalMove(OMG::VerticalDirection direction,
                                bool *shouldRedrawLayout) {
  Layout previousLayout = m_layout;
  bool moved = verticalMoveWithoutSelection(direction, shouldRedrawLayout);

  // Handle selection (find a common ancestor to previous and current layout)
  if (moved && isSelecting() && previousLayout != m_layout) {
    TreeHandle commonAncestor = m_layout.commonAncestorWith(previousLayout);
    assert(!commonAncestor.isUninitialized());
    Layout layoutAncestor = static_cast<Layout &>(commonAncestor);
    // Down goes left to right and up goes right to left
    setLayout(layoutAncestor, direction.isUp() ? OMG::Direction::Left()
                                               : OMG::Direction::Right());
    m_startOfSelection = m_position + (direction.isUp() ? 1 : -1);
  }
  return moved;
}

static void ScoreCursorInDescendants(KDPoint p, Layout l, KDFont::Size font,
                                     LayoutCursor *result) {
  KDCoordinate currentDistance =
      p.squareDistanceTo(result->middleLeftPoint(font));
  /* Put a cursor left and right of l.
   * If l.parent is an HorizontalLayout, just put it left since the right
   * of one child is the left of another one, except if l is the last child.
   * */
  Layout parent = l.parent();
  bool checkOnlyLeft = !parent.isUninitialized() && parent.isHorizontal() &&
                       parent.indexOfChild(l) < parent.numberOfChildren() - 1;
  int numberOfDirectionsToCheck = 1 + !checkOnlyLeft;
  for (int i = 0; i < numberOfDirectionsToCheck; i++) {
    LayoutCursor tempCursor = LayoutCursor(
        l, i == 0 ? OMG::Direction::Left() : OMG::Direction::Right());
    if (currentDistance >
        p.squareDistanceTo(tempCursor.middleLeftPoint(font))) {
      *result = tempCursor;
    }
  }
  int nChildren = l.numberOfChildren();
  for (int i = 0; i < nChildren; i++) {
    ScoreCursorInDescendants(p, l.childAtIndex(i), font, result);
  }
}

static LayoutCursor ClosestCursorInDescendantsOfLayout(
    LayoutCursor currentCursor, Layout l, KDFont::Size font) {
  LayoutCursor result = LayoutCursor(l, OMG::Direction::Left());
  ScoreCursorInDescendants(currentCursor.middleLeftPoint(font), l, font,
                           &result);
  return result;
}

bool LayoutCursor::verticalMoveWithoutSelection(
    OMG::VerticalDirection direction, bool *shouldRedrawLayout) {
  /* Step 1:
   * Try to enter right or left layout if it can be entered through up/down
   * */
  if (!isSelecting()) {
    Layout nextLayout = rightLayout();
    LayoutNode::PositionInLayout positionRelativeToNextLayout =
        LayoutNode::PositionInLayout::Left;
    // Repeat for right and left
    for (int i = 0; i < 2; i++) {
      if (!nextLayout.isUninitialized()) {
        int nextIndex = nextLayout.indexAfterVerticalCursorMove(
            direction, LayoutNode::k_outsideIndex, positionRelativeToNextLayout,
            shouldRedrawLayout);
        if (nextIndex != LayoutNode::k_cantMoveIndex) {
          assert(nextIndex != LayoutNode::k_outsideIndex);
          assert(!nextLayout.isHorizontal());
          m_layout = nextLayout.childAtIndex(nextIndex);
          m_position =
              positionRelativeToNextLayout == LayoutNode::PositionInLayout::Left
                  ? leftMostPosition()
                  : rightmostPosition();
          return true;
        }
      }
      nextLayout = leftLayout();
      positionRelativeToNextLayout = LayoutNode::PositionInLayout::Right;
    }
  }

  /* Step 2:
   * Ask ancestor if cursor can move vertically. */
  Layout p = m_layout.parent();
  Layout currentChild = m_layout;
  LayoutNode::PositionInLayout currentPosition =
      m_position == leftMostPosition()
          ? LayoutNode::PositionInLayout::Left
          : (m_position == rightmostPosition()
                 ? LayoutNode::PositionInLayout::Right
                 : LayoutNode::PositionInLayout::Middle);
  while (!p.isUninitialized()) {
    int childIndex = p.indexOfChild(currentChild);
    int nextIndex = p.indexAfterVerticalCursorMove(
        direction, childIndex, currentPosition, shouldRedrawLayout);
    if (nextIndex != LayoutNode::k_cantMoveIndex) {
      if (nextIndex == LayoutNode::k_outsideIndex) {
        assert(currentPosition != LayoutNode::PositionInLayout::Middle);
        setLayout(p, currentPosition == LayoutNode::PositionInLayout::Left
                         ? OMG::Direction::Left()
                         : OMG::Direction::Right());
      } else {
        assert(!p.isHorizontal());
        // We assume the new cursor is the same whatever the font
        LayoutCursor newCursor = ClosestCursorInDescendantsOfLayout(
            *this, p.childAtIndex(nextIndex), KDFont::Size::Large);
        m_layout = newCursor.layout();
        m_position = newCursor.position();
      }
      return true;
    }
    currentChild = p;
    p = p.parent();
    currentPosition = LayoutNode::PositionInLayout::Middle;
  }
  return false;
}

void LayoutCursor::privateStartSelecting() { m_startOfSelection = m_position; }

void LayoutCursor::resetSelection() { m_startOfSelection = -1; }

bool LayoutCursor::setEmptyRectangleVisibilityAtCurrentPosition(
    EmptyRectangle::State state) {
  bool result = false;
  if (m_layout.isHorizontal()) {
    result =
        static_cast<HorizontalLayout &>(m_layout).setEmptyVisibility(state);
  }
  Layout leftL = leftLayout();
  if (!leftL.isUninitialized() &&
      leftL.type() == LayoutNode::Type::VerticalOffsetLayout &&
      static_cast<VerticalOffsetLayout &>(leftL).horizontalPosition() ==
          VerticalOffsetLayoutNode::HorizontalPosition::Prefix) {
    result =
        static_cast<VerticalOffsetLayout &>(leftL).setEmptyVisibility(state) ||
        result;
  }
  Layout rightL = rightLayout();
  if (!rightL.isUninitialized() &&
      rightL.type() == LayoutNode::Type::VerticalOffsetLayout &&
      static_cast<VerticalOffsetLayout &>(rightL).horizontalPosition() ==
          VerticalOffsetLayoutNode::HorizontalPosition::Suffix) {
    result =
        static_cast<VerticalOffsetLayout &>(rightL).setEmptyVisibility(state) ||
        result;
  }
  return result;
}

void LayoutCursor::invalidateSizesAndPositions() {
  Layout layoutToInvalidate = m_layout;
  while (!layoutToInvalidate.parent().isUninitialized()) {
    layoutToInvalidate = layoutToInvalidate.parent();
  }
  layoutToInvalidate.invalidAllSizesPositionsAndBaselines();
}

void LayoutCursor::privateDelete(LayoutNode::DeletionMethod deletionMethod,
                                 bool deletionAppliedToParent) {
  assert(!deletionAppliedToParent || !m_layout.parent().isUninitialized());

  if (deletionMethod == LayoutNode::DeletionMethod::MoveLeft) {
    bool dummy = false;
    move(OMG::Direction::Left(), false, &dummy);
    return;
  }

  if (deletionMethod == LayoutNode::DeletionMethod::DeleteParent) {
    assert(deletionAppliedToParent);
    Layout p = m_layout.parent();
    assert(!p.isUninitialized() && !p.isHorizontal());
    Layout parentOfP = p.parent();
    if (parentOfP.isUninitialized() || !parentOfP.isHorizontal()) {
      assert(m_position == 0);
      p.replaceWithInPlace(m_layout);
    } else {
      m_position = parentOfP.indexOfChild(p);
      static_cast<HorizontalLayout &>(parentOfP).removeChildAtIndexInPlace(
          m_position);
      static_cast<HorizontalLayout &>(parentOfP).addOrMergeChildAtIndex(
          m_layout, m_position);
      m_layout = parentOfP;
    }
    return;
  }

  if (deletionMethod ==
      LayoutNode::DeletionMethod::AutocompletedBracketPairMakeTemporary) {
    if (deletionAppliedToParent) {  // Inside bracket
      Layout parent = m_layout.parent();
      assert(AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
          parent.type()));
      static_cast<AutocompletedBracketPairLayoutNode *>(parent.node())
          ->setTemporary(AutocompletedBracketPairLayoutNode::Side::Left, true);
    } else {  // Right of bracket
      assert(AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
          leftLayout().type()));
      static_cast<AutocompletedBracketPairLayoutNode *>(leftLayout().node())
          ->setTemporary(AutocompletedBracketPairLayoutNode::Side::Right, true);
    }
    bool dummy = false;
    move(OMG::Direction::Left(), false, &dummy);
    return;
  }

  if (deletionMethod ==
      LayoutNode::DeletionMethod::FractionDenominatorDeletion) {
    // Merge the numerator and denominator and replace the fraction with it
    assert(deletionAppliedToParent);
    Layout fraction = m_layout.parent();
    assert(fraction.type() == LayoutNode::Type::FractionLayout &&
           fraction.childAtIndex(1) == m_layout);
    Layout numerator = fraction.childAtIndex(0);
    if (!numerator.isHorizontal()) {
      HorizontalLayout hLayout = HorizontalLayout::Builder();
      numerator.replaceWithInPlace(hLayout);
      hLayout.addOrMergeChildAtIndex(numerator, 0);
      numerator = hLayout;
    }
    assert(numerator.isHorizontal());
    m_position = numerator.numberOfChildren();
    static_cast<HorizontalLayout &>(numerator).addOrMergeChildAtIndex(
        m_layout, numerator.numberOfChildren());
    Layout parentOfFraction = fraction.parent();
    if (parentOfFraction.isUninitialized() ||
        !parentOfFraction.isHorizontal()) {
      fraction.replaceWithInPlace(numerator);
      m_layout = numerator;
    } else {
      int indexOfFraction = parentOfFraction.indexOfChild(fraction);
      m_position += indexOfFraction;
      static_cast<HorizontalLayout &>(parentOfFraction)
          .removeChildAtIndexInPlace(indexOfFraction);
      static_cast<HorizontalLayout &>(parentOfFraction)
          .addOrMergeChildAtIndex(numerator, indexOfFraction);
      m_layout = parentOfFraction;
    }
    return;
  }

  if (deletionMethod ==
          LayoutNode::DeletionMethod::TwoRowsLayoutMoveFromLowertoUpper ||
      deletionMethod == LayoutNode::DeletionMethod::GridLayoutMoveToUpperRow) {
    assert(deletionAppliedToParent);
    int newIndex = -1;
    if (deletionMethod ==
        LayoutNode::DeletionMethod::TwoRowsLayoutMoveFromLowertoUpper) {
      assert(m_layout.parent().type() ==
                 LayoutNode::Type::BinomialCoefficientLayout ||
             m_layout.parent().type() == LayoutNode::Type::Point2DLayout);
      newIndex = TwoRowsLayoutNode::k_upperLayoutIndex;
    } else {
      assert(deletionMethod ==
             LayoutNode::DeletionMethod::GridLayoutMoveToUpperRow);
      assert(GridLayoutNode::IsGridLayoutType(m_layout.parent().type()));
      GridLayoutNode *gridNode =
          static_cast<GridLayoutNode *>(m_layout.parent().node());
      int currentIndex = m_layout.parent().indexOfChild(m_layout);
      int currentRow = gridNode->rowAtChildIndex(currentIndex);
      assert(currentRow > 0 && gridNode->numberOfColumns() >= 2);
      newIndex = gridNode->indexAtRowColumn(currentRow - 1,
                                            gridNode->rightmostNonGrayColumn());
    }
    m_layout = m_layout.parent().childAtIndex(newIndex);
    m_position = rightmostPosition();
    return;
  }

  if (deletionMethod == LayoutNode::DeletionMethod::GridLayoutDeleteColumn ||
      deletionMethod == LayoutNode::DeletionMethod::GridLayoutDeleteRow ||
      deletionMethod ==
          LayoutNode::DeletionMethod::GridLayoutDeleteColumnAndRow) {
    assert(deletionAppliedToParent);
    assert(GridLayoutNode::IsGridLayoutType(m_layout.parent().type()));
    GridLayoutNode *gridNode =
        static_cast<GridLayoutNode *>(m_layout.parent().node());
    int currentIndex = m_layout.parent().indexOfChild(m_layout);
    int currentRow = gridNode->rowAtChildIndex(currentIndex);
    int currentColumn = gridNode->columnAtChildIndex(currentIndex);
    if (deletionMethod != LayoutNode::DeletionMethod::GridLayoutDeleteColumn) {
      gridNode->deleteRowAtIndex(currentRow);
    }
    if (deletionMethod != LayoutNode::DeletionMethod::GridLayoutDeleteRow) {
      gridNode->deleteColumnAtIndex(currentColumn);
    }
    int newChildIndex = gridNode->indexAtRowColumn(currentRow, currentColumn);
    *this = LayoutCursor(Layout(gridNode).childAtIndex(newChildIndex));
    didEnterCurrentPosition();
    return;
  }

  assert(deletionMethod == LayoutNode::DeletionMethod::DeleteLayout);
  if (deletionAppliedToParent) {
    setLayout(m_layout.parent(), OMG::Direction::Right());
  }
  if (!m_layout.isHorizontal()) {
    assert(m_layout.parent().isUninitialized() ||
           !m_layout.parent().isHorizontal());
    HorizontalLayout hLayout = HorizontalLayout::Builder();
    m_layout.replaceWithInPlace(hLayout);
    hLayout.addOrMergeChildAtIndex(m_layout, 0);
    m_layout = hLayout;
  }
  assert(m_position != 0);
  assert(m_layout.isHorizontal());
  static_cast<HorizontalLayout &>(m_layout).removeChildAtIndexInPlace(
      m_position - 1);
  m_position--;
}

void LayoutCursor::removeEmptyRowOrColumnOfGridParentIfNeeded() {
  if (!IsEmptyChildOfGridLayout(m_layout)) {
    return;
  }
  Layout parentGrid = m_layout.parent();
  GridLayoutNode *gridNode = static_cast<GridLayoutNode *>(parentGrid.node());
  int currentChildIndex = parentGrid.indexOfChild(m_layout);
  int newChildIndex =
      gridNode->removeTrailingEmptyRowOrColumnAtChildIndex(currentChildIndex);
  if (parentGrid.childAtIndex(newChildIndex) != m_layout) {
    assert(parentGrid.numberOfChildren() > newChildIndex);
    *this = LayoutCursor(parentGrid.childAtIndex(newChildIndex));
    didEnterCurrentPosition();
  }
}

void LayoutCursor::collapseSiblingsOfLayout(Layout l) {
  if (l.shouldCollapseSiblingsOnRight()) {
    collapseSiblingsOfLayoutOnDirection(l, OMG::Direction::Right(),
                                        l.rightCollapsingAbsorbingChildIndex());
  }
  if (l.shouldCollapseSiblingsOnLeft()) {
    collapseSiblingsOfLayoutOnDirection(l, OMG::Direction::Left(),
                                        l.leftCollapsingAbsorbingChildIndex());
  }
}

void LayoutCursor::collapseSiblingsOfLayoutOnDirection(
    Layout l, OMG::HorizontalDirection direction, int absorbingChildIndex) {
  /* This method absorbs the siblings of a layout when it's inserted.
   *
   * Example:
   * When inserting √() was just inserted in "1 + √()45 + 3 ",
   * the square root should absorb the 45 and this will output
   * "1 + √(45) + 3"
   *
   * Here l = √(), and absorbingChildIndex = 0 (the inside of the sqrt)
   * */
  Layout absorbingChild = l.childAtIndex(absorbingChildIndex);
  if (absorbingChild.isUninitialized() || !absorbingChild.isEmpty()) {
    return;
  }
  Layout p = l.parent();
  if (p.isUninitialized() || !p.isHorizontal()) {
    return;
  }
  int idxInParent = p.indexOfChild(l);
  int numberOfSiblings = p.numberOfChildren();
  int numberOfOpenParenthesis = 0;

  assert(absorbingChild.isHorizontal());  // Empty is always horizontal
  HorizontalLayout horizontalAbsorbingChild =
      static_cast<HorizontalLayout &>(absorbingChild);
  HorizontalLayout horizontalParent = static_cast<HorizontalLayout &>(p);
  Layout sibling;
  int step = direction.isRight() ? 1 : -1;
  /* Loop through the siblings and add them into l until an uncollapsable
   * layout is encountered. */
  while (true) {
    if (idxInParent == (direction.isRight() ? numberOfSiblings - 1 : 0)) {
      break;
    }
    int siblingIndex = idxInParent + step;
    sibling = horizontalParent.childAtIndex(siblingIndex);
    if (!sibling.isCollapsable(&numberOfOpenParenthesis, direction)) {
      break;
    }
    horizontalParent.removeChildAtIndexInPlace(siblingIndex);
    int newIndex = direction.isRight() ? absorbingChild.numberOfChildren() : 0;
    assert(!sibling.isHorizontal());
    horizontalAbsorbingChild.addOrMergeChildAtIndex(sibling, newIndex);
    numberOfSiblings--;
    if (direction.isLeft()) {
      idxInParent--;
    }
  }
}

void LayoutCursor::balanceAutocompletedBracketsAndKeepAValidCursor() {
  if (!m_layout.isHorizontal()) {
    return;
  }
  /* Find the top horizontal layout for balancing brackets.
   *
   * This might go again through already balanced brackets but it's safer
   * in order to ensure that all brackets are always balanced after an
   * insertion or a deletion.
   *
   * Stop if the parent of the currentLayout is not horizontal neither
   * a bracket.
   * Ex: When balancing the brackets inside the numerator of a fraction,
   * it's useless to take the parent horizontal layout of the fraction, since
   * brackets outside of the fraction won't impact the ones inside the fraction
   * */
  Layout currentLayout = m_layout;
  Layout currentParent = currentLayout.parent();
  while (!currentParent.isUninitialized() &&
         (currentParent.isHorizontal() ||
          AutocompletedBracketPairLayoutNode::IsAutoCompletedBracketPairType(
              currentParent.type()))) {
    currentLayout = currentParent;
    currentParent = currentLayout.parent();
  }
  // If the top bracket does not have an horizontal parent, create one
  if (!currentLayout.isHorizontal()) {
    assert(!currentParent.isUninitialized());
    int indexOfLayout = currentParent.indexOfChild(currentLayout);
    currentLayout = HorizontalLayout::Builder(currentLayout);
    currentParent.replaceChildAtIndexInPlace(indexOfLayout, currentLayout);
  }
  HorizontalLayout topHorizontalLayout =
      static_cast<HorizontalLayout &>(currentLayout);
  AutocompletedBracketPairLayoutNode::BalanceBrackets(
      topHorizontalLayout, static_cast<HorizontalLayout *>(&m_layout),
      &m_position);
}

}  // namespace Poincare
