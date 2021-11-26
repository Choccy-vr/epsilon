#ifndef APPS_PROBABILITY_CONTROLLERS_PARAMETERS_CONTROLLER_H
#define APPS_PROBABILITY_CONTROLLERS_PARAMETERS_CONTROLLER_H

#include <escher/message_table_cell_with_editable_text.h>
#include <apps/shared/float_parameter_controller.h>
#include "probability/models/distribution/distribution.h"
#include "calculation_controller.h"

namespace Probability {

class ParametersController : public Shared::FloatParameterController<double> {
public:
  ParametersController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Distribution * m_distribution, CalculationController * calculationController);
  const char * title() override;
  Escher::View * view() override { return &m_contentView; }
  bool handleEvent(Ion::Events::Event event) override;
  void reinitCalculation();
  void didBecomeFirstResponder() override;
  void viewWillAppear() override;
  TELEMETRY_ID("Parameters");
  int numberOfRows() const override;
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
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
    Escher::MessageTextView * parameterDefinitionAtIndex(int index);
    void layoutSubviews(bool force = false) override;
    void setNumberOfParameters(int numberOfParameters);
  private:
    constexpr static KDCoordinate k_textMargin = Escher::Metric::CommonSmallMargin;
    // Removing a pixel to skew title's baseline downward.
    constexpr static KDCoordinate k_titleMargin = Escher::Metric::CommonTopMargin - 1;
    int numberOfSubviews() const override;
    Escher::View * subviewAtIndex(int index) override;
    int m_numberOfParameters;
    Escher::MessageTextView m_titleView;
    Escher::MessageTextView m_firstParameterDefinition;
    Escher::MessageTextView m_secondParameterDefinition;
    Escher::SelectableTableView * m_selectableTableView;
  };
  constexpr static int k_maxNumberOfCells = 2;
  ContentView m_contentView;
  Escher::MessageTableCellWithEditableText m_menuListCell[k_maxNumberOfCells];
  Distribution * m_distribution;
  CalculationController * m_calculationController;
};

}

#endif /* APPS_PROBABILITY_CONTROLLERS_PARAMETERS_CONTROLLER_H */
