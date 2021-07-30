#ifndef GRAPH_GRAPH_CURVE_PARAMETER_CONTROLLER_H
#define GRAPH_GRAPH_CURVE_PARAMETER_CONTROLLER_H

#include <escher/message_table_cell_with_chevron.h>
#include <escher/message_table_cell_with_switch.h>
#include "../../shared/function_curve_parameter_controller.h"
#include "calculation_parameter_controller.h"
#include "banner_view.h"

namespace Graph {

class GraphController;

class CurveParameterController : public Shared::FunctionCurveParameterController {
public:
  CurveParameterController(Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Shared::InteractiveCurveViewRange * graphRange, BannerView * bannerView, Shared::CurveViewCursor * cursor, GraphView * graphView, GraphController * graphController);
  const char * title() override;
  bool handleEvent(Ion::Events::Event event) override;
  int numberOfRows() const override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  void viewWillAppear() override;
private:
  bool shouldDisplayCalculationAndDerivative() const;
  int cellIndex(int visibleCellIndex) const;
  Shared::FunctionGoToParameterController * goToParameterController() override;
  Shared::FunctionGoToParameterController m_goToParameterController;
  GraphController * m_graphController;
  Escher::MessageTableCellWithChevron m_calculationCell;
  Escher::MessageTableCellWithSwitch m_derivativeCell;
  CalculationParameterController m_calculationParameterController;
};

}

#endif
