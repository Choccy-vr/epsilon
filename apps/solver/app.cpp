#include "app.h"
#include <apps/i18n.h>
#include "solver_icon.h"

using namespace Shared;
using namespace Escher;

namespace Solver {

I18n::Message App::Descriptor::name() const {
  return I18n::Message::SolverApp;
}

I18n::Message App::Descriptor::upperName() const {
  return I18n::Message::SolverAppCapital;
}

const Image * App::Descriptor::icon() const {
  return ImageStore::SolverIcon;
}

App::Snapshot::Snapshot() :
  m_equationStore()
{
}

App * App::Snapshot::unpack(Container * container) {
  return new (container->currentAppBuffer()) App(this);
}

static constexpr App::Descriptor sDescriptor;

const App::Descriptor * App::Snapshot::descriptor() const {
  return &sDescriptor;
}

void App::Snapshot::reset() {
  // Delete all equations
  m_equationStore.removeAll();
}

void App::Snapshot::storageDidChangeForRecord(const Ion::Storage::Record record) {
  m_equationStore.storageDidChangeForRecord(record);
}

void App::Snapshot::tidy() {
  // Delete all expressions of equations
  m_equationStore.tidy();
}

App::App(Snapshot * snapshot) :
  ExpressionFieldDelegateApp(snapshot, &m_inputViewController),
  m_solutionsController(&m_alternateEmptyViewController, snapshot->equationStore()),
  m_intervalController(nullptr, this, snapshot->equationStore()),
  m_alternateEmptyViewController(nullptr, &m_solutionsController, &m_solutionsController),
  m_listController(&m_listFooter, snapshot->equationStore(), &m_listFooter),
  m_listFooter(&m_stackViewController, &m_listController, &m_listController, ButtonRowController::Position::Bottom, ButtonRowController::Style::EmbossedGray, ButtonRowController::Size::Large),
  m_stackViewController(&m_inputViewController, &m_listFooter),
  m_inputViewController(&m_modalViewController, &m_stackViewController, this, &m_listController, &m_listController)
{
}

void App::willBecomeInactive() {
  if (m_modalViewController.isDisplayingModal()) {
    m_modalViewController.dismissModalViewController(true);
  }
  if (inputViewController()->isDisplayingModal()) {
    inputViewController()->abortEditionAndDismiss();
  }
  ::App::willBecomeInactive();
}


bool App::isAcceptableExpression(const Poincare::Expression exp) {
  return TextFieldDelegateApp::ExpressionCanBeSerialized(exp, false, Poincare::Expression(), localContext())
      && !(exp.isUninitialized() || exp.type() == Poincare::ExpressionNode::Type::Store);
}

}
