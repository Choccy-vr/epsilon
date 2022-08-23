#ifndef SEQUENCE_LIST_PARAM_CONTROLLER_H
#define SEQUENCE_LIST_PARAM_CONTROLLER_H

#include <escher/even_odd_expression_cell.h>
#include <escher/message_table_cell_with_chevron_and_expression.h>
#include <escher/message_table_cell_with_chevron_and_message.h>
#include <escher/message_table_cell_with_editable_text.h>
#include <escher/message_table_cell_with_switch.h>
#include <escher/memoized_list_view_data_source.h>
#include "../../shared/list_parameter_controller.h"
#include "../../shared/parameter_text_field_delegate.h"
#include "../../shared/sequence.h"
#include "../../shared/sequence_store.h"
#include "type_parameter_controller.h"

namespace Sequence {

class ListController;

class ListParameterController : public Shared::ListParameterController, public Escher::SelectableTableViewDelegate, public Shared::ParameterTextFieldDelegate {
public:
  ListParameterController(Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, ListController * list);
  const char * title() override;

  bool textFieldShouldFinishEditing(Escher::TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;
  void tableViewDidChangeSelectionAndDidScroll(Escher::SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) override;

  // MemoizedListViewDataSource
  Escher::HighlightCell * reusableCell(int index, int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  int numberOfRows() const override { return numberOfNonInheritedCells() + Shared::ListParameterController::numberOfRows(); }
  int typeAtIndex(int index) override;
private:
  constexpr static int k_typeCellType = k_numberOfSharedCells;
  constexpr static int k_initialRankCellType = k_typeCellType + 1;
  bool handleEnterOnRow(int rowIndex) override;
  bool rightEventIsEnterOnType(int type) override;
  int numberOfNonInheritedCells() const { return 1 + hasInitialRankRow(); } // number of non inherited cells
  Shared::Sequence * sequence() { return static_cast<Shared::Sequence *>(function().pointer()); }
  bool hasInitialRankRow() const;
  Escher::MessageTableCellWithChevronAndExpression m_typeCell;
  Escher::MessageTableCellWithEditableText m_initialRankCell;
  TypeParameterController m_typeParameterController;
};

}

#endif
