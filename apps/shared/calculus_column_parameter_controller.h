#ifndef SHARED_CALCULUS_COLUMN_PARAMETER_CONTROLLER_H
#define SHARED_CALCULUS_COLUMN_PARAMETER_CONTROLLER_H

#include <apps/shared/function_store.h>
#include <apps/shared/values_controller.h>
#include <escher/menu_cell.h>
#include <escher/message_text_view.h>

namespace Shared {

class CalculusColumnParameterController : public ColumnParameterController {
 public:
  CalculusColumnParameterController(I18n::Message hideMessage,
                                    ValuesController* valuesController);

  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override { return k_totalNumberOfCell; }
  Escher::HighlightCell* cell(int row) override { return &m_hideColumn; }
  void setRecord(Ion::Storage::Record record) { m_record = record; }

 protected:
  virtual void hideCalculusColumn() = 0;
  Shared::ColumnNameHelper* columnNameHelper() override {
    return m_valuesController;
  }
  constexpr static int k_totalNumberOfCell = 1;
  Escher::MenuCell<Escher::MessageTextView> m_hideColumn;
  Ion::Storage::Record m_record;
  ValuesController* m_valuesController;
};

}  // namespace Shared

#endif
