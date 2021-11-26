#ifndef APPS_PROBABILITY_GUI_SELECTABLE_CELL_LIST_CONTROLLER_H
#define APPS_PROBABILITY_GUI_SELECTABLE_CELL_LIST_CONTROLLER_H

#include <escher/highlight_cell.h>
#include <escher/responder.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/stack_view_controller.h>

#include "page_controller.h"

namespace Probability {
//TODO get rid ?
/*
 * This Class is useful to create a SelectableListViewController of
 * the same type of cells.
 * Warning: by design it's the opposite of the memoization implemented in MemoizedListViewDataSource (every cell is
 * stored).
 */
template <typename Cell, int n>
class SelectableCellListPage : public SelectableListViewPage {
 public:
  SelectableCellListPage(Escher::StackViewController * parent) : SelectableListViewPage(parent) {}
  int numberOfRows() const override { return k_numberOfRows; }
  Escher::HighlightCell * reusableCell(int i, int type) override {
    assert(type == 0);
    assert(i >= 0 && i < k_numberOfRows);
    return &m_cells[i];
  }
  Cell * cellAtIndex(int i) { return &m_cells[i]; }  // TODO useless?

 protected:
  constexpr static int k_numberOfRows = n;
  Cell m_cells[n];
};

}  // namespace Probability

#endif /* APPS_PROBABILITY_GUI_SELECTABLE_CELL_LIST_CONTROLLER_H */
