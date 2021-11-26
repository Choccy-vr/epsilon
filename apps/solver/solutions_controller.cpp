#include "solutions_controller.h"
#include "app.h"
#include "../shared/poincare_helpers.h"
#include <assert.h>
#include <limits.h>
#include <poincare/layout_helper.h>
#include <poincare/code_point_layout.h>
#include <poincare/horizontal_layout.h>
#include <poincare/preferences.h>
#include <poincare/symbol_abstract.h>
#include <poincare/symbol.h>
#include <poincare/vertical_offset_layout.h>
#include <algorithm>

using namespace Poincare;
using namespace Shared;
using namespace Escher;

namespace Solver {

constexpr KDColor SolutionsController::ContentView::k_backgroundColor;

SolutionsController::ContentView::ContentView(SolutionsController * controller) :
  m_warningMessageView0(KDFont::SmallFont, I18n::Message::Default, KDContext::k_alignCenter, KDContext::k_alignCenter, KDColorBlack, k_backgroundColor),
  m_warningMessageView1(KDFont::SmallFont, I18n::Message::Default, KDContext::k_alignCenter, KDContext::k_alignCenter, KDColorBlack, k_backgroundColor),
  m_selectableTableView(controller, controller, controller, controller),
  m_displayWarningMoreSolutions(false)
{
  m_selectableTableView.setBackgroundColor(k_backgroundColor);
  m_selectableTableView.setVerticalCellOverlap(0);
}

void SolutionsController::ContentView::drawRect(KDContext * ctx, KDRect rect) const {
  if (m_selectableTableView.numberOfDisplayableRows() == 0) {
    // No selectable table, fill the entire bound for background
    ctx->fillRect(KDRect(KDPointZero, bounds().size()), k_backgroundColor);
  } else if (m_displayWarningMoreSolutions) {
    // Fill the top margin for additional warnings
    ctx->fillRect(KDRect(KDPointZero, bounds().width(), k_topMargin), k_backgroundColor);
  }
}

void SolutionsController::ContentView::setWarning(bool warning) {
  m_displayWarningMoreSolutions = warning;
  KDCoordinate topMargin = m_displayWarningMoreSolutions ? 0 : Escher::Metric::CommonTopMargin;
  m_selectableTableView.setTopMargin(topMargin);
  // Fit m_selectableTableView scroll to content size
  m_selectableTableView.decorator()->setVerticalMargins(topMargin, Metric::CommonBottomMargin);
  /* m_displayWarningMoreSolutions might stay the same, but number of rows and
   * messages have changed. */
  layoutSubviews();
}

void SolutionsController::ContentView::setWarningMessages(I18n::Message message0, I18n::Message message1) {
  m_warningMessageView0.setMessage(message0);
  m_warningMessageView1.setMessage(message1);
}

int SolutionsController::ContentView::numberOfSubviews() const {
  // Exclude selectableTableView if there are no rows to display
  return (m_selectableTableView.numberOfDisplayableRows() == 0 ? 0 : 1) + 2 * m_displayWarningMoreSolutions;
}

View * SolutionsController::ContentView::subviewAtIndex(int index) {
  assert(index >= 0 && index < numberOfSubviews());
  if (m_displayWarningMoreSolutions) {
    if (index == 0) {
      return &m_warningMessageView0;
    }
    if (index == 1) {
      return &m_warningMessageView1;
    }
  }
  return &m_selectableTableView;
}

void SolutionsController::ContentView::layoutSubviews(bool force) {
  if (m_displayWarningMoreSolutions) {
    KDCoordinate textHeight = KDFont::SmallFont->glyphSize().height();
    KDCoordinate topMargin;
    // Empty warning messages are handled to center both single or double lines
    KDCoordinate warningMessage0Height = m_warningMessageView0.text()[0] == 0 ? 0 : textHeight;
    KDCoordinate warningMessage1Height = m_warningMessageView1.text()[0] == 0 ? 0 : textHeight;
    // Warning messages are vertically centered.
    if (m_selectableTableView.numberOfDisplayableRows() == 0) {
      // Warning messages must fit into the entire bound height
      topMargin = (bounds().height() - warningMessage0Height - warningMessage1Height) / 2;
    } else {
      // Warning messages must fit into a k_topMargin height bound
      topMargin = (k_topMargin - warningMessage0Height - warningMessage1Height) / 2;
      // Set table frame
      m_selectableTableView.setFrame(KDRect(0, k_topMargin, bounds().width(), bounds().height() - k_topMargin), force);
    }
    assert(topMargin >= 0);
    m_warningMessageView0.setFrame(KDRect(0, topMargin, bounds().width(), warningMessage0Height), force);
    m_warningMessageView1.setFrame(KDRect(0, topMargin + warningMessage0Height, bounds().width(), warningMessage1Height), force);
  } else {
    // Table frame occupy the entire view
    m_selectableTableView.setFrame(bounds(), force);
  }
}

SolutionsController::SolutionsController(Responder * parentResponder, EquationStore * equationStore) :
  ViewController(parentResponder),
  m_equationStore(equationStore),
  m_deltaCell(KDContext::k_alignCenter, KDContext::k_alignCenter),
  m_delta2Layout(),
  m_delta3Layout(),
  m_contentView(this)
{
  const char * delta = GlobalPreferences::sharedGlobalPreferences()->discriminantSymbol();
  size_t lenDelta = strlen(delta);
  const char * equalB = "=b";
  m_delta2Layout = HorizontalLayout::Builder(
    LayoutHelper::String(delta, lenDelta, KDFont::SmallFont),
    LayoutHelper::String(equalB, strlen(equalB), KDFont::SmallFont),
    VerticalOffsetLayout::Builder(CodePointLayout::Builder('2', KDFont::SmallFont), VerticalOffsetLayoutNode::Position::Superscript),
    LayoutHelper::String("-4ac", 4, KDFont::SmallFont)
  );
  m_delta3Layout = LayoutHelper::String(delta, lenDelta, KDFont::SmallFont);
  for (int i = 0; i < k_numberOfExactValueCells; i++) {
    m_exactValueCells[i].setParentResponder(m_contentView.selectableTableView());
  }
  for (int i = 0; i < k_numberOfApproximateValueCells; i++) {
    m_approximateValueCells[i].setFont(KDFont::LargeFont);
  }
  for (int i = 0; i < k_numberOfSymbolCells; i++) {
    m_symbolCells[i].setAlignment(KDContext::k_alignCenter, KDContext::k_alignCenter);
    m_symbolCells[i].setFont(KDFont::LargeFont);
  }
}

/* ViewController */
const char * SolutionsController::title() {
  if (m_equationStore->type() == EquationStore::Type::Monovariable) {
    return I18n::translate(I18n::Message::ApproximateSolution);
  }
  return I18n::translate(I18n::Message::Solution);
}

void SolutionsController::viewWillAppear() {
  ViewController::viewWillAppear();
  bool requireWarning = true;
  if (numberOfDisplayedSolutions() == 0) {
    // There are no solutions
    m_contentView.setWarningMessages(noSolutionMessage(), I18n::Message::Default);
  } else if (m_equationStore->type() == EquationStore::Type::Monovariable && m_equationStore->haveMoreApproximationSolutions()) {
    // There are more approximate solutions
    m_contentView.setWarningMessages(I18n::Message::OnlyFirstSolutionsDisplayed0, I18n::Message::OnlyFirstSolutionsDisplayed1);
  } else if (m_equationStore->type() == EquationStore::Type::PolynomialMonovariable && numberOfDisplayedSolutions() == 1) {
    // There are no real solutions
    assert(Preferences::sharedPreferences()->complexFormat() == Preferences::ComplexFormat::Real);
    m_contentView.setWarningMessages(I18n::Message::PolynomeHasNoRealSolution0, I18n::Message::PolynomeHasNoRealSolution1);
  } else {
    requireWarning = false;
  }
  m_contentView.setWarning(requireWarning);
  m_contentView.selectableTableView()->reloadData();
  selectCellAtLocation(0, 0);
}

void SolutionsController::viewDidDisappear() {
  selectCellAtLocation(-1, -1);
  // Destroy exact cell's layouts
  for (size_t i = 0; i < k_numberOfExactValueCells; i++) {
    m_exactValueCells[i].setLayouts(Poincare::Layout(), Poincare::Layout());
  }
}

void SolutionsController::didEnterResponderChain(Responder * previousFirstResponder) {
  // Select the most left present subview on all cells and reinitialize scroll
  for (int i = 0; i < EquationStore::k_maxNumberOfExactSolutions; i++) {
    m_exactValueCells[i].reinitSelection();
  }
}

/* TableViewDataSource */

int SolutionsController::numberOfRows() const {
  int numberOfRows = numberOfDisplayedSolutions();
  if (m_equationStore->numberOfUserVariables() > 0) {
    // Add the empty row if there are rows above predefined variables message
    numberOfRows += (numberOfRows > 0 ? 2 : 1) + m_equationStore->numberOfUserVariables();
  }
  return numberOfRows;
}

void SolutionsController::willDisplayCellAtLocation(HighlightCell * cell, int i, int j) {
  const int rowOfUserVariablesMessage = userVariablesMessageRow();
  if (j == rowOfUserVariablesMessage - 1) {
    return; // Empty row
  }
  if (j == rowOfUserVariablesMessage) {
    // Predefined variable used/ignored message
    assert(i >= 0);
    MessageCell * messageCell = static_cast<MessageCell *>(cell);
    // Message is split across two cells : |**** used | predefined  vars ****|
    messageCell->setHorizontalAlignment(i == 0 ? 1.0f : 0.0f);
    if (usedUserVariables()) {
      messageCell->setMessage(i == 0 ? I18n::Message::PredefinedVariablesUsedLeft : I18n::Message::PredefinedVariablesUsedRight);
    } else {
      messageCell->setMessage(i == 0 ? I18n::Message::PredefinedVariablesIgnoredLeft : I18n::Message::PredefinedVariablesIgnoredRight);
    }
    return;
  }
  if (i == 0) {
    if (m_equationStore->type() == EquationStore::Type::PolynomialMonovariable && j == numberOfDisplayedSolutions() - 1) {
      // Formula of the discriminant
      assert(m_equationStore->degree() == 2 || m_equationStore->degree() == 3);
      EvenOddExpressionCell * deltaCell = static_cast<EvenOddExpressionCell *>(cell);
      deltaCell->setLayout(m_equationStore->degree() == 2 ? m_delta2Layout : m_delta3Layout);
    } else {
      EvenOddBufferTextCell * symbolCell = static_cast<EvenOddBufferTextCell *>(cell);
      char bufferSymbol[Poincare::SymbolAbstract::k_maxNameSize+2]; // Holds at maximum the variable name + 2 digits (for 10)
      if (rowOfUserVariablesMessage < 0 || j < rowOfUserVariablesMessage - 1) {
        // It's a solution row, get symbol name
        if (m_equationStore->type() == EquationStore::Type::LinearSystem) {
          /* The system has more than one variable: the cell text is the
           * variable name */
          strlcpy(bufferSymbol, m_equationStore->variableAtIndex(j), Poincare::SymbolAbstract::k_maxNameSize);
        } else {
          /* The system has one variable but might have many solutions: the cell
           * text is variableX, with X the row index + 1 (e.g. x1, x2,...) */
          int length = strlcpy(bufferSymbol, m_equationStore->variableAtIndex(0), Poincare::SymbolAbstract::k_maxNameSize);
          if (j < 9) {
            bufferSymbol[length++] = j+'1';
          } else {
            assert(j == 9);
            bufferSymbol[length++] = '1';
            bufferSymbol[length++] = '0';
          }
          bufferSymbol[length] = 0;
        }
      } else {
        // It's a user variable row, get variable name
        assert(rowOfUserVariablesMessage >= 0);
        strlcpy(bufferSymbol, m_equationStore->userVariableAtIndex(j - rowOfUserVariablesMessage - 1), Poincare::SymbolAbstract::k_maxNameSize);
      }
      symbolCell->setText(bufferSymbol);
    }
  } else {
    if (rowOfUserVariablesMessage < 0 || j < rowOfUserVariablesMessage - 1) {
      // It's a solution row
      assert(numberOfDisplayedSolutions() > 0);
      if (m_equationStore->type() == EquationStore::Type::Monovariable) {
        // Get values of the solutions
        EvenOddBufferTextCell * valueCell = static_cast<EvenOddBufferTextCell *>(cell);
        constexpr int precision = Preferences::LargeNumberOfSignificantDigits;
        constexpr int bufferSize = PrintFloat::charSizeForFloatsWithPrecision(precision);
        char bufferValue[bufferSize];
        PoincareHelpers::ConvertFloatToText<double>(m_equationStore->approximateSolutionAtIndex(j), bufferValue, bufferSize, precision);
        valueCell->setText(bufferValue);
      } else {
        // Get values of the solutions or discriminant
        ScrollableTwoExpressionsCell * valueCell = static_cast<ScrollableTwoExpressionsCell *>(cell);
        Poincare::Layout exactLayout = m_equationStore->exactSolutionLayoutsAtIndexAreIdentical(j) ? Poincare::Layout() : m_equationStore->exactSolutionLayoutAtIndex(j, true);
        valueCell->setLayouts(exactLayout, m_equationStore->exactSolutionLayoutAtIndex(j, false));
        if (!exactLayout.isUninitialized()) {
          valueCell->setEqualMessage(m_equationStore->exactSolutionLayoutsAtIndexAreEqual(j) ? I18n::Message::Equal : I18n::Message::AlmostEqual);
        }
      }
    } else {
      // It's a user variable row, get values of the solutions or discriminant
      ScrollableTwoExpressionsCell * valueCell = static_cast<ScrollableTwoExpressionsCell *>(cell);
      const char * symbol = m_equationStore->userVariableAtIndex(j - rowOfUserVariablesMessage - 1);
      Poincare::Layout layout = PoincareHelpers::CreateLayout(App::app()->localContext()->expressionForSymbolAbstract(Poincare::Symbol::Builder(symbol, strlen(symbol)), false));
      valueCell->setLayouts(Poincare::Layout(), layout);
    }
  }
  EvenOddCell * evenOddCell = static_cast<EvenOddCell *>(cell);
  evenOddCell->setEven(j%2 == 0);
}

KDCoordinate SolutionsController::rowHeight(int j) {
  const int rowOfUserVariablesMessage = userVariablesMessageRow();
  if (rowOfUserVariablesMessage < 0 || j < rowOfUserVariablesMessage - 1) {
    // It's a solution row
    assert(numberOfDisplayedSolutions() > 0);
    if (m_equationStore->type() == EquationStore::Type::Monovariable) {
      return k_defaultCellHeight;
    }
    Poincare::Layout exactLayout = m_equationStore->exactSolutionLayoutAtIndex(j, true);
    Poincare::Layout approximateLayout = m_equationStore->exactSolutionLayoutAtIndex(j, false);
    KDCoordinate exactLayoutHeight = exactLayout.layoutSize().height();
    KDCoordinate approximateLayoutHeight = approximateLayout.layoutSize().height();
    KDCoordinate layoutHeight = std::max(exactLayout.baseline(), approximateLayout.baseline()) + std::max(exactLayoutHeight-exactLayout.baseline(), approximateLayoutHeight-approximateLayout.baseline());
    return layoutHeight + 2 * Metric::CommonSmallMargin;
  }
  if (j == rowOfUserVariablesMessage || j == rowOfUserVariablesMessage - 1 ) {
    // It's an empty or user variable message row
    return k_defaultCellHeight;
  }
  // TODO: memoize user symbols if too slow
  const char * symbol = m_equationStore->userVariableAtIndex(j - rowOfUserVariablesMessage - 1);
  Poincare::Layout layout = PoincareHelpers::CreateLayout(App::app()->localContext()->expressionForSymbolAbstract(Poincare::Symbol::Builder(symbol, strlen(symbol)), false));
  return layout.layoutSize().height() + 2 * Metric::CommonSmallMargin;
}

KDCoordinate SolutionsController::cumulatedWidthFromIndex(int i) {
  switch (i) {
    case 0:
      return 0;
    case 1:
      return k_symbolCellWidth;
    default:
      assert(i == 2);
      return k_symbolCellWidth+k_valueCellWidth;
  }
}

int SolutionsController::indexFromCumulatedWidth(KDCoordinate offsetX) {
  if (offsetX <= k_symbolCellWidth) {
    return 0;
  } else {
    if (offsetX <= k_symbolCellWidth+k_valueCellWidth)
      return 1;
    else {
      return 2;
    }
  }
}

HighlightCell * SolutionsController::reusableCell(int index, int type) {
  switch (type) {
    case k_symbolCellType:
      return &m_symbolCells[index];
    case k_deltaCellType:
      return &m_deltaCell;
    case k_exactValueCellType:
      return &m_exactValueCells[index];
    case k_messageCellType:
      return &m_messageCells[index];
    case k_emptyCellType:
      return &m_emptyCell[index];
    default:
      assert(type == k_approximateValueCellType);
      return &m_approximateValueCells[index];
  }
}

int SolutionsController::reusableCellCount(int type) {
  switch (type) {
    case k_symbolCellType:
      return k_numberOfSymbolCells;
    case k_deltaCellType:
      return 1;
    case k_exactValueCellType:
      return k_numberOfExactValueCells;
    case k_messageCellType:
      return k_numberOfMessageCells;
    case k_emptyCellType:
      return k_numberOfEmptyCells;
    default:
      assert(type == k_approximateValueCellType);
      return k_numberOfApproximateValueCells;
  }
}

int SolutionsController::typeAtLocation(int i, int j) {
  const int rowOfUserVariableMessage = userVariablesMessageRow();
  if (j == rowOfUserVariableMessage - 1) {
    return k_emptyCellType;
  }
  if (j == rowOfUserVariableMessage) {
    return k_messageCellType;
  }
  if (i == 0) {
    if (m_equationStore->type() == EquationStore::Type::PolynomialMonovariable && j == numberOfDisplayedSolutions() - 1) {
      return k_deltaCellType;
    }
    return k_symbolCellType;
  }
  if ((rowOfUserVariableMessage < 0 || j < rowOfUserVariableMessage - 1) && m_equationStore->type() == EquationStore::Type::Monovariable) {
    return k_approximateValueCellType;
  }
  return k_exactValueCellType;
}

void SolutionsController::didBecomeFirstResponder() {
  Container::activeApp()->setFirstResponder(m_contentView.selectableTableView());
}

void SolutionsController::tableViewDidChangeSelection(SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) {
  const int rowOfUserVariablesMessage = userVariablesMessageRow();
  if (rowOfUserVariablesMessage < 0) {
    return;
  }
  // Forbid the selection of the messages row and its empty row
  if (t->selectedRow() == rowOfUserVariablesMessage || t->selectedRow() == rowOfUserVariablesMessage - 1) {
    t->selectCellAtLocation(t->selectedColumn(), rowOfUserVariablesMessage + (rowOfUserVariablesMessage < previousSelectedCellY ? -2 : 1));
  }
}

int SolutionsController::userVariablesMessageRow() const {
  assert(m_equationStore->numberOfUserVariables() >= 0);
  if (m_equationStore->numberOfUserVariables() == 0) {
    // No user variables
    return -1;
  }
  if (numberOfDisplayedSolutions() == 0) {
    // Message row is first row, no need for an empty row
    return 0;
  }
  // Add an additional empty cell as a margin
  return numberOfDisplayedSolutions() + 1;
}

int SolutionsController::numberOfDisplayedSolutions() const {
  // No displayed solutions if there are an infinite number of them.
  return m_equationStore->numberOfSolutions() == INT_MAX ? 0 : m_equationStore->numberOfSolutions();
}

I18n::Message SolutionsController::noSolutionMessage() {
  if (m_equationStore->numberOfSolutions() == INT_MAX) {
    return I18n::Message::InfiniteNumberOfSolutions;
  }
  if (m_equationStore->type() == EquationStore::Type::Monovariable) {
    return I18n::Message::NoSolutionInterval;
  }
  if (m_equationStore->numberOfDefinedModels() <= 1) {
    return I18n::Message::NoSolutionEquation;
  }
  return I18n::Message::NoSolutionSystem;
}

}
