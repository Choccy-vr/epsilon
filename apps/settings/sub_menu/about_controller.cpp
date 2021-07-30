#include "about_controller.h"
#include <escher/container.h>
#include <assert.h>
#include <cmath>

using namespace Escher;

namespace Settings {

AboutController::AboutController(Responder * parentResponder) :
  GenericSubController(parentResponder),
  m_view(&m_selectableTableView)
{
  for (int i = 0; i < k_totalNumberOfCell; i++) {
    m_cells[i].setMessageFont(KDFont::LargeFont);
    m_cells[i].setAccessoryFont(KDFont::SmallFont);
    m_cells[i].setAccessoryTextColor(Palette::GrayDark);
  }
}

bool AboutController::handleEvent(Ion::Events::Event event) {
  /* We hide here the activation hardware test app: in the menu "about", by
   * clicking on '6' on the last row. */
  if ((event == Ion::Events::Six || event == Ion::Events::LowerT || event == Ion::Events::UpperT) && m_messageTreeModel->label() == I18n::Message::About && selectedRow() == numberOfRows()-1) {
    Container::activeApp()->displayModalViewController(&m_hardwareTestPopUpController, 0.f, 0.f, Metric::PopUpTopMargin, Metric::PopUpRightMargin, Metric::PopUpBottomMargin, Metric::PopUpLeftMargin);
    return true;
  }
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    if (selectedRow() == 0) {
      MessageTableCellWithBuffer * myCell = (MessageTableCellWithBuffer *)m_selectableTableView.selectedCell();
      if (strcmp(myCell->accessoryText(), Ion::patchLevel()) == 0) {
        myCell->setAccessoryText(Ion::epsilonVersion());
        return true;
      }
      myCell->setAccessoryText(Ion::patchLevel());
      return true;
    }
    return false;
  }
  return GenericSubController::handleEvent(event);
}

HighlightCell * AboutController::reusableCell(int index, int type) {
  assert(type == 0);
  assert(index >= 0 && index < k_totalNumberOfCell);
  return &m_cells[index];
}

int AboutController::reusableCellCount(int type) {
  assert(type == 0);
  return k_totalNumberOfCell;
}

void AboutController::willDisplayCellForIndex(HighlightCell * cell, int index) {
  GenericSubController::willDisplayCellForIndex(cell, index);
  MessageTableCellWithBuffer * myCell = (MessageTableCellWithBuffer *)cell;
  static const char * version = Ion::epsilonVersion();
  static char serialNumberBuffer[Ion::SerialNumberLength+1] = {0};
  if (serialNumberBuffer[0] == 0) {
    Ion::serialNumber(serialNumberBuffer);
  }
  static char fccIDBuffer[Ion::FccIDLength+1] = {0};
  if (fccIDBuffer[0] == 0) {
    Ion::fccId(fccIDBuffer);
  }
  const char * messages[] = {
    version,
    serialNumberBuffer,
    fccIDBuffer
  };
  assert(index >= 0 && index < 3);
  myCell->setAccessoryText(messages[index]);
}

}
