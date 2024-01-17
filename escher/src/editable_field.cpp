#include <escher/container.h>
#include <escher/editable_field.h>
#include <escher/metric.h>
#include <escher/toolbox.h>
#include <poincare/serialization_helper.h>
#include <poincare/symbol.h>
#include <poincare/xnt_helpers.h>

using namespace Poincare;

namespace Escher {

bool EditableField::privateHandleBoxEvent(Ion::Events::Event event) {
  App* app = App::app();
  EditableFieldHelpBox* box = nullptr;
  if (event == Ion::Events::Toolbox) {
    box = app->toolbox();
  } else if (event == Ion::Events::Var) {
    box = app->variableBox();
  }
  if (!box) {
    return false;
  }
  box->setSender(this);
  box->open();
  return true;
}

bool EditableField::handleXNT(int currentIndex, CodePoint startingXNT) {
  if (!prepareToEdit()) {
    return false;
  }
  constexpr int bufferSize = SymbolAbstractNode::k_maxNameSize;
  char buffer[bufferSize];
  size_t cycleSize;
  // Find special XNT
  if (!findXNT(buffer, bufferSize, &cycleSize)) {
    // Use default XNT cycle
    CodePoint xnt = XNTHelpers::CodePointAtIndexInDefaultCycle(
        currentIndex, startingXNT, &cycleSize);
    SerializationHelper::CodePoint(buffer, bufferSize, xnt);
  }
  if (cycleSize > 1 && currentIndex > 0) {
    removePreviousXNT();
  }
  return handleEventWithText(buffer, false, true);
}

}  // namespace Escher
