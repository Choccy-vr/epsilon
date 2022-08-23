#ifndef STATISTICS_GRAPH_BUTTON_ROW_DELEGATE_H
#define STATISTICS_GRAPH_BUTTON_ROW_DELEGATE_H

#include <apps/i18n.h>
#include <escher/button.h>
#include <escher/button_row_controller.h>
#include <escher/invocation.h>
#include <escher/stack_view_controller.h>

namespace Statistics {

/* This class handle the basic functions expected for a Statistics graph sub
 * controller view :
 * - ButtonRowDelegate common functions, type selector button
 * - Stack controller
 */
class GraphButtonRowDelegate : public Escher::ButtonRowDelegate {
public:
  GraphButtonRowDelegate(Escher::ButtonRowController * header,
                         Escher::StackViewController * stackViewController,
                         Escher::Responder * typeButtonParentResponder,
                         Escher::ViewController * typeViewController) :
      Escher::ButtonRowDelegate(header, nullptr),
      m_stackViewController(stackViewController),
      m_typeViewController(typeViewController),
      m_typeButton(typeButtonParentResponder, I18n::Message::StatisticsGraphType,
                   Escher::Invocation(
                       [](void * context, void * sender) {
                         GraphButtonRowDelegate * delegate = static_cast<GraphButtonRowDelegate *>(context);
                         delegate->pushTypeController();
                         return true;
                       }, this), KDFont::SmallFont)
  {}

  Escher::StackViewController * stackController() {
    return m_stackViewController;
  }

  // ButtonRowDelegate
  int numberOfButtons(Escher::ButtonRowController::Position position) const override {
    assert(position == Escher::ButtonRowController::Position::Top);
    return 1;
  }
  Escher::Button * buttonAtIndex(int index, Escher::ButtonRowController::Position position) const override {
    assert(index == 0 && position == Escher::ButtonRowController::Position::Top);
    return const_cast<Escher::Button *>(&m_typeButton);
  }

private:
  void pushTypeController() { stackController()->push(m_typeViewController); }

  Escher::StackViewController * m_stackViewController;
  Escher::ViewController * m_typeViewController;
  Escher::Button m_typeButton;
};

}  // namespace Statistics

#endif /* STATISTICS_GRAPH_BUTTON_ROW_DELEGATE_H */
