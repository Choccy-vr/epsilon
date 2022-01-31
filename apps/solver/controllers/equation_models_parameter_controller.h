#ifndef SOLVER_EQUATION_MODELS_PARAMETER_CONTROLLER_H
#define SOLVER_EQUATION_MODELS_PARAMETER_CONTROLLER_H

#include <escher/selectable_list_view_controller.h>
#include <escher/expression_table_cell.h>
#include <escher/message_table_cell.h>
#include <escher/stack_view_controller.h>
#include "../model/equation_store.h"

namespace Solver {

class ListController;

class EquationModelsParameterController : public Escher::SelectableListViewController {
public:
  EquationModelsParameterController(Escher::Responder * parentResponder, EquationStore * equationStore, ListController * listController);
  const char * title() override;
  void viewWillAppear() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override;
  KDCoordinate nonMemoizedRowHeight(int j) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtIndex(int index) override { return index == 0 ? k_emptyModelCellType : k_modelCellType; }
private:
  constexpr static int k_emptyModelCellType = 0;
  constexpr static int k_modelCellType = 1;
  constexpr static int k_numberOfModels = 6;
  static constexpr const char * k_models[k_numberOfModels] = {
    "", "x+y=0", "x^2+x+1=0", "x+y+z=0", "x^3+x^2+x+1=0", "x+y+z+t=0"
  };
  Escher::StackViewController * stackController() const;
  constexpr static int k_numberOfExpressionCells = k_numberOfModels-1;
  Escher::MessageTableCell m_emptyModelCell;
  Escher::ExpressionTableCell m_modelCells[k_numberOfExpressionCells];
  Poincare::Layout m_layouts[k_numberOfExpressionCells];
  EquationStore * m_equationStore;
  ListController * m_listController;
};

}

#endif
