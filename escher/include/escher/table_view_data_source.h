#ifndef ESCHER_TABLE_VIEW_DATA_SOURCE_H
#define ESCHER_TABLE_VIEW_DATA_SOURCE_H

#include <escher/highlight_cell.h>
#include <escher/table_size_1D_manager.h>
#include <escher/view.h>

namespace Escher {

class TableView;

class TableViewDataSource {
  template <int N>
  friend class MemoizedRowHeightManager;
  template <int N>
  friend class MemoizedColumnWidthManager;
  friend class TableView;
  friend class ListWithTopAndBottomController;

 public:
  // TODO: Factorize ovelap and separator
  constexpr static KDCoordinate k_defaultRowSeparator =
      Escher::Metric::CommonMenuMargin + Escher::Metric::CellSeparatorThickness;

  virtual int numberOfRows() const = 0;
  virtual int numberOfColumns() const = 0;
  virtual void fillCellForLocation(HighlightCell* cell, int column, int row);
  virtual bool canSelectCellAtLocation(int column, int row) { return true; }
  virtual bool canStoreCellAtLocation(int column, int row) { return true; }

  KDCoordinate columnWidth(int column, bool withSeparator = true);
  KDCoordinate rowHeight(int row, bool withSeparator = true);

  KDCoordinate cumulatedWidthBeforeColumn(int column);
  KDCoordinate cumulatedHeightBeforeRow(int row);

  int columnAfterCumulatedWidth(KDCoordinate offsetX);
  int rowAfterCumulatedHeight(KDCoordinate offsetY);

  virtual KDCoordinate separatorBeforeColumn(int column) { return 0; }
  virtual KDCoordinate separatorBeforeRow(int row) { return 0; }

  virtual HighlightCell* reusableCell(int index, int type) = 0;
  virtual int reusableCellCount(int type) = 0;
  virtual int typeAtLocation(int column, int row) const = 0;

  void lockSizeMemoization(bool state);

 protected:
  /* These should always be overriden unless sizes are regular, in which case
   * only defaultColumnWidth and defaultRowHeight need to be overriden.
   *
   * These two methods return width and height without separator. */
  virtual KDCoordinate nonMemoizedColumnWidth(int column) {
    return defaultColumnWidth();
  }
  virtual KDCoordinate nonMemoizedRowHeight(int row) {
    return defaultRowHeight();
  }

  KDCoordinate nonMemoizedColumnWidth(int column, bool withSeparator);
  KDCoordinate nonMemoizedRowHeight(int row, bool withSeparator);

  // These should be overriden if a RegularTableSize1DManager is used.
  virtual KDCoordinate defaultColumnWidth() {
    return TableSize1DManager::k_undefinedSize;
  }
  virtual KDCoordinate defaultRowHeight() {
    return TableSize1DManager::k_undefinedSize;
  }

  /* TODO: These are virtual only for PrefacedTableView.
   * We might find a way to avoid having this in the vtable ? */
  virtual KDCoordinate nonMemoizedCumulatedWidthBeforeColumn(int column);
  virtual KDCoordinate nonMemoizedCumulatedHeightBeforeRow(int row);
  virtual int nonMemoizedColumnAfterCumulatedWidth(KDCoordinate offsetX);
  virtual int nonMemoizedRowAfterCumulatedHeight(KDCoordinate offsetY);

  /* These handle the potential memoization of sizes, the computation of
   * cumulatedXBeforeIndex, and the computation of indexAfterCumulatedX.
   * They can be left to nullptr, in which case these previous methods will
   * fallback on default implementation. But if the table is too large, it
   * might become laggy when scrolling down.
   * To optimize:
   * Use a RegularTableSize1DManager if the size is always the same.
   * Use a MemoizedTableSize1DManager if the sizes are variable. */
  virtual TableSize1DManager* columnWidthManager() { return nullptr; }
  virtual TableSize1DManager* rowHeightManager() { return nullptr; }
  virtual void resetSizeMemoization();
};

}  // namespace Escher
#endif
