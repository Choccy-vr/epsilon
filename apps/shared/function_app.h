#ifndef SHARED_FUNCTION_APP_H
#define SHARED_FUNCTION_APP_H

#include <escher/input_view_controller.h>
#include <escher/tab_view_data_source.h>
#include "expression_field_delegate_app.h"
#include "function_graph_controller.h"
#include "function_store.h"
#include "curve_view_cursor.h"
#include "values_controller.h"
#include "shared_app.h"

namespace Shared {

class FunctionApp : public ExpressionFieldDelegateApp {
public:
  class Snapshot : public Shared::SharedApp::Snapshot, public Escher::TabViewDataSource {
  public:
    Snapshot();
    CurveViewCursor * cursor() { return &m_cursor; }
    uint32_t * rangeVersion() { return &m_rangeVersion; }
    virtual FunctionStore * functionStore() = 0;
    int * indexFunctionSelectedByCursor() { return &m_indexFunctionSelectedByCursor; }
    void reset() override;
    void storageDidChangeForRecord(const Ion::Storage::Record record) override;
  protected:
    CurveViewCursor m_cursor;
  private:
    int m_indexFunctionSelectedByCursor;
    uint32_t m_rangeVersion;
  };
  static FunctionApp * app() {
    return static_cast<FunctionApp *>(Escher::Container::activeApp());
  }
  virtual ~FunctionApp() = default;
  Snapshot * snapshot() const {
    return static_cast<Snapshot *>(Escher::App::snapshot());
  }
  virtual FunctionStore * functionStore() { return snapshot()->functionStore(); }
  virtual ValuesController * valuesController() = 0;
  virtual Escher::InputViewController * inputViewController() = 0;
  void willBecomeInactive() override;

protected:
  FunctionApp(Snapshot * snapshot, Escher::ViewController * rootViewController) :
    ExpressionFieldDelegateApp(snapshot, rootViewController)
  {}
};

}

#endif
