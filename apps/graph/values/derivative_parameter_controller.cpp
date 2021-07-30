#include "derivative_parameter_controller.h"
#include "values_controller.h"
#include "../app.h"
#include <assert.h>

using namespace Escher;

namespace Graph {

DerivativeParameterController::DerivativeParameterController(ValuesController * valuesController) :
  SelectableListViewController(valuesController),
  m_hideColumn(I18n::Message::HideDerivativeColumn),
#if COPY_COLUMN
  m_copyColumn(I18n::Message::CopyColumnInList),
#endif
  m_record(),
  m_valuesController(valuesController)
{
}

void DerivativeParameterController::viewWillAppear() {
  functionStore()->modelForRecord(m_record)->derivativeNameWithArgument(m_pageTitle, k_maxNumberOfCharsInTitle);
}

const char * DerivativeParameterController::title() {
  return m_pageTitle;
}

void DerivativeParameterController::didBecomeFirstResponder() {
  selectCellAtLocation(0, 0);
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool DerivativeParameterController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    switch (selectedRow()) {
      case 0:
      {
        m_valuesController->selectCellAtLocation(m_valuesController->selectedColumn()-1, m_valuesController->selectedRow());
        functionStore()->modelForRecord(m_record)->setDisplayDerivative(false);
        StackViewController * stack = (StackViewController *)(parentResponder());
        stack->pop();
        return true;
      }
#if COPY_COLUMN
    case 1:
    /* TODO: implement function copy column */
      return true;
#endif
      default:
        assert(false);
        return false;
    }
  }
  return false;
}

int DerivativeParameterController::numberOfRows() const {
  return k_totalNumberOfCell;
};

HighlightCell * DerivativeParameterController::reusableCell(int index, int type) {
  assert(index >= 0);
  assert(index < k_totalNumberOfCell);
#if COPY_COLUMN
  HighlightCell * cells[] = {&m_hideColumn, &m_copyColumn};
#else
  HighlightCell * cells[] = {&m_hideColumn};
#endif
  return cells[index];
}

ContinuousFunctionStore * DerivativeParameterController::functionStore() {
  return App::app()->functionStore();
}

}
