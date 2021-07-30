#ifndef SEQUENCE_APP_H
#define SEQUENCE_APP_H

#include <escher/alternate_empty_view_controller.h>
#include "../shared/sequence_context.h"
#include "../shared/sequence_store.h"
#include "graph/graph_controller.h"
#include "graph/curve_view_range.h"
#include "list/list_controller.h"
#include "values/values_controller.h"
#include "../shared/function_app.h"
#include "../shared/interval.h"
#include "../shared/global_context.h"
#include "../apps_container.h"

namespace Sequence {

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
    Shared::SequenceStore * functionStore() override { return static_cast<Shared::GlobalContext *>(AppsContainer::sharedAppsContainer()->globalContext())->sequenceStore(); }
    CurveViewRange * graphRange() { return &m_graphRange; }
    Shared::Interval * interval() { return &m_interval; }
  private:
    void tidy() override;
    CurveViewRange m_graphRange;
    Shared::Interval m_interval;
  };
  static App * app() {
    return static_cast<App *>(Escher::Container::activeApp());
  }
  Snapshot * snapshot() const {
    return static_cast<Snapshot *>(Escher::App::snapshot());
  }
  TELEMETRY_ID("Sequence");
  // TODO: override variableBoxForInputEventHandler to lock sequence in the variable box once they appear there
  // NestedMenuController * variableBoxForInputEventHandler(InputEventHandler * textInput) override;
  CodePoint XNT() override { return 'n'; }
  Escher::NestedMenuController * variableBoxForInputEventHandler(Escher::InputEventHandler * textInput) override;
  Shared::SequenceContext * localContext() override;
  Shared::SequenceStore * functionStore() override { return static_cast<Shared::GlobalContext *>(AppsContainer::sharedAppsContainer()->globalContext())->sequenceStore(); }
  Shared::Interval * interval() { return snapshot()->interval(); }
  ValuesController * valuesController() override {
    return &m_valuesController;
  }
  Escher::InputViewController * inputViewController() override {
    return &m_inputViewController;
  }
private:
  App(Snapshot * snapshot);
  Shared::SequenceContext m_sequenceContext;
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
