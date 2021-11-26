#include "simple_interactive_curve_view_controller.h"
#include <escher/text_field.h>
#include <cmath>
#include <assert.h>

using namespace Poincare;
using namespace Escher;

namespace Shared {

bool SimpleInteractiveCurveViewController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Left || event == Ion::Events::Right) {
    return handleLeftRightEvent(event);
  }
  return ZoomCurveViewController::handleEvent(event);
}

bool SimpleInteractiveCurveViewController::textFieldDidReceiveEvent(TextField * textField, Ion::Events::Event event) {
  if ((event == Ion::Events::OK || event == Ion::Events::EXE) && !textField->isEditing()) {
    return handleEnter();
  }
  return TextFieldDelegate::textFieldDidReceiveEvent(textField, event);
}

bool SimpleInteractiveCurveViewController::handleLeftRightEvent(Ion::Events::Event event) {
  int direction = event == Ion::Events::Left ? -1 : 1;
  if (moveCursorHorizontally(direction, Ion::Events::repetitionFactor())) {
    reloadBannerView();
    bool moved = interactiveCurveViewRange()->panToMakePointVisible(
      m_cursor->x(), m_cursor->y(),
      cursorTopMarginRatio(), cursorRightMarginRatio(), cursorBottomMarginRatio(), cursorLeftMarginRatio(),
      curveView()->pixelWidth()
    );
    /* Restart drawing of interrupted curves when the window pans. */
    curveView()->reload(moved);
    return true;
  }
  return false;
}

}
