#ifndef CALCULATION_ADDITIONAL_OUTPUTS_COMPLEX_LIST_CONTROLLER_H
#define CALCULATION_ADDITIONAL_OUTPUTS_COMPLEX_LIST_CONTROLLER_H

#include "complex_graph_cell.h"
#include "complex_model.h"
#include "illustrated_list_controller.h"

namespace Calculation {

class ComplexListController : public IllustratedListController {
public:
  ComplexListController(EditExpressionController * editExpressionController) :
    IllustratedListController(editExpressionController),
    m_complexGraphCell(&m_model) {}

  // ViewController
  void viewWillAppear() override;

  void setExpression(Poincare::Expression e) override;
private:
  static constexpr char k_symbol[] = "z";
  const char * symbol() const override { return k_symbol; }
  Escher::HighlightCell * illustrationCell() override { return &m_complexGraphCell; }
  ComplexGraphCell m_complexGraphCell;
  ComplexModel m_model;
};

}

#endif

