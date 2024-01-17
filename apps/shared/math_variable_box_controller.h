#ifndef APPS_MATH_VARIABLE_BOX_CONTROLLER_H
#define APPS_MATH_VARIABLE_BOX_CONTROLLER_H

#include <apps/i18n.h>
#include <escher/layout_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/nested_menu_controller.h>
#include <escher/scrollable_layout_view.h>
#include <ion.h>

namespace Shared {

/*  WARNING: MathVariableBoxController can only be used within AppWithStoreMenu,
 * since it does intrusive storage changes. */

class MathVariableBoxController : public Escher::NestedMenuController {
 public:
  MathVariableBoxController();

  // View Controller
  void viewDidDisappear() override;

  // Responder
  bool handleEvent(Ion::Events::Event event) override;

  // ListViewDataSource
  int numberOfRows() const override;
  int reusableCellCount(int type) const override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  KDCoordinate nonMemoizedRowHeight(int row) override;
  int typeAtRow(int row) const override;

  // Menu
  enum class Page {
    RootMenu = 0,
    Expression = 1,
    Function = 2,
    List = 3,
    Matrix = 4,
    Sequence = 5,
    NumberOfPages = 6  // use this value only to know the number of pages
  };

 private:
  constexpr static int k_maxNumberOfDisplayedRows =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          Escher::AbstractMenuCell::k_minimalLargeFontCellHeight,
          Escher::Metric::PopUpMargins.top() +
              Escher::Metric::StackTitleHeight);
  constexpr static int k_numberOfMenuRows =
      static_cast<int>(Page::NumberOfPages) - 1 /* RootMenu */ +
      1 /* DefineVariable */;
  constexpr static KDCoordinate k_leafMargin = 20;
  constexpr static KDFont::Size k_subLabelFont = KDFont::Size::Small;
  constexpr static int k_defineVariableCellType = 2;

  using LeafCell =
      Escher::MenuCell<Escher::LayoutView, Escher::ScrollableLayoutView>;
  LeafCell* leafCellAtIndex(int index) override;
  int defineVariableCellIndex() const { return numberOfRows() - 1; }
  Escher::NestedMenuController::NodeCell* nodeCellAtIndex(int index) override;
  I18n::Message subTitle() override;
  int numberOfElements(Page page) const;
  Page pageAtIndex(int index);
  void setPage(Page page);
  bool selectSubMenu(int selectedRow) override;
  bool returnToPreviousMenu() override;
  bool returnToRootMenu() override;
  bool selectLeaf(int selectedRow) override;
  I18n::Message nodeLabel(Page page);
  Poincare::Layout expressionLayoutForRecord(Ion::Storage::Record record,
                                             int index);
  static const char* Extension(Page page);
  Ion::Storage::Record recordAtIndex(int row);
  void resetVarBoxMemoization();
  // Return false if destruction is prevented by the app
  bool destroyRecordAtRow(int row);
  Page m_currentPage;
  LeafCell m_leafCells[k_maxNumberOfDisplayedRows];
  Escher::NestedMenuController::NodeCell m_nodeCells[k_numberOfMenuRows];
  Escher::MenuCell<Escher::MessageTextView> m_defineVariableCell;
  /* Layout memoization
   * TODO: make a helper doing the RingMemoizationOfConsecutiveObjets to
   * factorize this code and ExpressionModelStore code. */
  int m_firstMemoizedLayoutIndex;
  Poincare::Layout m_layouts[k_maxNumberOfDisplayedRows];
};

}  // namespace Shared

#endif
