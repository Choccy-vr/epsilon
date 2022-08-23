#ifndef SHARED_FLOAT_PARAMETER_CONTROLLER_H
#define SHARED_FLOAT_PARAMETER_CONTROLLER_H

#include "button_with_separator.h"
#include "parameter_text_field_delegate.h"
#include <escher/selectable_list_view_controller.h>
#include <escher/stack_view_controller.h>

namespace Shared {

/* This controller edits float parameter of any model (given through
 * parameterAtIndex and setParameterAtIndex). */

template<typename T>
class FloatParameterController : public Escher::SelectableListViewController<Escher::MemoizedListViewDataSource>, public ParameterTextFieldDelegate {
public:
  FloatParameterController(Escher::Responder * parentResponder);
  void didBecomeFirstResponder() override;
  void viewWillAppear() override;
  void viewDidDisappear() override;
  bool handleEvent(Ion::Events::Event event) override;

  int typeAtIndex(int index) override;
  int reusableCellCount(int type) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  KDCoordinate nonMemoizedRowHeight(int j) override;
  bool textFieldShouldFinishEditing(Escher::TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;

protected:
  static constexpr int k_parameterCellType = 0;
  static constexpr int k_buttonCellType = 1;

  enum class InfinityTolerance {
    None,
    PlusInfinity,
    MinusInfinity
  };
  int activeCell() { return selectedRow(); }
  Escher::StackViewController * stackController() { return static_cast<Escher::StackViewController *>(parentResponder()); }
  virtual T parameterAtIndex(int index) = 0;
  virtual bool isCellEditing(Escher::HighlightCell * cell, int index);
  virtual void setTextInCell(Escher::HighlightCell * cell, const char * text, int index);
  virtual void buttonAction();

  ButtonWithSeparator m_okButton;

private:
  virtual InfinityTolerance infinityAllowanceForRow(int row) const { return InfinityTolerance::None; }
  virtual int reusableParameterCellCount(int type) = 0;
  virtual Escher::HighlightCell * reusableParameterCell(int index, int type) = 0;
  virtual bool setParameterAtIndex(int parameterIndex, T f) = 0;
};

}

#endif
