#include "calculation_controller.h"

#include <apps/shared/poincare_helpers.h>
#include <assert.h>
#include <poincare/preferences.h>
#include <poincare/print.h>

#include <algorithm>
#include <cmath>

#include "../images/calculation1_icon.h"
#include "probability/app.h"
#include "probability/constants.h"
#include "probability/models/calculation/discrete_calculation.h"
#include "probability/models/calculation/finite_integral_calculation.h"
#include "probability/models/calculation/left_integral_calculation.h"
#include "probability/models/calculation/right_integral_calculation.h"
#include "probability/models/data.h"

using namespace Poincare;
using namespace Shared;
using namespace Escher;

namespace Probability {

constexpr int CalculationController::k_titleBufferSize;

CalculationController::ContentView::ContentView(SelectableTableView * selectableTableView,
                                                Distribution * distribution,
                                                Calculation * calculation) :
      m_selectableTableView(selectableTableView),
      m_distributionCurveView(distribution, calculation) {
}

View * CalculationController::ContentView::subviewAtIndex(int index) {
  assert(index >= 0 && index < 2);
  if (index == 0) {
    return m_selectableTableView;
  }
  return &m_distributionCurveView;
}

void CalculationController::ContentView::layoutSubviews(bool force) {
  KDSize tableSize = m_selectableTableView->minimalSizeForOptimalDisplay();
  m_selectableTableView->setFrame(KDRect(0, 0, bounds().width(), tableSize.height()), force);
  m_distributionCurveView.setFrame(
      KDRect(0, tableSize.height(), bounds().width(), bounds().height() - tableSize.height()),
      force);
}

CalculationController::CalculationController(Escher::StackViewController * parentResponder,
                                             InputEventHandlerDelegate * inputEventHandlerDelegate,
                                             Distribution * distribution,
                                             Calculation * calculation) :
      Page(parentResponder),
      m_calculation(calculation),
      m_distribution(distribution),
      m_contentView(&m_selectableTableView, distribution, calculation),
      m_selectableTableView(this),
      m_imagesDataSource(distribution),
      m_dropdown(&m_selectableTableView, &m_imagesDataSource, this) {
  assert(distribution != nullptr);
  assert(calculation != nullptr);
  m_selectableTableView.setMargins(k_tableMargin);
  m_selectableTableView.setVerticalCellOverlap(0);
  m_selectableTableView.setDecoratorType(ScrollView::Decorator::Type::None);
  m_selectableTableView.setBackgroundColor(KDColorWhite);

  for (int i = 0; i < k_numberOfCalculationCells; i++) {
    m_calculationCells[i].editableTextCell()->setParentResponder(&m_selectableTableView);
    m_calculationCells[i].editableTextCell()->textField()->setDelegates(inputEventHandlerDelegate,
                                                                        this);
  }

  HighlightImageCell * firstImageCell = m_imagesDataSource.reusableCell(0, 0);
  m_dropdown.setInnerCell(firstImageCell);
  firstImageCell->setImage(ImageStore::Calculation1Icon);
}

void CalculationController::didBecomeFirstResponder() {
  Probability::App::app()->setPage(Data::Page::ProbaGraph);
  updateTitle();
  reloadDistributionCurveView();
  m_dropdown.init();
  m_selectableTableView.reloadData();
}

bool CalculationController::handleEvent(Ion::Events::Event event) {
  if (m_selectableTableView.selectedColumn() == 0 && event == Ion::Events::Down) {
    m_dropdown.open();
    return true;
  }
  return false;
}

void CalculationController::viewWillAppear() {
  ViewController::viewWillAppear();
  selectCellAtLocation(1, 0);
}

void CalculationController::viewDidDisappear() {
  m_selectableTableView.deselectTable();
  ViewController::viewDidDisappear();
}

int CalculationController::numberOfColumns() const {
  return m_calculation->numberOfParameters() + 1;
}

/* WARNING: we set one type per cell to be able to deduce the column width from
 * the cell minimalSizeForOptimalDisplay. Otherwise, we can not know which cell
 * to interrogate to get the column width and we neither can call
 * tableView->cellAtLocation as this function depends on the
 * numberOfDisplaybleRows which depends on the column width! */

KDCoordinate CalculationController::columnWidth(int i) {
  if (i == 0) {
    return m_dropdown.minimalSizeForOptimalDisplay().width();
  }
  // WARNING: that is possible only because we know which view cell corresponds to which cell
  return m_calculationCells[i - 1].minimalSizeForOptimalDisplay().width();
}

KDCoordinate CalculationController::rowHeight(int j) {
  return m_dropdown.minimalSizeForOptimalDisplay().height();
}

KDCoordinate CalculationController::cumulatedHeightFromIndex(int j) {
  return rowHeight(0) * j;
}

int CalculationController::indexFromCumulatedHeight(KDCoordinate offsetY) {
  KDCoordinate height = rowHeight(0);
  if (height == 0) {
    return 0;
  }
  return (offsetY - 1) / height;
}

HighlightCell * CalculationController::reusableCell(int index, int type) {
  assert(index == 0);
  switch (type) {
    case 0:
      return &m_dropdown;
    default:
      return &m_calculationCells[type - 1];
  }
}

void CalculationController::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  if (i > 0) {
    CalculationCell * myCell = static_cast<CalculationCell *>(cell);
    myCell->messageTextView()->setMessage(m_calculation->legendForParameterAtIndex(i - 1));
    bool calculationCellIsResponder = true;
    if ((m_distribution->type() != Distribution::Type::Normal && i == 3) ||
        (m_calculation->type() == Calculation::Type::Discrete && i == 2)) {
      calculationCellIsResponder = false;
    }
    myCell->setResponder(calculationCellIsResponder);
    TextField * field = static_cast<CalculationCell *>(cell)->editableTextCell()->textField();
    if (field->isEditing()) {
      return;
    }
    constexpr int bufferSize = Constants::k_shortBufferSize;
    char buffer[bufferSize];
    // FIXME: Leo has not decided yet if we should use the prefered mode instead of always using
    // scientific mode
    defaultConvertFloatToText(m_calculation->parameterAtIndex(i - 1), buffer, bufferSize);
    field->setText(buffer);
  }
}

bool CalculationController::textFieldDidHandleEvent(::TextField * textField,
                                                    bool returnValue,
                                                    bool textSizeDidChange) {
  if (returnValue && textSizeDidChange) {
    /* We do not reload the responder because the first responder might be the
     * toolbox (or the variable  box) and reloading the responder would corrupt
     * the first responder. */
    bool shouldUpdateFirstResponder = Container::activeApp()->firstResponder() == textField;
    m_selectableTableView.reloadData(shouldUpdateFirstResponder);
    // The textField frame might have increased which forces to reload the textField scroll
    textField->scrollToCursor();
  }
  return returnValue;
}

bool CalculationController::textFieldShouldFinishEditing(TextField * textField,
                                                         Ion::Events::Event event) {
  return TextFieldDelegate::textFieldShouldFinishEditing(textField, event) ||
         (event == Ion::Events::Right &&
          textField->cursorLocation() == textField->text() + textField->draftTextLength() &&
          selectedColumn() < m_calculation->numberOfParameters()) ||
         (event == Ion::Events::Left && textField->cursorLocation() == textField->text());
}

bool CalculationController::textFieldDidFinishEditing(TextField * textField,
                                                      const char * text,
                                                      Ion::Events::Event event) {
  assert(selectedColumn() != 0);
  double floatBody;
  if (textFieldDelegateApp()->hasUndefinedValue(text, &floatBody)) {
    return false;
  }
  int resultColumn = m_calculation->type() == Calculation::Type::FiniteIntegral ? 3 : 2;
  if (selectedColumn() == resultColumn) {
    if (floatBody < 0.0) {
      floatBody = 0.0;
    }
    if (floatBody > 1.0) {
      floatBody = 1.0;
    }
  } else if (!m_distribution->isContinuous()) {
    assert(selectedColumn() == 1 || (selectedColumn() == 2 && m_calculation->type() == Calculation::Type::FiniteIntegral));
    floatBody = std::round(floatBody);
  }
  m_calculation->setParameterAtIndex(floatBody, selectedColumn() - 1);
  if (event == Ion::Events::Right || event == Ion::Events::Left) {
    m_selectableTableView.handleEvent(event);
  }
  reload();
  return true;
}

void CalculationController::reloadDistributionCurveView() {
  m_contentView.distributionCurveView()->reload();
}

void CalculationController::reload() {
  m_selectableTableView.reloadData();
  reloadDistributionCurveView();
}

void CalculationController::setCalculationAccordingToIndex(int index, bool forceReinitialisation) {
  if ((int)m_calculation->type() == index && !forceReinitialisation) {
    return;
  }
  m_calculation->~Calculation();
  switch (index) {
    case 0:
      new (m_calculation) LeftIntegralCalculation(m_distribution);
      return;
    case 1:
      new (m_calculation) FiniteIntegralCalculation(m_distribution);
      return;
    case 2:
      new (m_calculation) RightIntegralCalculation(m_distribution);
      return;
    case 3:
      new (m_calculation) DiscreteCalculation(m_distribution);
      return;
    default:
      return;
  }
}

void CalculationController::onDropdownSelected(int selectedRow) {
  setCalculationAccordingToIndex(selectedRow);
  reload();
}

bool CalculationController::popupDidReceiveEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Right) {
    m_dropdown.selectRow(m_dropdown.selectedRow());
    m_dropdown.close();
    m_dropdown.setHighlighted(false);
    setCalculationAccordingToIndex(m_dropdown.selectedRow());
    selectColumn(selectedColumn() + 1);
    reload();
    return true;
  }
  return false;
}

void CalculationController::updateTitle() {
  int currentChar = 0;
  for (int index = 0; index < m_distribution->numberOfParameter(); index++) {
    currentChar += Poincare::Print::customPrintf(m_titleBuffer + currentChar, k_titleBufferSize - currentChar, "%s = %*.*ed ",
        I18n::translate(m_distribution->parameterNameAtIndex(index)),
        m_distribution->parameterValueAtIndex(index), Poincare::Preferences::PrintFloatMode::Decimal, Poincare::Preferences::ShortNumberOfSignificantDigits);
  }
}

}  // namespace Probability
