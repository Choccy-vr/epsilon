#ifndef STATISTICS_BOX_CONTROLLER_H
#define STATISTICS_BOX_CONTROLLER_H

#include <escher/button_row_controller.h>

#include "../store.h"
#include "box_parameter_controller.h"
#include "multiple_boxes_view.h"
#include "multiple_data_view_controller.h"

namespace Statistics {

class BoxController : public MultipleDataViewController {
 public:
  BoxController(Escher::Responder* parentResponder,
                Escher::ButtonRowController* header,
                Escher::TabViewController* tabController,
                Escher::StackViewController* stackViewController,
                Escher::ViewController* typeViewController, Store* store);

  BoxParameterController* boxParameterController() {
    return &m_boxParameterController;
  }

  // ButtonRowDelegate
  int numberOfButtons(
      Escher::ButtonRowController::Position position) const override {
    return GraphButtonRowDelegate::numberOfButtons(position) + 1;
  }
  Escher::ButtonCell* buttonAtIndex(
      int index, Escher::ButtonRowController::Position position) const override;

  // MultipleDataViewController
  MultipleDataView* multipleDataView() override { return &m_view; }
  bool moveSelectionHorizontally(OMG::HorizontalDirection direction) override;
  /* Reset the selected calculation index in case modifying the series made
   * this calculation disappear. */
  void viewWillAppearBeforeReload() override { setSelectedIndex(0); }

  // Responder
  bool handleEvent(Ion::Events::Event event) override;

  TELEMETRY_ID("Box");

 private:
  bool reloadBannerView() override;

  // DataViewController
  void updateHorizontalIndexAfterSelectingNewSeries(
      int previousSelectedSeries) override;

  MultipleBoxesView m_view;
  BoxParameterController m_boxParameterController;
  Escher::SimpleButtonCell m_parameterButton;
};

}  // namespace Statistics

#endif
