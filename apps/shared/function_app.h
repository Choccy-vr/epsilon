#ifndef SHARED_FUNCTION_APP_H
#define SHARED_FUNCTION_APP_H

#include <escher/alternate_empty_view_controller.h>
#include <escher/input_view_controller.h>
#include <escher/tab_union_view_controller.h>
#include <escher/tab_view_data_source.h>

#include "curve_view_cursor.h"
#include "function_graph_controller.h"
#include "function_list_controller.h"
#include "function_store.h"
#include "math_app.h"
#include "values_controller.h"

namespace Shared {

class FunctionApp : public MathApp {
 public:
  class Snapshot : public SharedApp::Snapshot,
                   public Escher::TabViewDataSource {
   public:
    Snapshot();
    CurveViewCursor *cursor() { return &m_cursor; }
    virtual FunctionStore *functionStore() = 0;
    int *selectedCurveIndex() { return &m_selectedCurveIndex; }
    void reset() override;

   private:
    CurveViewCursor m_cursor;
    int m_selectedCurveIndex;
  };
  static FunctionApp *app() {
    return static_cast<FunctionApp *>(Escher::App::app());
  }
  virtual ~FunctionApp() = default;
  Snapshot *snapshot() const {
    return static_cast<Snapshot *>(Escher::App::snapshot());
  }

  virtual FunctionStore *functionStore() const {
    return snapshot()->functionStore();
  }
  virtual ValuesController *valuesController() = 0;

  bool storageCanChangeForRecordName(
      const Ion::Storage::Record::Name recordName) const override;
  void prepareForIntrusiveStorageChange() override;
  void concludeIntrusiveStorageChange() override;

 protected:
  FunctionApp(Snapshot *snapshot, Escher::AbstractTabUnion *tabs,
              I18n::Message firstTabName);

  struct ListTab : public Escher::Tab {
    ListTab(Shared::FunctionListController *listController);
    Escher::ViewController *top() override {
      return &m_listStackViewController;
    }
    Escher::ButtonRowController m_listFooter;
    Escher::ButtonRowController m_listHeader;
    Escher::StackViewController::Default m_listStackViewController;
  };
  struct GraphTab : public Escher::Tab {
    static constexpr I18n::Message k_title = I18n::Message::GraphTab;
    GraphTab(Shared::FunctionGraphController *graphController);
    Escher::ViewController *top() override {
      return &m_graphStackViewController;
    }
    Escher::AlternateEmptyViewController m_graphAlternateEmptyViewController;
    Escher::ButtonRowController m_graphHeader;
    Escher::StackViewController::Default m_graphStackViewController;
  };
  struct ValuesTab : public Escher::Tab {
    static constexpr I18n::Message k_title = I18n::Message::ValuesTab;
    ValuesTab(Shared::ValuesController *valuesController);
    Escher::ViewController *top() override {
      return &m_valuesStackViewController;
    }
    Escher::AlternateEmptyViewController m_valuesAlternateEmptyViewController;
    Escher::ButtonRowController m_valuesHeader;
    Escher::StackViewController::Default m_valuesStackViewController;
  };

  Escher::TabUnionViewController m_tabViewController;
  Escher::ViewController *m_activeControllerBeforeStore;
};

}  // namespace Shared

#endif
