#ifndef REGRESSION_STORE_CONTROLLER_H
#define REGRESSION_STORE_CONTROLLER_H

#include "store.h"
#include "regression_context.h"
#include "store_parameter_controller.h"
#include "../shared/store_controller.h"

namespace Regression {

class StoreController : public Shared::StoreController {
public:
  StoreController(Escher::Responder * parentResponder, Escher::InputEventHandlerDelegate * inputEventHandlerDelegate, Store * store, Escher::ButtonRowController * header, Poincare::Context * parentContext);
  Shared::StoreContext * storeContext() override { return &m_regressionContext; }
  bool fillColumnWithFormula(Poincare::Expression formula) override;
  Model * model() { return static_cast<Store *>(m_store)->modelForSeries(selectedSeries()); }
  void fillColumnName(int columnIndex, char * buffer) override;

private:
  Shared::ColumnParameterController * columnParameterController() override { return &m_storeParameterController; }
  RegressionContext m_regressionContext;
  StoreParameterController m_storeParameterController;

};

}

#endif
