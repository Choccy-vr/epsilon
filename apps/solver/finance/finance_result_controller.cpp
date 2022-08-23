#include "finance_result_controller.h"
#include <apps/i18n.h>
#include <apps/shared/poincare_helpers.h>
#include <escher/clipboard.h>
#include <escher/container.h>
#include <poincare/print.h>
#include <assert.h>

using namespace Solver;

FinanceResultController::FinanceResultController(Escher::StackViewController * parentResponder, InterestData * data) :
      Escher::SelectableCellListPage<Escher::MessageTableCellWithMessageWithBuffer, k_numberOfResultCells>(parentResponder),
      m_messageView(KDFont::SmallFont, I18n::Message::CalculatedValues, KDContext::k_alignCenter, KDContext::k_alignCenter, Escher::Palette::GrayDark, Escher::Palette::WallScreen),
      m_contentView(&m_selectableTableView, this, &m_messageView),
      m_data(data) {
}

void FinanceResultController::didBecomeFirstResponder() {
  /* Build the result cell here because it only needs to be updated once this
   * controller become first responder. */
  cellAtIndex(0)->setMessage(m_data->labelForParameter(m_data->getUnknown()));
  cellAtIndex(0)->setSubLabelMessage(m_data->sublabelForParameter(m_data->getUnknown()));
  double value = m_data->computeUnknownValue();
  constexpr int precision = Poincare::Preferences::VeryLargeNumberOfSignificantDigits;
  constexpr int bufferSize = Poincare::PrintFloat::charSizeForFloatsWithPrecision(precision);
  char buffer[bufferSize];
  Shared::PoincareHelpers::ConvertFloatToTextWithDisplayMode<double>(value, buffer, bufferSize, precision, Poincare::Preferences::PrintFloatMode::Decimal);
  cellAtIndex(0)->setAccessoryText(buffer);
  resetMemoization(true);
  selectRow(-1);
  m_contentView.reload();
}

bool FinanceResultController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Copy || event == Ion::Events::Cut) {
    Escher::Clipboard::sharedClipboard()->store(cellAtIndex(0)->text());
    return true;
  }
  return popFromStackViewControllerOnLeftEvent(event);
}

const char * FinanceResultController::title() {
  /* Try fitting the known parameters values in the title using a minimal
   * precision. Use "..." at the end if not all parameters fit. */
  constexpr int precision = Poincare::Preferences::ShortNumberOfSignificantDigits;
  constexpr Poincare::Preferences::PrintFloatMode printFloatMode = Poincare::Preferences::PrintFloatMode::Decimal;
  // At least "..." should fit in the title.
  assert(k_titleBufferSize > strlen("..."));
  const char * parameterTemplate = "%s=%*.*ed...";
  const char * lastKnownParameterTemplate = "%s=%*.*ed";
  // The boolean parameter isn't displayed
  uint8_t doubleParameters = m_data->numberOfDoubleValues();
  uint8_t unknownParam = m_data->getUnknown();
  bool unknownParamIsLast = (unknownParam == doubleParameters - 1);
  int length = 0;
  for (uint8_t param = 0; param < doubleParameters; param++) {
    if (param == unknownParam) {
      // The unknown parameter isn't displayed
      continue;
    }
    // Ensure "..." fits if it isn't the last known parameter
    bool lastKnownParameter = (param == doubleParameters - 1 - unknownParamIsLast);
    // Attempting the parameter insertion
    int parameterLength = Poincare::Print::safeCustomPrintf(
        m_titleBuffer + length, k_titleBufferSize - length,
        (lastKnownParameter ? lastKnownParameterTemplate : parameterTemplate),
        I18n::translate(m_data->labelForParameter(param)),
        m_data->getValue(param), printFloatMode, precision);
    if (length + parameterLength >= k_titleBufferSize) {
      // Text did not fit, insert "..." and overwite last " " if there is one
      if (length > strlen(" ")) {
        length -= strlen(" ");
      }
      length += Poincare::Print::customPrintf(m_titleBuffer + length, k_titleBufferSize - length, "...");
      break;
    }
    length += parameterLength;
    if (!lastKnownParameter) {
      // Text did fit, "..." isn't needed for now and can be replaced with " "
      length -= strlen("...");
      length += Poincare::Print::customPrintf(m_titleBuffer + length, k_titleBufferSize - length, " ");
    }
  }
  assert(length < k_titleBufferSize);
  m_titleBuffer[length] = 0;
  return m_titleBuffer;
}
