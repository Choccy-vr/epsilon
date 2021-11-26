#ifndef GRAPH_APP_H
#define GRAPH_APP_H

#include <escher/alternate_empty_view_controller.h>
#include "graph/graph_controller.h"
#include "list/list_controller.h"
#include "values/values_controller.h"
#include "../shared/continuous_function_store.h"
#include "../shared/new_function.h"
#include "../shared/function_app.h"
#include "../shared/interval.h"

namespace Graph {

class App : public Shared::FunctionApp {
public:
  class Descriptor : public Escher::App::Descriptor {
  public:
    I18n::Message name() const override;
    I18n::Message upperName() const override;
    const Escher::Image * icon() const override;
  };
  class Snapshot : public Shared::FunctionApp::Snapshot {
  public:
    Snapshot();
    App * unpack(Escher::Container * container) override;
    void reset() override;
    const Descriptor * descriptor() const override;
    Shared::ContinuousFunctionStore * functionStore() override { return &m_functionStore; }
    Shared::InteractiveCurveViewRange * graphRange() { return &m_graphRange; }
    Shared::Interval * intervalForType(Shared::NewFunction::PlotType plotType) {
      return m_interval + static_cast<size_t>(plotType);
    }
  private:
    void tidy() override;
    Shared::ContinuousFunctionStore m_functionStore;
    Shared::InteractiveCurveViewRange m_graphRange;
    Shared::Interval m_interval[Shared::NewFunction::k_numberOfPlotTypes]; // TODO Hugo : Have 1 interval per symbol, so back to three
  };
  static App * app() {
    return static_cast<App *>(Escher::Container::activeApp());
  }
  Snapshot * snapshot() const {
    return static_cast<Snapshot *>(Escher::App::snapshot());
  }
  TELEMETRY_ID("Graph");
  CodePoint XNT() override;
  Escher::NestedMenuController * variableBoxForInputEventHandler(Escher::InputEventHandler * textInput) override;
  Shared::ContinuousFunctionStore * functionStore() override { return snapshot()->functionStore(); }
  Shared::Interval * intervalForType(Shared::NewFunction::PlotType plotType) {
    return snapshot()->intervalForType(plotType);
  }
  ValuesController * valuesController() override {
    return &m_valuesController;
  }
  Escher::InputViewController * inputViewController() override {
    return &m_inputViewController;
  }
  bool isAcceptableExpression(const Poincare::Expression expression) override;
private:
  App(Snapshot * snapshot);
  ListController m_listController;
  Escher::ButtonRowController m_listFooter;
  Escher::ButtonRowController m_listHeader;
  Escher::StackViewController m_listStackViewController;
  GraphController m_graphController;
  Escher::AlternateEmptyViewController m_graphAlternateEmptyViewController;
  Escher::ButtonRowController m_graphHeader;
  Escher::StackViewController m_graphStackViewController;
  ValuesController m_valuesController;
  Escher::AlternateEmptyViewController m_valuesAlternateEmptyViewController;
  Escher::ButtonRowController m_valuesHeader;
  Escher::StackViewController m_valuesStackViewController;
  Escher::TabViewController m_tabViewController;
  Escher::InputViewController m_inputViewController;
};

}

#endif
