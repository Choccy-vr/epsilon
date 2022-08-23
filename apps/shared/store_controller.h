#ifndef SHARED_STORE_CONTROLLER_H
#define SHARED_STORE_CONTROLLER_H

#include <escher/button_row_controller.h>
#include <escher/input_view_controller.h>
#include <escher/stack_view_controller.h>
#include "editable_cell_table_view_controller.h"
#include "double_pair_store.h"
#include "layout_field_delegate.h"
#include "formula_template_menu_controller.h"
#include "input_event_handler_delegate.h"
#include "store_cell.h"
#include "store_context.h"
#include "store_parameter_controller.h"
#include "store_selectable_table_view.h"
#include "store_title_cell.h"

namespace Shared {

class StoreController : public EditableCellTableViewController, public Escher::ButtonRowDelegate, public LayoutFieldDelegate, public Shared::InputEventHandlerDelegate {
public:
  StoreController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, DoublePairStore * store, Escher::ButtonRowController * header, Poincare::Context * parentContext);
  Escher::View * view() override { return &m_dataView; }
  TELEMETRY_ID("Store");

  // EditableCellViewController
  virtual int fillColumnName(int columnIndex, char * buffer) override { return m_store->fillColumnName(m_store->seriesAtColumn(columnIndex), m_store->relativeColumnIndex(columnIndex), buffer); }

  void displayFormulaInput();
  void fillFormulaInputWithTemplate(Poincare::Layout layout);
  bool fillColumnWithFormula(Poincare::Expression formula);
  virtual void sortSelectedColumn();
  // Return false if the series can't switch hide status because it's invalid
  bool switchSelectedColumnHideStatus();

  // LayoutFieldDelegate
  bool createExpressionForFillingColumnWithFormula(const char * text);

  //TextFieldDelegate
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;

  // TableViewDataSource
  int numberOfColumns() const override;
  KDCoordinate columnWidth(int i) override;
  KDCoordinate cumulatedWidthFromIndex(int i) override;
  int indexFromCumulatedWidth(KDCoordinate offsetX) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override;
  void willDisplayCellAtLocation(Escher::HighlightCell * cell, int i, int j) override;

  // ViewController
  const char * title() override;

  // Responder
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;

  int selectedSeries() { return m_store->seriesAtColumn(selectedColumn()); }
  bool selectedSeriesIsValid() { return m_store->seriesIsValid(selectedSeries()); }

protected:
  static constexpr KDCoordinate k_cellWidth = Poincare::PrintFloat::glyphLengthForFloatWithPrecision(Poincare::Preferences::VeryLargeNumberOfSignificantDigits) * 7 + 2*Escher::Metric::SmallCellMargin + Escher::Metric::TableSeparatorThickness; // KDFont::SmallFont->glyphSize().width() = 7

  constexpr static int k_maxNumberOfDisplayableRows = (Ion::Display::Height - Escher::Metric::TitleBarHeight - Escher::Metric::TabHeight)/k_cellHeight + 2;
  constexpr static int k_maxNumberOfDisplayableColumns = Ion::Display::Width/k_cellWidth + 2;
  constexpr static int k_maxNumberOfEditableCells = k_maxNumberOfDisplayableRows * k_maxNumberOfDisplayableColumns;
  constexpr static int k_numberOfTitleCells = 4;
  static constexpr int k_titleCellType = 0;
  static constexpr int k_editableCellType = 1;

  Escher::StackViewController * stackController() const override;
  Escher::Responder * tabController() const override;
  bool setDataAtLocation(double floatBody, int columnIndex, int rowIndex) override;
  double dataAtLocation(int columnIndex, int rowIndex) override;
  void setTitleCellText(Escher::HighlightCell * titleCell, int columnIndex) override;
  void setTitleCellStyle(Escher::HighlightCell * titleCell, int columnIndex) override;
  int numberOfElementsInColumn(int columnIndex) const override;
  Escher::SelectableTableView * selectableTableView() override { return &m_dataView; }
  void reloadSeriesVisibleCells(int series, int relativeColumn = -1);

  StoreCell m_editableCells[k_maxNumberOfEditableCells];
  DoublePairStore * m_store;

  virtual Escher::InputViewController * inputViewController() = 0;

private:
  bool cellAtLocationIsEditable(int columnIndex, int rowIndex) override;
  int maxNumberOfElements() const override { return DoublePairStore::k_maxNumberOfPairs; }

  StoreTitleCell m_titleCells[k_numberOfTitleCells];
  StoreSelectableTableView m_dataView;
  FormulaTemplateMenuController m_templateController;
  Escher::StackViewController m_templateStackController;
  StoreContext m_storeContext;

};

}

#endif
