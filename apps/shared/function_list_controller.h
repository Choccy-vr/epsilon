#ifndef SHARED_FUNCTION_LIST_CONTROLLER_H
#define SHARED_FUNCTION_LIST_CONTROLLER_H

#include <escher/button_row_controller.h>
#include <escher/stack_view_controller.h>
#include <escher/tab_view_controller.h>
#include <escher/highlight_cell.h>
#include "list_parameter_controller.h"
#include "expression_model_list_controller.h"

namespace Shared {

class FunctionListController : public ExpressionModelListController, public Escher::ButtonRowDelegate {
public:
  FunctionListController(Escher::Responder * parentResponder, Escher::ButtonRowController * header, Escher::ButtonRowController * footer, I18n::Message text);

  TELEMETRY_ID("List");

  /* ButtonRowDelegate */
  int numberOfButtons(Escher::ButtonRowController::Position position) const override;
  Escher::Button * buttonAtIndex(int index, Escher::ButtonRowController::Position position) const override;

  /* Responder */
  void didEnterResponderChain(Escher::Responder * previousFirstResponder) override;
  void willExitResponderChain(Escher::Responder * nextFirstResponder) override;

  /* ViewController */
  ViewController::TitlesDisplay titlesDisplay() override { return TitlesDisplay::NeverDisplayOwnTitle; }
protected:
  Escher::StackViewController * stackController() const;
  void configureFunction(Ion::Storage::Record record);
  Escher::TabViewController * tabController() const;
private:
  Escher::InputViewController * inputController() override;
  virtual ListParameterController * parameterController() = 0;
  virtual int maxNumberOfDisplayableRows() = 0;
  virtual Escher::HighlightCell * functionCells(int index) = 0;
  Escher::Button m_plotButton;
  Escher::Button m_valuesButton;
};

}

#endif
