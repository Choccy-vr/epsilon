#ifndef SOLVER_APP_H
#define SOLVER_APP_H

#include <apps/i18n.h>
#include "../shared/expression_field_delegate_app.h"
#include "../shared/shared_app.h"
#include "model/equation_store.h"
#include "controllers/list_controller.h"
#include "controllers/interval_controller.h"
#include "controllers/solutions_controller.h"
#include "controllers/menu_controller.h"
#include "controllers/finance_controller.h"

namespace Solver {

class App : public Shared::ExpressionFieldDelegateApp {
public:
  // Descriptor
  class Descriptor : public Escher::App::Descriptor {
    public:
      I18n::Message name() const override { return I18n::Message::SolverApp; };
      I18n::Message upperName() const override { return I18n::Message::SolverAppCapital; };
      const Escher::Image * icon() const override;
  };

  // Snapshot
  class Snapshot : public Shared::SharedApp::Snapshot {
  public:
    Snapshot() : m_equationStore() {};
    App * unpack(Escher::Container * container) override {
      return new (container->currentAppBuffer()) App(this);
    };
    const Descriptor * descriptor() const override;
    void reset() override;
    EquationStore * equationStore() { return &m_equationStore; }
    void storageDidChangeForRecord(const Ion::Storage::Record record) override;
  private:
    void tidy() override;
    EquationStore m_equationStore;
  };

  static App * app() { return static_cast<App *>(Escher::Container::activeApp()); }
  Snapshot * snapshot() { return static_cast<Snapshot *>(Escher::App::snapshot()); }
  EquationStore * equationStore() { return snapshot()->equationStore(); }
  Escher::InputViewController * inputViewController() { return &m_inputViewController; }
  Escher::ViewController * solutionsControllerStack() { return &m_alternateEmptyViewController; }
  Escher::ViewController * intervalController() { return &m_intervalController; }
  SolutionsController * solutionsController() { return &m_solutionsController; }

  TELEMETRY_ID("Solver");

private:
  App(Snapshot * snapshot);
  // TextFieldDelegateApp
  bool isAcceptableExpression(const Poincare::Expression expression) override;

  // Controllers
  SolutionsController m_solutionsController;
  IntervalController m_intervalController;
  Escher::AlternateEmptyViewController m_alternateEmptyViewController;
  ListController m_listController;
  Escher::ButtonRowController m_listFooter;
  FinanceController m_financeController;
  MenuController m_menuController;
  Escher::StackViewController m_stackViewController;
  Escher::InputViewController m_inputViewController;
};

}  // namespace Solver

#endif /* SOLVER_APP_H */
