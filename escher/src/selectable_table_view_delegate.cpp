#include <escher/selectable_table_view.h>
#include <escher/selectable_table_view_delegate.h>

namespace Escher {

KDPoint SelectableTableViewDelegate::offsetToRestoreAfterReload(
    const SelectableTableView* t) const {
  return t->contentOffset();
}

int SelectableTableViewDelegate::numberOfRowsAtColumn(
    const SelectableTableView* t, int column) {
  return t->totalNumberOfRows();
}

}  // namespace Escher