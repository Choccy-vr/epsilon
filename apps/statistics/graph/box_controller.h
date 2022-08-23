#ifndef STATISTICS_BOX_CONTROLLER_H
#define STATISTICS_BOX_CONTROLLER_H

#include <escher/button_row_controller.h>
#include "../store.h"
#include "multiple_boxes_view.h"
#include "multiple_data_view_controller.h"
#include "box_parameter_controller.h"

namespace Statistics {

class BoxController : public MultipleDataViewController {
public:
  BoxController(Escher::Responder * parentResponder,
                Escher::ButtonRowController * header,
                Escher::Responder * tabController,
                Escher::StackViewController * stackViewController,
                Escher::ViewController * typeViewController,
                Store * store);

  BoxParameterController * boxParameterController() { return &m_boxParameterController; }

  // ButtonRowDelegate
  int numberOfButtons(Escher::ButtonRowController::Position position) const override { return GraphButtonRowDelegate::numberOfButtons(position) + 1; }
  Escher::Button * buttonAtIndex(int index, Escher::ButtonRowController::Position position) const override;

  // MultipleDataViewController
  MultipleDataView * multipleDataView() override { return &m_view; }
  bool moveSelectionHorizontally(int deltaIndex) override;

  // Responder
  bool handleEvent(Ion::Events::Event event) override;
  void didEnterResponderChain(Responder * firstResponder) override;
  void willExitResponderChain(Escher::Responder * nextFirstResponder) override;

  TELEMETRY_ID("Box");
private:
  bool reloadBannerView() override;
  MultipleBoxesView m_view;
  BoxParameterController m_boxParameterController;
  Escher::Button m_parameterButton;
};

}


#endif
