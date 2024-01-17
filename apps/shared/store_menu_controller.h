#ifndef SHARED_STORE_MENU_CONTROLLER_H
#define SHARED_STORE_MENU_CONTROLLER_H

#include <apps/i18n.h>
#include <escher/editable_expression_cell.h>
#include <escher/editable_fiel_help_box.h>
#include <escher/list_view_data_source.h>
#include <escher/selectable_list_view.h>
#include <escher/selectable_table_view_data_source.h>
#include <escher/stack_view_controller.h>

#include "math_field_delegate.h"
#include "pop_up_controller.h"

namespace Shared {

class StoreMenuController : public Escher::ModalViewController,
                            public Escher::ListViewDataSource,
                            public Escher::SelectableListViewDataSource,
                            public MathLayoutFieldDelegate {
 public:
  StoreMenuController();
  void setText(const char* text);

  void open();
  void close();

  // Responder
  void didBecomeFirstResponder() override;

  // ListViewDataSource
  int numberOfRows() const override { return 1; }
  int reusableCellCount(int type) const override { return 1; }
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  int typeAtRow(int row) const override { return 0; }
  Escher::HighlightCell* reusableCell(int index, int type) override {
    return &m_cell;
  }
  KDCoordinate nonMemoizedRowHeight(int row) override {
    return m_cell.minimalSizeForOptimalDisplay().height();
  }

  // LayoutFieldDelegate
  bool layoutFieldDidFinishEditing(Escher::LayoutField* layoutField,
                                   Ion::Events::Event event) override;
  void layoutFieldDidAbortEditing(Escher::LayoutField* layoutField) override;
  void layoutFieldDidChangeSize(Escher::LayoutField* layoutField) override;
  bool insertTextForStoEvent(Escher::LayoutField* layoutField) const override {
    return true;
  }

 private:
  class InnerListController : public ViewController {
   public:
    InnerListController(StoreMenuController* dataSource);
    const char* title() override {
      return I18n::translate(I18n::Message::DefineVariable);
    }
    Escher::View* view() override { return &m_selectableListView; }
    void didBecomeFirstResponder() override;

   private:
    Escher::SelectableListView m_selectableListView;
  };

  bool parseAndStore(const char* text);
  void openAbortWarning();

  char m_savedDraftTextBuffer[Escher::AbstractTextField::MaxBufferSize()];
  Escher::StackViewController m_stackViewController;
  InnerListController m_listController;
  Escher::EditableExpressionCell m_cell;
  Escher::MessagePopUpControllerWithCustomCancel m_abortController;
  bool m_preventReload;
};

}  // namespace Shared

#endif
