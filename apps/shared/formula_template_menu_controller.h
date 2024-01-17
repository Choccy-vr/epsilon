#ifndef APPS_SHARED_FORMULA_TEMPLATE_MENU_CONTROLLER_H
#define APPS_SHARED_FORMULA_TEMPLATE_MENU_CONTROLLER_H

#include <apps/i18n.h>
#include <escher/buffer_text_view.h>
#include <escher/layout_view.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>
#include <escher/nested_menu_controller.h>
#include <escher/selectable_list_view_controller.h>

namespace Shared {

class StoreColumnHelper;

class FormulaTemplateMenuController
    : public Escher::SelectableListViewController<
          Escher::StandardMemoizedListViewDataSource> {
 public:
  constexpr static int k_maxSizeOfTemplateText = 30;
  FormulaTemplateMenuController(Escher::Responder* parentResponder,
                                StoreColumnHelper* storeColumnHelper);

  const char* title() override;
  void viewWillAppear() override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  void viewDidDisappear() override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override {
    return k_numberOfTemplates - (shouldDisplayOtherAppCell() ? 0 : 1);
  }
  KDCoordinate nonMemoizedRowHeight(int row) override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  int typeAtRow(int row) const override;

 private:
  enum class Cell {
    EmptyTemplate = 0,
    Integers = 1,
    Random = 2,
    Logarithm = 3,
    OtherColumns = 4,
    OtherApp = 5,
    MaxNumberOfRows = 6  // Only used to know the number of cells.
  };
  constexpr static int k_numberOfTemplates =
      static_cast<int>(Cell::MaxNumberOfRows);
  constexpr static int k_numberOfExpressionCellsWithMessage =
      1 + static_cast<int>(Cell::Logarithm) - static_cast<int>(Cell::Integers);
  constexpr static int k_numberOfExpressionCellsWithBuffer =
      1 + static_cast<int>(Cell::OtherApp) -
      static_cast<int>(Cell::OtherColumns);
  static_assert(k_numberOfTemplates ==
                    1 + k_numberOfExpressionCellsWithMessage +
                        k_numberOfExpressionCellsWithBuffer,
                "Wrong number of cells in FormulaTemplateMenuController");
  enum class CellType {
    EmptyTemplate = 0,
    TemplateWithMessage = 1,
    TemplateWithBuffer = 2
  };
  constexpr static const char*
      k_templates[k_numberOfExpressionCellsWithMessage] = {
          "sequence(k,k,10)", "sequence(random(),k,10)",
          "sequence(ln(k),k,10)"};
  // TODO : Replace with translations
  constexpr static I18n::Message
      k_subLabelMessages[k_numberOfExpressionCellsWithMessage +
                         k_numberOfExpressionCellsWithBuffer] = {
          I18n::Message::FormulaTemplateInt,
          I18n::Message::FormulaTemplateRand,
          I18n::Message::FormulaTemplateLog,
          I18n::Message::FormulaTemplateSum,
          I18n::Message::FormulaTemplateList,
      };

  using BufferTemplateCell =
      Escher::MenuCell<Escher::LayoutView, Escher::OneLineBufferTextView<>>;
  using MessageTemplateCell =
      Escher::MenuCell<Escher::LayoutView, Escher::MessageTextView>;

  int relativeCellIndex(int index, CellType type);
  bool shouldDisplayOtherAppCell() const;
  Poincare::Expression templateExpressionForCell(Cell cell);
  void computeUninitializedLayouts();
  void fillSubLabelBuffer(BufferTemplateCell* cell, int index);
  void fillSumColumnNames(char* buffers[]) const;
  void fillOtherAppColumnName(char* buffer) const;

  Escher::MenuCell<Escher::MessageTextView> m_emptyTemplateCell;
  MessageTemplateCell
      m_templatesWithMessage[k_numberOfExpressionCellsWithMessage];
  BufferTemplateCell m_templatesWithBuffer[k_numberOfExpressionCellsWithBuffer];
  Poincare::Layout m_layouts[k_numberOfTemplates - 1];
  StoreColumnHelper* m_storeColumnHelper;
};

}  // namespace Shared
#endif
