#include "single_range_controller.h"

using namespace Escher;
using namespace Poincare;

namespace Shared {

// SingleRangeController

SingleRangeController::SingleRangeController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, InteractiveCurveViewRange * interactiveRange) :
  SimpleFloatParameterController<float>(parentResponder),
  m_autoCell(I18n::Message::DefaultSetting),
  m_boundsCells{},
  m_range(interactiveRange)
{
  for (int i = 0; i < k_numberOfTextCells; i++) {
    m_boundsCells[i].setController(this);
    m_boundsCells[i].setParentResponder(&m_selectableTableView);
    m_boundsCells[i].textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

void SingleRangeController::willDisplayCellForIndex(Escher::HighlightCell * cell, int index) {
  /* We take advantage of this window not being supposed to scroll to
   * attribute the same type to all cells, which in turns avoid the need to
   * reimplement methods such as nonMemoizedRowHeight, reusableCellCount,
   * typeAtIndex... */
  if (index == 0) {
    SwitchView * switchView = static_cast<SwitchView *>(const_cast<View *>(m_autoCell.accessoryView()));
    switchView->setState(autoStatus());
    return;
  }
  if (index < k_numberOfTextCells + 1) {
    LockableEditableCell * castedCell = static_cast<LockableEditableCell *>(cell);
    castedCell->setMessage(index == 1 ? I18n::Message::Minimum : I18n::Message::Maximum);
    KDColor color = autoStatus() ? Palette::GrayDark : KDColorBlack;
    castedCell->setTextColor(color);
    castedCell->textField()->setTextColor(color);
  }
  SimpleFloatParameterController<float>::willDisplayCellForIndex(cell, index);
}

bool SingleRangeController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Left) {
    stackController()->pop();
    return true;
  }
  if (selectedRow() == 0 && (event == Ion::Events::OK || event == Ion::Events::EXE)) {
    if (m_editXRange) {
      m_range->setXAuto(!m_range->xAuto());
    } else {
      m_range->setYAuto(!m_range->yAuto());
    }
    m_range->computeRanges();
    resetMemoization();
    m_selectableTableView.reloadData();
    return true;
  }
  return SimpleFloatParameterController<float>::handleEvent(event);
}

float SingleRangeController::parameterAtIndex(int index) {
  assert(index >= 1 && index < k_numberOfTextCells + 1);
  index--;
  ParameterGetterPointer getters[] = { &InteractiveCurveViewRange::yMin, &InteractiveCurveViewRange::yMax, &InteractiveCurveViewRange::xMin, &InteractiveCurveViewRange::xMax };
  return (m_range->*getters[index + 2 * m_editXRange])();
}

HighlightCell * SingleRangeController::reusableParameterCell(int index, int type) {
  if (index == 0) {
    return &m_autoCell;
  }
  return &m_boundsCells[index - 1];
}

bool SingleRangeController::setParameterAtIndex(int parameterIndex, float f) {
  assert(parameterIndex >= 1 && parameterIndex < k_numberOfTextCells + 1);
  parameterIndex--;
  ParameterSetterPointer setters[] = { &InteractiveCurveViewRange::setYMin, &InteractiveCurveViewRange::setYMax, &InteractiveCurveViewRange::setXMin, &InteractiveCurveViewRange::setXMax };
  (m_range->*setters[parameterIndex + 2 * m_editXRange])(f);
  return true;
}

// SingleRangeController::LockableEditableCell

Responder * SingleRangeController::LockableEditableCell::responder() {
  return m_controller->autoStatus() ? nullptr : this;
}

}
