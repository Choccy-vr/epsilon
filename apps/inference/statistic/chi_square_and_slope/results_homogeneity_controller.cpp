#include "results_homogeneity_controller.h"

namespace Inference {

// ResultsHomogeneityController

ResultsHomogeneityController::ResultsHomogeneityController(Escher::StackViewController * parent, Escher::ViewController * nextController, HomogeneityTest * statistic) :
  TabViewController(parent, this, &m_expectedValuesController, &m_contributionsController, nullptr),
  m_tableController(nextController, statistic),
  m_expectedValuesController(this, &m_tableController),
  m_contributionsController(this, &m_tableController)
{
  TabViewController::initView();
}

// ResultsTableController

ResultsHomogeneityController::ResultsTableController::ResultsTableController(Escher::ViewController * resultsController, HomogeneityTest * statistic) :
  CategoricalController(nullptr, resultsController, Invocation(&CategoricalController::ButtonAction, this)),
  m_resultHomogeneityTable(&m_selectableTableView, this, statistic)
{}

void ResultsHomogeneityController::ResultsTableController::viewWillAppear() {
  m_selectableTableView.reloadData(false, false);
  CategoricalController::viewWillAppear();
}

bool ResultsHomogeneityController::ResultsTableController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::Up) {
    m_resultHomogeneityTable.tableView()->deselectTable();
    Escher::Container::activeApp()->setFirstResponder(tabController());
    return true;
  }
  return false;
}

void ResultsHomogeneityController::ResultsTableController::tableViewDidChangeSelection(SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) {
  if (m_resultHomogeneityTable.unselectTopLeftCell(t, previousSelectedCellX, previousSelectedCellY) && t->selectedColumn() == 0) {
    m_resultHomogeneityTable.tableView()->deselectTable();
    Escher::Container::activeApp()->setFirstResponder(tabController());
  }
}

// SingleModeController

void ResultsHomogeneityController::SingleModeController::switchToTableWithMode(ResultHomogeneityTableCell::Mode mode) {
  m_tableController->setMode(mode);
  m_tableController->setParentResponder(this);
}

}
