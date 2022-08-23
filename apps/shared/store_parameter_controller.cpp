#include "store_parameter_controller.h"
#include <assert.h>
#include <escher/message_table_cell_with_editable_text.h>
#include <escher/stack_view_controller.h>
#include <escher/container.h>
#include "store_controller.h"

using namespace Escher;

namespace Shared {

StoreParameterController::StoreParameterController(Responder * parentResponder, StoreController * storeController) :
  ColumnParameterController(parentResponder),
  m_storeController(storeController),
  m_clearColumn(),
  m_sortCell(I18n::Message::SortCellLabel),
  m_fillFormula(I18n::Message::FillWithFormula)
{
  m_clearColumn.setMessageWithPlaceholder(I18n::Message::ClearColumn);
}

void StoreParameterController::initializeColumnParameters() {
  ColumnParameterController::initializeColumnParameters();
  m_sortCell.setSubLabelMessage(sortMessage());
}

bool StoreParameterController::handleEvent(Ion::Events::Event event) {
  if (event != Ion::Events::OK && event != Ion::Events::EXE) {
    return false;
  }
  switch (selectedRow()) {
    case k_indexOfClearColumn:
    {
      stackView()->pop();
      m_storeController->presentClearSelectedColumnPopupIfClearable();
      break;
    }
    case k_indexOfFillFormula:
    {
      m_storeController->displayFormulaInput();
      stackView()->pop();
      break;
    }
    case k_indexOfSortCell:
    {
      m_storeController->sortSelectedColumn();
      stackView()->pop();
      break;
    }
    default :
      assert(false);
      return false;
  }
  return true;
}

HighlightCell * StoreParameterController::reusableCell(int index) {
  assert(index >= 0 && index < numberOfCells());
  HighlightCell * cells[] = {&m_sortCell, &m_fillFormula, &m_clearColumn};
  return cells[index];
}

EditableCellTableViewController * StoreParameterController::editableCellTableViewController() {
  return m_storeController;
}

}
