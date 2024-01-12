#ifndef HOME_CONTROLLER_H
#define HOME_CONTROLLER_H

#include <escher/regular_table_view_data_source.h>
#include <escher/selectable_table_view.h>
#include <escher/selectable_table_view_data_source.h>
#include <escher/selectable_table_view_delegate.h>
#include <escher/view_controller.h>

#include "app_cell.h"

namespace Home {

class Controller : public Escher::ViewController,
                   public Escher::SimpleTableViewDataSource,
                   public Escher::SelectableTableViewDelegate {
 public:
  Controller(Escher::Responder* parentResponder,
             Escher::SelectableTableViewDataSource* selectionDataSource);

  Escher::View* view() override { return &m_view; }

  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
  TELEMETRY_ID("");

  int numberOfRows() const override {
    return ((numberOfIcons() - 1) / k_numberOfColumns) + 1;
  }
  int numberOfColumns() const override { return k_numberOfColumns; }
  Escher::HighlightCell* reusableCell(int index) override;
  int reusableCellCount() const override { return k_numberOfReusableCells; }
  void fillCellForLocation(Escher::HighlightCell* cell, int column,
                           int row) override;
  bool canSelectCellAtLocation(int column, int row) override {
    assert(column >= 0 && column < numberOfColumns() && row >= 0 &&
           row < numberOfRows());
    return row < numberOfRows() - 1 || column <= columnOfLastIcon();
  }
  void tableViewDidChangeSelectionAndDidScroll(
      Escher::SelectableTableView* t, int previousSelectedCol,
      int previousSelectedRow, KDPoint previousOffset,
      bool withinTemporarySelection) override;

 private:
  // SimpleTableViewDataSource
  KDCoordinate defaultRowHeight() override { return k_cellHeight; }
  KDCoordinate defaultColumnWidth() override { return k_cellWidth; }

  int numberOfIcons() const;
  Escher::SelectableTableViewDataSource* selectionDataSource() const;
  void switchToSelectedApp();
  bool appIsForbidden(I18n::Message appName) const;
  I18n::Message forbiddenAppMessage() const;

  // Conversion index <--> column/row
  int columnOfIconAtIndex(int iconIndex) const {
    return iconIndex % k_numberOfColumns;
  }
  int rowOfIconAtIndex(int iconIndex) const {
    return iconIndex / k_numberOfColumns;
  }
  int indexOfIconAtColumnAndRow(int column, int row) const {
    return row * k_numberOfColumns + column;
  }
  int indexOfAppAtColumnAndRow(int column, int row) const {
    return indexOfIconAtColumnAndRow(column, row) + 1;
  }
  int columnOfLastIcon() { return columnOfIconAtIndex(numberOfIcons() - 1); }

  class ContentView : public Escher::View {
   public:
    ContentView(Controller* controller,
                Escher::SelectableTableViewDataSource* selectionDataSource);
    Escher::SelectableTableView* selectableTableView();
    void drawRect(KDContext* ctx, KDRect rect) const override {
      ctx->fillRect(bounds(), KDColorWhite);
    }
    void reload() { markWholeFrameAsDirty(); }
    void reloadBottomRow(SimpleTableViewDataSource* dataSource,
                         int lastIconColumn);

   private:
    int numberOfSubviews() const override { return 1; }
    View* subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    Escher::SelectableTableView m_selectableTableView;
  };
  constexpr static KDMargins k_margins = {4, 4, 0, 0};
  constexpr static KDCoordinate k_bottomMargin = 14;
  constexpr static KDCoordinate k_indicatorMargin = 61;
  constexpr static int k_numberOfColumns = 3;
  constexpr static int k_numberOfReusableCells = 6;
  constexpr static int k_cellHeight = 104;
  constexpr static int k_cellWidth = 104;
  ContentView m_view;
  AppCell m_reusableCells[k_numberOfReusableCells];
};

}  // namespace Home

#endif
