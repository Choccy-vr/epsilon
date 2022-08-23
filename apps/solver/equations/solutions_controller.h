#ifndef SOLVER_SOLUTIONS_CONTROLLER_H
#define SOLVER_SOLUTIONS_CONTROLLER_H

#include <apps/i18n.h>
#include <escher/alternate_empty_view_controller.h>
#include <escher/even_odd_buffer_text_cell.h>
#include <escher/even_odd_expression_cell.h>
#include <escher/selectable_table_view.h>
#include <escher/selectable_table_view_data_source.h>
#include <escher/selectable_table_view_delegate.h>
#include <escher/table_view_data_source.h>
#include <ion.h>
#include "equation_store.h"
#include <apps/shared/scrollable_two_expressions_cell.h>

namespace Solver {

class SolutionsController : public Escher::ViewController, public Escher::AlternateEmptyViewDefaultDelegate, public Escher::SelectableTableViewDataSource, public Escher::TableViewDataSource, public Escher::SelectableTableViewDelegate {
public:
  SolutionsController(Escher::Responder * parentResponder, EquationStore * equationStore);
  /* ViewController */
  const char * title() override;
  Escher::View * view() override { return &m_contentView; }
  void viewWillAppear() override;
  void viewDidDisappear() override;
  void didEnterResponderChain(Escher::Responder * previousFirstResponder) override;
  TELEMETRY_ID("Solutions");

  /* AlternateEmptyViewDefaultDelegate */
  bool isEmpty() const override { return false; }  // View cannot be empty
  I18n::Message emptyMessage() override { assert(false); return static_cast<I18n::Message>(0); }
  Escher::Responder * defaultController() override { return parentResponder()->parentResponder(); }
  /* TableViewDataSource */
  int numberOfRows() const override;
  int numberOfColumns() const override { return 2; }
  void willDisplayCellAtLocation(Escher::HighlightCell * cell, int i, int j) override;
  KDCoordinate columnWidth(int i) override { return i == 0 ? k_symbolCellWidth : k_valueCellWidth; }
  KDCoordinate rowHeight(int j) override;
  KDCoordinate cumulatedWidthFromIndex(int i) override;
  int indexFromCumulatedWidth(KDCoordinate offsetX) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override;
  /* Responder */
  void didBecomeFirstResponder() override;
  /* SelectableTableViewDelegate */
  void tableViewDidChangeSelection(Escher::SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection = false) override;
private:
  class ContentView : public Escher::View {
  public:
    constexpr static KDCoordinate k_topMargin = 50;
    constexpr static KDColor k_backgroundColor = Escher::Palette::WallScreenDark;
    ContentView(SolutionsController * controller);
    void drawRect(KDContext * ctx, KDRect rect) const override;
    void setWarning(bool warning);
    void setWarningMessages(I18n::Message message0, I18n::Message message1);
    Escher::SelectableTableView * selectableTableView() { return &m_selectableTableView; }
  private:
    int numberOfSubviews() const override;
    Escher::View * subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    Escher::MessageTextView m_warningMessageView0;
    Escher::MessageTextView m_warningMessageView1;
    Escher::SelectableTableView m_selectableTableView;
    bool m_displayWarningMoreSolutions;
  };

  class MessageCell : public Escher::HighlightCell {
  public:
    MessageCell() : m_messageView(KDFont::SmallFont, (I18n::Message)0, 0.0f, k_verticalAlignment, KDColorBlack, SolutionsController::ContentView::k_backgroundColor) {}
    void setBackgroundColor(KDColor color) { m_messageView.setBackgroundColor(color); }
    void setHorizontalAlignment(float alignment) { m_messageView.setAlignment(alignment, k_verticalAlignment); }
    void setMessage(I18n::Message message) { m_messageView.setMessage(message); }
  private:
    /* Text is placed at the very top of the cell to simplify text centering
     * when there are no cells above. To add a "margin" in other cases, we
     * precede the message row with an empty row. */
    constexpr static float k_verticalAlignment = 0.0f;
    int numberOfSubviews() const override { return 1; }
    Escher::View * subviewAtIndex(int index) override { assert(index == 0); return &m_messageView; }
    void layoutSubviews(bool force = false) override { m_messageView.setFrame(bounds(), force); }
    Escher::MessageTextView m_messageView;
  };

  class EmptyCell : public Escher::HighlightCell {
  public:
    void drawRect(KDContext * ctx, KDRect rect) const override { ctx->fillRect(bounds(), SolutionsController::ContentView::k_backgroundColor); }
  };

  // Cell types
  constexpr static int k_symbolCellType = 0;
  constexpr static int k_deltaCellType = 1;
  constexpr static int k_exactValueCellType = 2;
  constexpr static int k_approximateValueCellType = 3;
  constexpr static int k_messageCellType = 4;
  constexpr static int k_emptyCellType = 5;

  // Heights and widths
  constexpr static KDCoordinate k_defaultCellHeight = 20;
  constexpr static int k_symbolCellWidth = 10 * (Poincare::SymbolAbstract::k_maxNameSize - 1 + 2) + 2 * Escher::EvenOddBufferTextCell::k_horizontalMargin; // We concatenate symbol name with a number of at most 2 digits and KDFont::LargeFont->glyphSize().width() = 10
  constexpr static int k_valueCellWidth = 190;

  // Number of cells
  constexpr static int k_maxNumberOfVisibleCells = (Ion::Display::Height - 3 * Escher::Metric::TitleBarHeight) / k_defaultCellHeight + 1; // When displaying approximate solutions for cos(x) = 0 between 0 and 1800 and scrolling down
  static_assert(k_maxNumberOfVisibleCells == 10, "k_maxNumberOfVisibleCells has changed"); // This assert is just for information purposes
  static_assert(k_maxNumberOfVisibleCells <= EquationStore::k_maxNumberOfSolutions + Poincare::Expression::k_maxNumberOfVariables, "We can reduce the number of cells in Solver:SolutionsController.");
  constexpr static int k_maxNumberOfSymbols = EquationStore::k_maxNumberOfSolutions + Poincare::Expression::k_maxNumberOfVariables;
  constexpr static int k_numberOfSymbolCells = (k_maxNumberOfVisibleCells < k_maxNumberOfSymbols) ? k_maxNumberOfVisibleCells : k_maxNumberOfSymbols;
  constexpr static int k_maxNumberOfExactValues = EquationStore::k_maxNumberOfExactSolutions + Poincare::Expression::k_maxNumberOfVariables;
  constexpr static int k_numberOfExactValueCells = (k_maxNumberOfVisibleCells < k_maxNumberOfExactValues) ? k_maxNumberOfVisibleCells : k_maxNumberOfExactValues;
  constexpr static int k_numberOfApproximateValueCells = 1 + (k_maxNumberOfVisibleCells < EquationStore::k_maxNumberOfApproximateSolutions ? k_maxNumberOfVisibleCells : EquationStore::k_maxNumberOfApproximateSolutions);
  constexpr static int k_numberOfMessageCells = 2;
  constexpr static int k_numberOfEmptyCells = 2;

  bool usedUserVariables() const { return m_equationStore->userVariablesUsed(); }
  int userVariablesMessageRow() const;
  int numberOfDisplayedSolutions() const;
  I18n::Message noSolutionMessage();

  EquationStore * m_equationStore;
  Escher::EvenOddBufferTextCell m_symbolCells[k_numberOfSymbolCells];
  Escher::EvenOddExpressionCell m_deltaCell;
  Poincare::Layout m_delta2Layout;
  Poincare::Layout m_delta3Layout;
  Shared::ScrollableTwoExpressionsCell m_exactValueCells[k_numberOfExactValueCells];
  Escher::EvenOddBufferTextCell m_approximateValueCells[k_numberOfApproximateValueCells];
  MessageCell m_messageCells[k_numberOfMessageCells];
  EmptyCell m_emptyCell[k_numberOfEmptyCells];
  ContentView m_contentView;
};

}

#endif

