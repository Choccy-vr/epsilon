#ifndef GRAPH_CALCULATION_PARAMETER_CONTROLLER_H
#define GRAPH_CALCULATION_PARAMETER_CONTROLLER_H

#include <escher/message_table_cell_with_chevron.h>
#include <escher/selectable_list_view_controller.h>
#include "preimage_parameter_controller.h"
#include "tangent_graph_controller.h"
#include "extremum_graph_controller.h"
#include "integral_graph_controller.h"
#include "intersection_graph_controller.h"
#include "root_graph_controller.h"
#include "graph_view.h"
#include "banner_view.h"
#include <apps/i18n.h>

namespace Graph {

class CalculationParameterController : public Escher::SelectableListViewController<Escher::RegularListViewDataSource> {
public:
  CalculationParameterController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, GraphView * graphView, BannerView * bannerView, Shared::InteractiveCurveViewRange * range, Shared::CurveViewCursor * cursor);
  const char * title() override;
  bool handleEvent(Ion::Events::Event event) override;
  void viewWillAppear() override;
  void didBecomeFirstResponder() override;
  TELEMETRY_ID("CalculationParameter");
  int numberOfRows() const override;

  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtIndex(int index) override { return index == 0 ? k_preImageCellType : k_defaultCellType; }
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  void setRecord(Ion::Storage::Record record);
private:
  bool shouldDisplayIntersection() const;
  Escher::MessageTableCellWithChevron m_preimageCell;
  constexpr static int k_totalNumberOfReusableCells = 6;
  constexpr static int k_defaultCellType = 0;
  constexpr static int k_preImageCellType = 1;
  Escher::MessageTableCell m_cells[k_totalNumberOfReusableCells];
  Ion::Storage::Record m_record;
  PreimageParameterController m_preimageParameterController;
  PreimageGraphController m_preimageGraphController;
  TangentGraphController m_tangentGraphController;
  IntegralGraphController m_integralGraphController;
  MinimumGraphController m_minimumGraphController;
  MaximumGraphController m_maximumGraphController;
  RootGraphController m_rootGraphController;
  IntersectionGraphController m_intersectionGraphController;
};

}

#endif

