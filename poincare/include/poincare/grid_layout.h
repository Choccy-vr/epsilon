#ifndef POINCARE_GRID_LAYOUT_NODE_H
#define POINCARE_GRID_LAYOUT_NODE_H

#include <poincare/array.h>
#include <poincare/empty_rectangle.h>
#include <poincare/layout.h>
#include <poincare/layout_cursor.h>

namespace Poincare {

class GridLayout;
class MatrixLayoutNode;

class GridLayoutNode : public Array, public LayoutNode {
  friend class BinomialCoefficientLayoutNode;
  friend class GridLayout;

 public:
  constexpr static KDCoordinate k_gridEntryMargin = 6;

  static bool IsGridLayoutType(Type type) {
    return type == Type::MatrixLayout || type == Type::PiecewiseOperatorLayout;
  }

  GridLayoutNode() : Array(), LayoutNode() {}

  // Grid layout
  KDSize gridSize(KDFont::Size font) const {
    return KDSize(gridWidth(font), gridHeight(font));
  }

  void willFillEmptyChildAtIndex(int childIndex);
  int removeTrailingEmptyRowOrColumnAtChildIndex(int childIndex);
  virtual void startEditing() = 0;
  virtual void stopEditing() = 0;

  // LayoutNode
  int indexAfterHorizontalCursorMove(OMG::HorizontalDirection direction,
                                     int currentIndex,
                                     bool *shouldRedrawLayout) override;
  int indexAfterVerticalCursorMove(OMG::VerticalDirection direction,
                                   int currentIndex,
                                   PositionInLayout positionAtCurrentIndex,
                                   bool *shouldRedrawLayout) override;
  DeletionMethod deletionMethodForCursorLeftOfChild(
      int childIndex) const override;

  // SerializableNode
  size_t serialize(char *buffer, size_t bufferSize,
                   Preferences::PrintFloatMode floatDisplayMode,
                   int numberOfSignificantDigits) const override {
    assert(false);
    return 0;
  }

  // TreeNode
  size_t size() const override { return sizeof(GridLayoutNode); }
  void didChangeArity(int newNumberOfChildren) override {
    return Array::didChangeNumberOfChildren(newNumberOfChildren);
  }
  int numberOfChildren() const override {
    return m_numberOfRows * m_numberOfColumns;
  }
  void setNumberOfChildren(int numberOfChildren) override {
    if (numberOfChildren == 0) {  // Used to erase children
      m_numberOfRows = 0;
      m_numberOfColumns = 0;
    }
  }
#if POINCARE_TREE_LOG
  void logNodeName(std::ostream &stream) const override {
    stream << "GridLayout";
  }
#endif

  int rowAtChildIndex(int index) const;
  int columnAtChildIndex(int index) const;
  int indexAtRowColumn(int row, int column) const;
  int rightmostNonGrayColumn() const {
    return numberOfColumns() - 1 - (isEditing() && !numberOfColumnsIsFixed());
  }
  int closestNonGrayIndex(int index) const;

  void deleteColumnAtIndex(int index);
  void deleteRowAtIndex(int index);

 protected:
  constexpr static int k_minimalNumberOfRowsAndColumnsWhileEditing = 2;
  // Row and columns
  virtual bool numberOfRowsIsFixed() const { return false; }
  virtual bool numberOfColumnsIsFixed() const { return false; }
  virtual bool isEditing() const = 0;
  bool isColumnEmpty(int index) const {
    return isColumnOrRowEmpty(true, index);
  }
  bool isRowEmpty(int index) const { return isColumnOrRowEmpty(false, index); }
  void addEmptyColumn(EmptyRectangle::Color color) {
    assert(!numberOfColumnsIsFixed());
    return addEmptyRowOrColumn(true, color);
  }
  void addEmptyRow(EmptyRectangle::Color color) {
    assert(!numberOfRowsIsFixed());
    return addEmptyRowOrColumn(false, color);
  }
  bool childIsRightOfGrid(int index) const;
  bool childIsLeftOfGrid(int index) const;
  bool childIsTopOfGrid(int index) const;
  bool childIsBottomOfGrid(int index) const;
  bool childIsInLastNonGrayColumn(int index) const;
  bool childIsInLastNonGrayRow(int index) const;

  // Sizes
  virtual KDCoordinate horizontalGridEntryMargin(KDFont::Size font) const {
    return k_gridEntryMargin;
  }
  KDCoordinate verticalGridEntryMargin(KDFont::Size font) const {
    return k_gridEntryMargin;
  }
  KDCoordinate rowBaseline(int row, KDFont::Size font);
  KDCoordinate rowHeight(int row, KDFont::Size font) const;
  KDCoordinate gridHeight(KDFont::Size font) const;
  KDCoordinate columnWidth(int column, KDFont::Size font) const;
  KDCoordinate gridWidth(KDFont::Size font) const;

  // LayoutNode
  KDSize computeSize(KDFont::Size font) override;
  KDCoordinate computeBaseline(KDFont::Size font) override;
  KDPoint positionOfChild(LayoutNode *l, KDFont::Size font) override;

 private:
  // GridLayoutNode

  bool isColumnOrRowEmpty(bool column, int index) const;
  void addEmptyRowOrColumn(bool column, EmptyRectangle::Color color);
  void colorGrayEmptyLayoutsInYellowInColumnOrRow(bool column, int lineIndex);
};

class GridLayout : public Layout {
 public:
  GridLayout(const GridLayoutNode *n) : Layout(n) {}

  void setDimensions(int rows, int columns);
  using Layout::addChildAtIndexInPlace;
  int numberOfRows() const { return node()->numberOfRows(); }
  int numberOfColumns() const { return node()->numberOfColumns(); }
  bool isEditing() const { return node()->isEditing(); }
  void startEditing() { node()->startEditing(); }
  void stopEditing() { node()->stopEditing(); }

 private:
  virtual GridLayoutNode *node() const {
    return static_cast<GridLayoutNode *>(Layout::node());
  }
  void setNumberOfRows(int rows) { node()->setNumberOfRows(rows); }
  void setNumberOfColumns(int columns) { node()->setNumberOfColumns(columns); }
};

}  // namespace Poincare

#endif
