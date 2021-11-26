#ifndef SETTINGS_ABOUT_CONTROLLER_H
#define SETTINGS_ABOUT_CONTROLLER_H

#include <escher/message_table_cell_with_buffer.h>
#include "generic_sub_controller.h"
#include "selectable_view_with_messages.h"
#include "../../hardware_test/pop_up_controller.h"

namespace Settings {

class AboutController : public GenericSubController {
public:
  AboutController(Escher::Responder * parentResponder);
  TELEMETRY_ID("About");
  bool handleEvent(Ion::Events::Event event) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  KDCoordinate nonMemoizedRowHeight(int j) override;
private:
  constexpr static int k_totalNumberOfCell = 3;
  Escher::MessageTableCellWithBuffer m_cells[k_totalNumberOfCell];
  HardwareTest::PopUpController m_hardwareTestPopUpController;
};

}

#endif
