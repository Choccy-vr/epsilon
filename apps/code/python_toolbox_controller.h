#ifndef CODE_PYTHON_TOOLBOX_CONTROLLER_H
#define CODE_PYTHON_TOOLBOX_CONTROLLER_H

#include <apps/i18n.h>
#include <escher/container.h>
#include <escher/menu_cell.h>
#include <escher/toolbox.h>
#include <ion/events.h>
#include <kandinsky/font.h>

namespace Code {

using ToolboxLeafCell =
    Escher::MenuCell<Escher::MessageTextView, Escher::MessageTextView>;

class PythonToolboxController : public Escher::Toolbox {
 public:
  // PythonToolboxController
  PythonToolboxController();
  const Escher::ToolboxMessageTree* moduleChildren(const char* name,
                                                   int* numberOfNodes) const;

  // Toolbox
  bool handleEvent(Ion::Events::Event event) override;

  // MemoizedListViewDataSource
  KDCoordinate nonMemoizedRowHeight(int row) override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;

 protected:
  bool selectLeaf(int selectedRow) override;
  const Escher::ToolboxMessageTree* rootModel() const override;
  ToolboxLeafCell* leafCellAtIndex(int index) override;
  Escher::NestedMenuController::NodeCell* nodeCellAtIndex(int index) override;
  int maxNumberOfDisplayedRows() const override;
  constexpr static int k_maxNumberOfDisplayedRows =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          Escher::AbstractMenuCell::k_minimalLargeFontCellHeight,
          Escher::Metric::PopUpMargins.top() +
              Escher::Metric::StackTitleHeight);

 private:
  void scrollToLetter(char letter);
  void scrollToAndSelectChild(int i);
  ToolboxLeafCell m_leafCells[k_maxNumberOfDisplayedRows];
  Escher::NestedMenuController::NodeCell
      m_nodeCells[k_maxNumberOfDisplayedRows];
};

}  // namespace Code

#endif
