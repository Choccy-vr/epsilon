#ifndef PROBABILITY_GUI_INPUT_HOMOGENEITY_TABLE_CELL_H
#define PROBABILITY_GUI_INPUT_HOMOGENEITY_TABLE_CELL_H

#include "inference/statistic/chi_square_test/categorical_table_cell.h"
#include "inference/statistic/chi_square_test/homogeneity_data_source.h"
#include "inference/models/statistic/homogeneity_test.h"

namespace Inference {

class InputHomogeneityTableCell : public EditableCategoricalTableCell, public HomogeneityTableDataSource, public DynamicCellsDataSource<EvenOddEditableTextCell, k_homogeneityTableNumberOfReusableInnerCells> {
public:
  InputHomogeneityTableCell(Escher::Responder * parentResponder, DynamicSizeTableViewDataSourceDelegate * dynamicSizeTableViewDataSourceDelegate, Escher::SelectableTableViewDelegate * selectableTableViewDelegate, HomogeneityTest * test);

  // Responder
  void didBecomeFirstResponder() override;

  // EditableCategoricalTableCell
  CategoricalTableViewDataSource * tableViewDataSource() override { return this; }

  // DataSource
  int innerNumberOfRows() const override { return m_numberOfRows; }
  int innerNumberOfColumns() const override { return m_numberOfColumns; }

  // DynamicCellsDataSource
  Escher::SelectableTableView * tableView() override { return &m_selectableTableView; }

private:
  // CategoricalTableViewDataSource
  int relativeColumnIndex(int columnIndex) override { return columnIndex - 1; }

  // HomogeneityTableViewDataSource
  Escher::HighlightCell * innerCell(int i) override { return DynamicCellsDataSource<EvenOddEditableTextCell, k_homogeneityTableNumberOfReusableInnerCells>::cell(i); }
  void willDisplayInnerCellAtLocation(Escher::HighlightCell * cell, int column, int row) override;

  // DynamicCellsDataSource
  void createCells() override;
  void destroyCells() override;
};

}  // namespace Inference

#endif /* PROBABILITY_GUI_INPUT_HOMOGENEITY_TABLE_CELL_H */
