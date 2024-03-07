#ifndef SHARED_FUNCTION_LIST_CONTROLLER_H
#define SHARED_FUNCTION_LIST_CONTROLLER_H

#include <escher/button_row_controller.h>
#include <escher/highlight_cell.h>
#include <escher/stack_view_controller.h>
#include <escher/tab_view_controller.h>

#include "expression_model_list_controller.h"
#include "list_parameter_controller.h"

namespace Shared {

class FunctionListController : public ExpressionModelListController,
                               public Escher::ButtonRowDelegate {
 public:
  FunctionListController(Escher::Responder* parentResponder,
                         Escher::ButtonRowController* header,
                         Escher::ButtonRowController* footer,
                         I18n::Message text);

  TELEMETRY_ID("List");

  /* ButtonRowDelegate */
  int numberOfButtons(
      Escher::ButtonRowController::Position position) const override;
  Escher::AbstractButtonCell* buttonAtIndex(
      int index, Escher::ButtonRowController::Position position) const override;

  /* Responder */
  void didEnterResponderChain(Escher::Responder* nextFirstResponder) override;
  void willExitResponderChain(Escher::Responder* nextFirstResponder) override;
  void didBecomeFirstResponder() override;

  /* ViewController */
  Escher::View* view() override { return &m_selectableListView; }
  ViewController::TitlesDisplay titlesDisplay() override {
    return TitlesDisplay::NeverDisplayOwnTitle;
  }

  /* ExpressionModelListController */
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  Escher::SelectableListView* selectableListView() override {
    return &m_selectableListView;
  }

 protected:
  Escher::StackViewController* stackController() const;
  void configureFunction(Ion::Storage::Record record);
  Escher::TabViewController* tabController() const;

 private:
  virtual ListParameterController* parameterController() = 0;
  virtual int maxNumberOfDisplayableRows() const = 0;
  virtual Escher::HighlightCell* functionCells(int index) = 0;
  Escher::SelectableListView m_selectableListView;
  Escher::AbstractButtonCell m_plotButton;
  Escher::AbstractButtonCell m_valuesButton;
};

}  // namespace Shared

#endif
