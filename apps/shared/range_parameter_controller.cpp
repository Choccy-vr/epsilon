#include "range_parameter_controller.h"
#include <assert.h>

using namespace Escher;
using namespace Poincare;

namespace Shared {

KDSize RangeParameterController::NormalizeCell::minimalSizeForOptimalDisplay() const {
  m_cell.setSize(bounds().size());
  KDSize cellSize = m_cell.minimalSizeForOptimalDisplay();
  // An additional border is required after separator (and will be overlapped)
  return KDSize(cellSize.width(), cellSize.height() + k_margin + k_lineThickness);
}

RangeParameterController::RangeParameterController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, InteractiveCurveViewRange * interactiveRange) :
  FloatParameterController<float>(parentResponder),
  m_interactiveRange(interactiveRange),
  m_tempInteractiveRange(*interactiveRange),
  m_rangeCells{},
  m_confirmPopUpController(Invocation([](void * context, void * sender) {
    Container::activeApp()->dismissModalViewController();
    ((RangeParameterController *)context)->stackController()->pop();
    return true;
  }, this)),
  m_forceHideNormalizeCell(false)
{
  for (int i = 0; i < k_numberOfTextCell; i++) {
    m_rangeCells[i].setParentResponder(&m_selectableTableView);
    m_rangeCells[i].textField()->setDelegates(inputEventHandlerDelegate, this);
  }
}

const char * RangeParameterController::title() {
  return I18n::translate(I18n::Message::Axis);
}

int RangeParameterController::numberOfRows() const {
  return k_numberOfTextCell + 1 + displayNormalizeCell();
}

int RangeParameterController::typeAtIndex(int index) {
  if (displayNormalizeCell() && index == 0) {
    return k_normalizeCellType;
  }
  return FloatParameterController::typeAtIndex(index);
}

int RangeParameterController::reusableCellCount(int type) {
  if (type == k_normalizeCellType) {
    return displayNormalizeCell();
  }
  return FloatParameterController::reusableCellCount(type);
}

HighlightCell * RangeParameterController::reusableCell(int index, int type) {
  if (type == k_normalizeCellType) {
    assert(index == 0);
    return &m_normalizeCell;
  }
  return FloatParameterController::reusableCell(index, type);
}

KDCoordinate RangeParameterController::nonMemoizedRowHeight(int j) {
  if (displayNormalizeCell()) {
    if (j == 0) {
      m_normalizeCell.setSize(KDSize(cellWidth(), 0));
      return m_normalizeCell.minimalSizeForOptimalDisplay().height();
    }
    j--;
  }
  m_forceHideNormalizeCell = true;
  KDCoordinate res = FloatParameterController::nonMemoizedRowHeight(j);
  m_forceHideNormalizeCell = false;
  return res;
}

void RangeParameterController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  if (index == numberOfRows()-1) {
    return;
  }
  if (displayNormalizeCell()) {
    if (index == 0) {
      return;
    }
    index--;
  }
  MessageTableCellWithEditableText * myCell = static_cast<MessageTableCellWithEditableText *>(cell);
  I18n::Message labels[k_numberOfTextCell] = {I18n::Message::XMin, I18n::Message::XMax, I18n::Message::YMin, I18n::Message::YMax};
  myCell->setMessage(labels[index]);
  FloatParameterController::willDisplayCellForIndex(cell, index);
}

void RangeParameterController::setRange(InteractiveCurveViewRange * range){
  m_interactiveRange = range;
  m_tempInteractiveRange = *range;
}

bool RangeParameterController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Back && m_interactiveRange->rangeChecksum() != m_tempInteractiveRange.rangeChecksum()) {
    // Open pop-up to confirm discarding values
    Container::activeApp()->displayModalViewController(&m_confirmPopUpController, 0.f, 0.f, Metric::PopUpTopMargin, Metric::PopUpRightMargin, Metric::PopUpBottomMargin, Metric::PopUpLeftMargin);
    return true;
  }
  if ((event == Ion::Events::OK || event == Ion::Events::EXE) && displayNormalizeCell() && selectedRow() == 0) {
    m_interactiveRange->normalize();
    FloatParameterController::buttonAction();
    return true;
  }
  return FloatParameterController::handleEvent(event);
}

float RangeParameterController::parameterAtIndex(int parameterIndex) {
  ParameterGetterPointer getters[k_numberOfTextCell] = {&InteractiveCurveViewRange::xMin,
    &InteractiveCurveViewRange::xMax, &InteractiveCurveViewRange::yMin, &InteractiveCurveViewRange::yMax};
  return (m_tempInteractiveRange.*getters[parameterIndex])();
}

bool RangeParameterController::setParameterAtIndex(int parameterIndex, float f) {
  parameterIndex -= displayNormalizeCell();
  ParameterSetterPointer setters[k_numberOfTextCell] = {&InteractiveCurveViewRange::setXMin,
    &InteractiveCurveViewRange::setXMax, &InteractiveCurveViewRange::setYMin, &InteractiveCurveViewRange::setYMax};
  (m_tempInteractiveRange.*setters[parameterIndex])(f);
  return true;
}

HighlightCell * RangeParameterController::reusableParameterCell(int index, int type) {
  assert(type == k_parameterCellType);
  assert(index >= 0 && index < k_numberOfTextCell);
  return m_rangeCells + index;
}

int RangeParameterController::reusableParameterCellCount(int type) {
  assert(type == k_parameterCellType);
  return k_numberOfTextCell;
}

void RangeParameterController::buttonAction() {
  /* Deselect the table before denormalizing, as it would mess up the index by
   * adding a new row. */
  m_selectableTableView.deselectTable();

  *m_interactiveRange = m_tempInteractiveRange;
  m_interactiveRange->setZoomAuto(false);
  m_interactiveRange->setZoomNormalize(m_interactiveRange->isOrthonormal());

  FloatParameterController::buttonAction();
}

}
