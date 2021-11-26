#include "type_parameter_controller.h"
#include "type_helper.h"
#include <apps/i18n.h>
#include "../app.h"
#include <assert.h>

using namespace Escher;

namespace Graph {

TypeParameterController::TypeParameterController(Responder * parentResponder) :
  SelectableListViewController(parentResponder),
  m_record()
{
}

void TypeParameterController::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool TypeParameterController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    assert(!m_record.isNull());
    // TODO Hugo : Remove this menu all together
    // NewFunction::PlotType plotType = static_cast<NewFunction::PlotType>(selectedRow());
    // App * myApp = App::app();
    // assert(!m_record.isNull());
    // Shared::ExpiringPointer<NewFunction> function = myApp->functionStore()->modelForRecord(m_record);
    // function->setPlotType(plotType, Poincare::Preferences::sharedPreferences()->angleUnit(), myApp->localContext());
    // if (function->plotType() != plotType) {
    //   /* Updating plot type failed due to full storage. Do not quit menu as
    //    * there is a "full storage" warning pop-up as first responder. */
    //   return true;
    // }
    StackViewController * stack = stackController();
    stack->pop();
    stack->pop();
    return true;
  }
  if (event == Ion::Events::Left && !m_record.isNull()) {
    stackController()->pop();
    return true;
  }
  return false;
}

const char * TypeParameterController::title() {
  return I18n::translate(I18n::Message::CurveType);
}

void TypeParameterController::viewWillAppear() {
  ViewController::viewWillAppear();
  App * myApp = App::app();
  assert(!m_record.isNull());
  Shared::ExpiringPointer<NewFunction> function = myApp->functionStore()->modelForRecord(m_record);
  int row = static_cast<int>(function->plotType());
  selectCellAtLocation(0, row);
  resetMemoization();
  m_selectableTableView.reloadData();
}

KDCoordinate TypeParameterController::nonMemoizedRowHeight(int j) {
  MessageTableCellWithExpression tempCell;
  return heightForCellAtIndex(&tempCell, j);
}

void TypeParameterController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  assert(0 <= index && index < k_numberOfTypes);
  MessageTableCellWithExpression * myCell = static_cast<MessageTableCellWithExpression *>(cell);
  myCell->setMessage(PlotTypeHelper::Message(index));
  myCell->setLayout(PlotTypeHelper::Layout(index));
}

MessageTableCellWithExpression * TypeParameterController::reusableCell(int index, int type) {
  assert(0 <= index && index < reusableCellCount(type));
  return &m_cells[index];
}

StackViewController * TypeParameterController::stackController() const {
  return static_cast<StackViewController *>(parentResponder());
}

}
