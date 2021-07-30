#ifndef SHARED_LIST_PARAM_CONTROLLER_H
#define SHARED_LIST_PARAM_CONTROLLER_H

#include <apps/i18n.h>
#include <escher/selectable_list_view_controller.h>
#include <escher/message_table_cell_with_switch.h>
#include <escher/selectable_table_view_delegate.h>
#include "function_store.h"

namespace Shared {

class ListParameterController : public Escher::SelectableListViewController {
public:
  ListParameterController(Responder * parentResponder, I18n::Message functionColorMessage, I18n::Message deleteFunctionMessage, Escher::SelectableTableViewDelegate * tableDelegate = nullptr);

  const char * title() override;
  bool handleEvent(Ion::Events::Event event) override;
  TELEMETRY_ID("ListParameter");
  void setRecord(Ion::Storage::Record record);
  void didBecomeFirstResponder() override;
  void viewWillAppear() override;

  // MemoizedListViewDataSource
  int numberOfRows() const override { return k_numberOfSharedCells; }
  Escher::HighlightCell * reusableCell(int index, int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
protected:
  virtual bool handleEnterOnRow(int rowIndex);
  FunctionStore * functionStore();
  ExpiringPointer<Function> function();
  Ion::Storage::Record m_record;
private:
  // Return index of shared cell from row number
  int sharedCellIndex(int j);
  constexpr static int k_numberOfSharedCells = 2;
  Escher::MessageTableCellWithSwitch m_enableCell;
  Escher::MessageTableCell m_deleteCell;
};

}

#endif
