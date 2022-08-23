#include "histogram_parameter_controller.h"
#include "../app.h"
#include <algorithm>
#include <assert.h>
#include <cmath>

using namespace Shared;
using namespace Escher;

namespace Statistics {

HistogramParameterController::HistogramParameterController(Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Store * store) :
  FloatParameterController<double>(parentResponder),
  m_store(store),
  m_confirmPopUpController(Invocation([](void * context, void * sender) {
    Container::activeApp()->dismissModalViewController();
    ((HistogramParameterController *)context)->stackController()->pop();
    return true;
  }, this))
{
  for (int i = 0; i < k_numberOfCells; i++) {
    m_cells[i].setParentResponder(&m_selectableTableView);
    m_cells[i].setDelegates(inputEventHandlerDelegate, this);
  }
}

void HistogramParameterController::viewWillAppear() {
  // Initialize temporary parameters to the extracted value.
  m_tempBarWidth = extractParameterAtIndex(0);
  m_tempFirstDrawnBarAbscissa = extractParameterAtIndex(1);
  FloatParameterController::viewWillAppear();
}

const char * HistogramParameterController::title() {
  return I18n::translate(I18n::Message::StatisticsGraphSettings);
}

void HistogramParameterController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  if (index == numberOfRows()-1) {
    return;
  }
  MessageTableCellWithEditableTextWithMessage * myCell = static_cast<MessageTableCellWithEditableTextWithMessage *>(cell);
  I18n::Message labels[k_numberOfCells] = {I18n::Message::RectangleWidth, I18n::Message::BarStart};
  I18n::Message sublabels[k_numberOfCells] = {I18n::Message::RectangleWidthDescription, I18n::Message::BarStartDescrition};
  myCell->setMessage(labels[index]);
  myCell->setSubLabelMessage(sublabels[index]);
  FloatParameterController::willDisplayCellForIndex(cell, index);
}

bool HistogramParameterController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Back && (extractParameterAtIndex(0) != parameterAtIndex(0) || extractParameterAtIndex(1) != parameterAtIndex(1))) {
    // Temporary values are different, open pop-up to confirm discarding values
    m_confirmPopUpController.presentModally();
    return true;
  }
  return false;
}

double HistogramParameterController::extractParameterAtIndex(int index) {
  assert(index >= 0 && index < k_numberOfCells);
  return index == 0 ? m_store->barWidth() : m_store->firstDrawnBarAbscissa();
}

double HistogramParameterController::parameterAtIndex(int index) {
  assert(index >= 0 && index < k_numberOfCells);
  return index == 0 ? m_tempBarWidth : m_tempFirstDrawnBarAbscissa;
}

bool HistogramParameterController::confirmParameterAtIndex(int parameterIndex, double value) {
  assert(parameterIndex == 0 || parameterIndex == 1);
  if (parameterIndex == 0) {
    // Set the bar width
    m_store->setBarWidth(value);
  } else {
    m_store->setFirstDrawnBarAbscissa(value);
  }
  return true;
}

bool HistogramParameterController::setParameterAtIndex(int parameterIndex, double value) {
  assert(parameterIndex == 0 || parameterIndex == 1);
  const bool setBarWidth = parameterIndex == 0;

  if (setBarWidth && value <= 0.0) {
    // The bar width cannot be negative
    assert(m_tempBarWidth > 0.0);
    Container::activeApp()->displayWarning(I18n::Message::ForbiddenValue);
    return false;
  }

  const double nextFirstDrawnBarAbscissa = setBarWidth ? m_tempFirstDrawnBarAbscissa : value;
  const double nextBarWidth = setBarWidth ? value : m_tempBarWidth;

  // The number of bars cannot be above the max
  assert(DoublePairStore::k_numberOfSeries > 0);
  for (int i = 0; i < DoublePairStore::k_numberOfSeries; i++) {
    if (!Shared::DoublePairStore::DefaultValidSeries(m_store, i)) {
      continue;
    }
    const double min = std::min(m_store->minValue(i), nextFirstDrawnBarAbscissa);
    const double max = m_store->maxValue(i);
    double numberOfBars = std::ceil((max - min)/nextBarWidth);
    // First escape case: if the bars are too thin or there is too much bars
    if (numberOfBars > HistogramRange::k_maxNumberOfBars
    // Second escape case : X-start > max
        || max < nextFirstDrawnBarAbscissa
    /* Third escape case: Since interval width is computed in float, we
     * need to check if the values are not too close.
     * If max == min then the interval goes from min to min + barWidth.
     * But if min == min + barWidth, the display is bugged.
     * */
        || (static_cast<float>(min) == static_cast<float>(max) && static_cast<float>(min + nextBarWidth) == static_cast<float>(min))) {
      // Assert the current temporary values were valid in the first place
      assert(nextBarWidth != m_tempBarWidth || nextFirstDrawnBarAbscissa != m_tempFirstDrawnBarAbscissa);
      Container::activeApp()->displayWarning(I18n::Message::ForbiddenValue);
      return false;
    }
  }

  if (setBarWidth) {
    // Set the bar width
    m_tempBarWidth = value;
  } else {
    m_tempFirstDrawnBarAbscissa = value;
  }
  return true;
}

HighlightCell * HistogramParameterController::reusableParameterCell(int index, int type) {
  assert(index >= 0 && index < k_numberOfCells);
  return &m_cells[index];
}

void HistogramParameterController::buttonAction() {
  // Update parameters values and proceed.
  if (confirmParameterAtIndex(0, m_tempBarWidth) && confirmParameterAtIndex(1, m_tempFirstDrawnBarAbscissa)) {
    FloatParameterController::buttonAction();
  }
}

}

