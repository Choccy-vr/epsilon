#ifndef SOLVER_INTERVAL_CONTROLLER_H
#define SOLVER_INTERVAL_CONTROLLER_H

#include <escher/message_table_cell_with_editable_text.h>
#include "equation_store.h"
#include <apps/shared/float_parameter_controller.h>

namespace Solver {

class IntervalController : public Shared::FloatParameterController<double> {
public:
  IntervalController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, EquationStore * equationStore);
  const char * title() override;
  Escher::View * view() override { return &m_contentView; }
  TELEMETRY_ID("Interval");
  int numberOfRows() const override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  void setShouldReplaceFuncionsButNotSymbols(bool shouldReplaceFunctionsButNotSymbols) { m_shouldReplaceFunctionsButNotSymbols = shouldReplaceFunctionsButNotSymbols; }
private:
  Escher::HighlightCell * reusableParameterCell(int index, int type) override;
  int reusableParameterCellCount(int type) override;
  void buttonAction() override;
  double parameterAtIndex(int index) override;
  bool setParameterAtIndex(int parameterIndex, double f) override;
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;
  class ContentView : public Escher::View {
  public:
    ContentView(Escher::SelectableTableView * selectableTableView);
    void drawRect(KDContext * ctx, KDRect rect) const override;
  private:
    constexpr static KDCoordinate k_topMargin = 50;
    int numberOfSubviews() const override;
    Escher::View * subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    Escher::MessageTextView m_instructions0;
    Escher::MessageTextView m_instructions1;
    Escher::SelectableTableView * m_selectableTableView;
  };
  ContentView m_contentView;
  constexpr static int k_maxNumberOfCells = 2;
  Escher::MessageTableCellWithEditableText m_intervalCell[k_maxNumberOfCells];
  EquationStore * m_equationStore;
  bool m_shouldReplaceFunctionsButNotSymbols;
};

}

#endif
