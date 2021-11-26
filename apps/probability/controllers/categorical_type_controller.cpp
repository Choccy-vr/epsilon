#include "categorical_type_controller.h"

#include <apps/i18n.h>
#include <escher/stack_view_controller.h>

#include "input_goodness_controller.h"
#include "input_homogeneity_controller.h"
#include "probability/app.h"
#include "probability/gui/selectable_cell_list_controller.h"

using namespace Probability;

CategoricalTypeController::CategoricalTypeController(
    Escher::StackViewController * parent,
    Chi2Statistic * statistic,
    Data::CategoricalType * globalCategoricalType,
    InputGoodnessController * inputGoodnessController,
    InputHomogeneityController * inputHomogeneityController) :
    SelectableCellListPage(parent),
    m_statistic(statistic),
    m_globalCategoricalType(globalCategoricalType),
    m_inputGoodnessController(inputGoodnessController),
    m_inputHomogeneityController(inputHomogeneityController) {
  selectRow(0);  // Select first row by default
  m_cells[k_indexOfGoodnessCell].setMessage(I18n::Message::GoodnessOfFit);
  m_cells[k_indexOfHomogeneityCell].setMessage(I18n::Message::Homogeneity);
}

void CategoricalTypeController::didBecomeFirstResponder() {
  Probability::App::app()->setPage(Data::Page::Categorical);
  Escher::Container::activeApp()->setFirstResponder(&m_selectableTableView);
}

bool CategoricalTypeController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE || event == Ion::Events::Right) {
    Escher::ViewController * view;
    Data::CategoricalType type;
    switch (selectedRow()) {
      case k_indexOfGoodnessCell:
        view = m_inputGoodnessController;
        type = Data::CategoricalType::Goodness;
        break;
      case k_indexOfHomogeneityCell:
        view = m_inputHomogeneityController;
        type = Data::CategoricalType::Homogeneity;
        break;
    }
    assert(view != nullptr);
    if (type != App::app()->categoricalType()) {
      *m_globalCategoricalType = type;
      Statistic::initializeStatistic(m_statistic,
                                     App::app()->test(),
                                     App::app()->testType(),
                                     App::app()->categoricalType());
    }
    openPage(view);
    return true;
  } else if (event == Ion::Events::Left) {
    stackViewController()->pop();
    return true;
  }
  return false;
}
