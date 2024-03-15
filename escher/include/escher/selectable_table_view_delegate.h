#ifndef ESCHER_SELECTABLE_TABLE_VIEW_DELEGATE_H
#define ESCHER_SELECTABLE_TABLE_VIEW_DELEGATE_H

#include <escher/context_provider.h>

namespace Escher {

class SelectableTableView;

class SelectableTableViewDelegate : public ContextProvider {
 public:
  /* withinTemporarySelection flag indicates when the selection change happens
   * in a temporary deselection: indeed, when reloading the data of the table,
   * we deselect the table before re-layouting the entire table and re-select
   * the previous selected cell. We might implement different course of action
   * when the selection change is 'real' or within temporary selection. */
  virtual void tableViewDidChangeSelectionAndDidScroll(
      SelectableTableView* t, int previousSelectedCol, int previousSelectedRow,
      KDPoint previousOffset, bool withinTemporarySelection = false) {}
  virtual KDPoint offsetToRestoreAfterReload(
      const SelectableTableView* t) const;
  virtual int numberOfRowsAtColumn(const SelectableTableView* t, int column);
};

}  // namespace Escher
#endif
