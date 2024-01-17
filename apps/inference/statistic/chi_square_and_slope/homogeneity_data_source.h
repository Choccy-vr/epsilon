#ifndef INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_HOMOGENEITY_DATA_SOURCE_H
#define INFERENCE_STATISTIC_CHI_SQUARE_AND_SLOPE_HOMOGENEITY_DATA_SOURCE_H

#include <apps/i18n.h>
#include <escher/even_odd_buffer_text_cell.h>
#include <escher/solid_color_cell.h>

#include <algorithm>

#include "inference/models/statistic/homogeneity_test.h"
#include "inference/shared/dynamic_cells_data_source.h"
#include "inference/statistic/chi_square_and_slope/categorical_table_view_data_source.h"

namespace Inference {

/* This class wraps a TableViewDataSource by adding a Row & Column header around
 * it. Specifically meant for InputHomogeneity and HomogeneityResults. */
class HomogeneityTableDataSource
    : public CategoricalTableViewDataSource,
      public DynamicCellsDataSource<
          InferenceEvenOddBufferCell,
          k_homogeneityTableNumberOfReusableHeaderCells>,
      public DynamicCellsDataSourceDelegate<InferenceEvenOddBufferCell> {
 public:
  HomogeneityTableDataSource();

  // TableViewDataSource
  int numberOfRows() const override { return innerNumberOfRows() + 1; }
  int numberOfColumns() const override { return innerNumberOfColumns() + 1; }
  int reusableCellCount(int type) override;
  int typeAtLocation(int column, int row) const override;
  Escher::HighlightCell* reusableCell(int i, int type) override;
  void fillCellForLocation(Escher::HighlightCell* cell, int column,
                           int row) override;
  bool canSelectCellAtLocation(int column, int row) override {
    return typeAtLocation(column, row) != k_typeOfTopLeftCell;
  }

  // DynamicCellsDataSource
  void initCell(InferenceEvenOddBufferCell, void* cell, int index) override;

  constexpr static int k_columnWidth = 82;
  // 5 - we take into account the potential "Total" column
  constexpr static int k_numberOfReusableColumns =
      std::min(Ion::Display::Width / k_columnWidth + 2,
               HomogeneityTest::k_maxNumberOfColumns + 1);
  // std::min(12, 9 + 1) - we take into account the potential "Total" row
  constexpr static int k_maxNumberOfReusableRows =
      std::min(CategoricalTableViewDataSource::k_maxNumberOfReusableRows,
               HomogeneityTest::k_maxNumberOfRows + 1);
  // 5 * 10
  constexpr static int k_numberOfReusableCells =
      k_maxNumberOfReusableRows * k_numberOfReusableColumns;

 protected:
  constexpr static int k_maxNumberOfColumns =
      HomogeneityTest::k_maxNumberOfColumns;
  constexpr static int k_maxNumberOfRows = HomogeneityTest::k_maxNumberOfRows;

  KDCoordinate nonMemoizedColumnWidth(int column) override {
    return k_columnWidth;
  }
  virtual int innerNumberOfRows() const = 0;
  virtual int innerNumberOfColumns() const = 0;
  virtual void fillInnerCellForLocation(Escher::HighlightCell* cell, int column,
                                        int row) = 0;
  virtual Escher::HighlightCell* innerCell(int i) = 0;

 private:
  constexpr static int k_typeOfTopLeftCell = k_typeOfHeaderCells + 1;
  constexpr static int k_headerTranslationBuffer = 20;

  I18n::Message m_headerPrefix;
  Escher::SolidColorCell m_topLeftCell;
};

}  // namespace Inference

#endif
