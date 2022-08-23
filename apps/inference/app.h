#ifndef PROBABILITY_APP_H
#define PROBABILITY_APP_H

#include <apps/shared/menu_controller.h>
#include <apps/shared/shared_app.h>
#include <apps/shared/text_field_delegate_app.h>
#include <escher/app.h>
#include <escher/container.h>
#include <escher/stack_view_controller.h>
#include <ion/ring_buffer.h>

#include "abstract/dynamic_cells_data_source.h"
#include "controllers/categorical_type_controller.h"
#include "controllers/distribution_controller.h"
#include "controllers/hypothesis_controller.h"
#include "controllers/input_controller.h"
#include "controllers/input_goodness_controller.h"
#include "controllers/input_homogeneity_controller.h"
#include "controllers/interval_graph_controller.h"
#include "controllers/parameters_controller.h"
#include "controllers/results_controller.h"
#include "controllers/results_homogeneity_controller.h"
#include "controllers/test_graph_controller.h"
#include "controllers/test_controller.h"
#include "controllers/type_controller.h"
#include "models/models_buffer.h"

namespace Inference {

class App : public Shared::TextFieldDelegateApp, public Shared::MenuControllerDelegate {
public:
  // Descriptor
  class Descriptor : public Escher::App::Descriptor {
  public:
    I18n::Message name() const override { return I18n::Message::DistributionApp; };
    I18n::Message upperName() const override { return I18n::Message::DistributionAppCapital; };
    const Escher::Image * icon() const override;
  };

  // Snapshot
  class Snapshot : public Shared::SharedApp::Snapshot {
  public:
    App * unpack(Escher::Container * container) override {
      return new (container->currentAppBuffer()) App(this);
    };
    const Descriptor * descriptor() const override;
    void tidy() override;
    void reset() override;

    Inference * inference() { return m_modelBuffer.inference(); }
    Distribution * distribution() { return m_modelBuffer.distribution(); }
    Calculation * calculation() { return m_modelBuffer.calculation(); }
    Statistic * statistic() { return m_modelBuffer.statistic(); }

    Ion::RingBuffer<Escher::ViewController *, Escher::k_MaxNumberOfStacks> * pageQueue() { return &m_pageQueue; }
  private:
    friend App;
    // TODO: optimize size of Stack
    Ion::RingBuffer<Escher::ViewController *, Escher::k_MaxNumberOfStacks> m_pageQueue;
    ModelBuffer m_modelBuffer;
  };

  static App * app() { return static_cast<App *>(Escher::Container::activeApp()); }
  void didBecomeActive(Window * window) override;

  // Navigation
  void willOpenPage(ViewController * controller) override;
  void didExitPage(ViewController * controller) override;

  // Cells buffer API
  void * buffer(size_t offset = 0) { return m_buffer + offset; }
  void cleanBuffer(DynamicCellsDataSourceDestructor * destructor);

  static constexpr int k_bufferSize = std::max({
      sizeof(ExpressionCellWithBufferWithMessage) * k_maxNumberOfExpressionCellsWithBufferWithMessage,
      sizeof(ExpressionCellWithEditableTextWithMessage) * k_maxNumberOfExpressionCellsWithEditableTextWithMessage,
      sizeof(EvenOddBufferTextCell) * (k_homogeneityTableNumberOfReusableHeaderCells + k_homogeneityTableNumberOfReusableInnerCells),
      sizeof(EvenOddEditableTextCell) * k_homogeneityTableNumberOfReusableInnerCells + sizeof(EvenOddBufferTextCell) * k_homogeneityTableNumberOfReusableHeaderCells,
      sizeof(EvenOddEditableTextCell) * k_inputGoodnessTableNumberOfReusableCells
    });

  TELEMETRY_ID("Inference");

  // Shared::MenuControllerDelegate
  void selectSubApp(int subAppIndex) override;
  int selectedSubApp() const override { return static_cast<int>(snapshot()->inference()->subApp()); }
  int numberOfSubApps() const override { return static_cast<int>(Inference::SubApp::NumberOfSubApps); }

private:
  App(Snapshot *);
  Snapshot * snapshot() const { return static_cast<Snapshot *>(Escher::App::snapshot()); }

  // Controllers
  TestGraphController m_testGraphController;
  IntervalGraphController m_intervalGraphController;
  ResultsHomogeneityController m_homogeneityResultsController;
  InputHomogeneityController m_inputHomogeneityController;
  InputGoodnessController m_inputGoodnessController;
  ResultsController m_resultsController;
  InputController m_inputController;
  TypeController m_typeController;
  CategoricalTypeController m_categoricalTypeController;
  HypothesisController m_hypothesisController;
  CalculationController m_calculationController;
  ParametersController m_parameterController;
  DistributionController m_distributionController;
  TestController m_testController;
  Shared::MenuController m_menuController;
  Escher::StackViewController m_stackViewController;
  /* Buffer used for allocating table cells to avoid duplicating required
   * space for these memory-needy tables. */
  char m_buffer[k_bufferSize];
  DynamicCellsDataSourceDestructor * m_bufferDestructor;
};

}  // namespace Inference

#endif /* PROBABILITY_APP_H */
