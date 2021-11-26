#ifndef PROBABILITY_ABSTRACT_INPUT_CATEGORICAL_CONTROLLER_H
#define PROBABILITY_ABSTRACT_INPUT_CATEGORICAL_CONTROLLER_H

#include <escher/selectable_table_view.h>

#include "button_delegate.h"
#include "input_categorical_view.h"
#include "probability/gui/page_controller.h"
#include "probability/models/statistic/chi2_statistic.h"

namespace Probability {

/* This is the common Controller between Homogeneity and Goodness input controllers. It parses
 * significance level input and own the content view. */
class InputCategoricalController : public Page,
                                   public Shared::ParameterTextFieldDelegate,
                                   public ButtonDelegate,
                                   public Escher::SelectableTableViewDelegate {
public:
  InputCategoricalController(Escher::StackViewController * parent,
                             Page * resultsController,
                             Chi2Statistic * statistic,
                             Escher::InputEventHandlerDelegate * inputEventHandlerDelegate);

  virtual TableViewController * tableViewController() = 0;

  // TextFieldDelegate
  bool textFieldShouldFinishEditing(Escher::TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(Escher::TextField * textField,
                                 const char * text,
                                 Ion::Events::Event event) override;

  // Responder
  void didEnterResponderChain(Responder * previousResponder) override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  // ButtonDelegate
  bool buttonAction() override;

  // ViewController
  Escher::View * view() override { return &m_contentView; }
  Escher::ViewController::TitlesDisplay titlesDisplay() override {
    return Escher::ViewController::TitlesDisplay::DisplayLastTitle;
  }

  // SelectableTableViewDelegate
  void tableViewDidChangeSelectionAndDidScroll(Escher::SelectableTableView * t,
                                               int previousSelectedCellX,
                                               int previousSelectedCellY,
                                               bool withinTemporarySelection) override;

protected:
  Chi2Statistic * m_statistic;
  Page * m_resultsController;
  InputCategoricalView m_contentView;
};

}  // namespace Probability

#endif /* PROBABILITY_ABSTRACT_INPUT_CATEGORICAL_CONTROLLER_H */
