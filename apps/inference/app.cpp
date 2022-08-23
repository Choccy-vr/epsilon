#include "app.h"

#include <apps/shared/text_field_delegate_app.h>
#include <apps/exam_mode_configuration.h>

#include "models/statistic/homogeneity_test.h"
#include "inference_icon.h"
#include "images/confidence_interval.h"
#include "images/probability.h"
#include "images/significance_test.h"

namespace Inference {

const Escher::Image * App::Descriptor::icon() const {
  return ImageStore::InferenceIcon;
}

App::App(Snapshot * snapshot) :
    TextFieldDelegateApp(snapshot, &m_stackViewController),
    m_testGraphController(&m_stackViewController, static_cast<Test *>(snapshot->statistic())),
    m_intervalGraphController(&m_stackViewController, static_cast<Interval *>(snapshot->statistic())),
    m_homogeneityResultsController(
        &m_stackViewController,
        &m_resultsController,
        static_cast<HomogeneityTest *>(snapshot->statistic())),
    m_inputHomogeneityController(
        &m_stackViewController,
        &m_homogeneityResultsController,
        static_cast<HomogeneityTest *>(snapshot->statistic()),
        this),
    m_inputGoodnessController(&m_stackViewController,
                              &m_resultsController,
                              static_cast<GoodnessTest *>(snapshot->statistic()),
                              this),
    m_inputSlopeController(&m_stackViewController,
                              &m_resultsController,
                              snapshot->statistic(),
                              this),
    m_resultsController(&m_stackViewController,
                        snapshot->statistic(),
                        &m_testGraphController,
                        &m_intervalGraphController,
                        this,
                        this),
    m_inputController(&m_stackViewController,
                      &m_resultsController,
                      snapshot->statistic(),
                      this),
    m_typeController(&m_stackViewController,
                     &m_hypothesisController,
                     &m_inputController,
                     snapshot->statistic()),
    m_categoricalTypeController(&m_stackViewController,
                                static_cast<Chi2Test *>(snapshot->statistic()),
                                &m_inputGoodnessController,
                                &m_inputHomogeneityController),
    m_hypothesisController(&m_stackViewController,
                           &m_inputController,
                           &m_inputSlopeController,
                           this,
                           static_cast<Test *>(snapshot->statistic())),
    m_calculationController(&m_stackViewController,
                            this,
                            snapshot->distribution(),
                            snapshot->calculation()),
    m_parameterController(&m_stackViewController,
                          this,
                          snapshot->distribution(),
                          &m_calculationController),
    m_distributionController(&m_stackViewController,
                             snapshot->distribution(),
                             &m_parameterController),
    m_testController(&m_stackViewController,
                     &m_hypothesisController,
                     &m_typeController,
                     &m_categoricalTypeController,
                     &m_inputSlopeController,
                     &m_inputController,
                     snapshot->statistic()),
    m_menuController(
        &m_stackViewController,
        {&m_distributionController, &m_testController, &m_testController},
        {{I18n::Message::ProbaApp, I18n::Message::ProbaDescr}, {I18n::Message::Tests, I18n::Message::TestDescr}, {I18n::Message::Intervals, I18n::Message::IntervalDescr}},
        {ImageStore::Probability, ImageStore::SignificanceTest, ImageStore::ConfidenceInterval},
        this
      ),
    m_stackViewController(&m_modalViewController, &m_menuController, StackViewController::Style::GrayGradation),
    m_bufferDestructor(nullptr)
{
}

void App::didBecomeActive(Window * windows) {
  Ion::RingBuffer<Escher::ViewController *, Escher::k_MaxNumberOfStacks> * queue = snapshot()->pageQueue();
  int queueLength = queue->length();
  Escher::ViewController * currentController = &m_menuController;
  for (int i = 0; i < queueLength; i++) {
    /* The queue is refilled dynamically when "stackOpenPage"ing which prevents
     * from popping until the queue is empty. */
    Escher::ViewController * controller = queue->queuePop();
    currentController->stackOpenPage(controller);
    currentController = controller;
  }
  Escher::App::didBecomeActive(windows);
}

void App::willOpenPage(ViewController * controller) {
  snapshot()->pageQueue()->push(controller);
}

void App::didExitPage(ViewController * controller) {
  ViewController * c = snapshot()->pageQueue()->stackPop();
  assert(c == controller);
  (void)c;
}

void App::cleanBuffer(DynamicCellsDataSourceDestructor * destructor) {
  assert(destructor);
  if (m_bufferDestructor) {
    m_bufferDestructor->destroyCells();
  }
  m_bufferDestructor = destructor;
}

bool App::selectSubApp(int subAppIndex) {
  if (subAppIndex >= 0 && Inference::Initialize(snapshot()->inference(), static_cast<Inference::SubApp>(subAppIndex))) {
    m_distributionController.selectRow(0);
    m_testController.selectRow(0);
    m_hypothesisController.selectRow(0);
    m_typeController.selectRow(0);
  }
  return true;
}

const App::Descriptor * App::Snapshot::descriptor() const {
  static App::Descriptor s_descriptor;
  return &s_descriptor;
}

void App::Snapshot::tidy() {
  inference()->tidy();
}

void App::Snapshot::reset() {
  m_pageQueue.reset();
}

}  // namespace Inference
