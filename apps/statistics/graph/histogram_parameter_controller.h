#ifndef STATISTICS_HISTOGRAM_PARAMETER_CONTROLLER_H
#define STATISTICS_HISTOGRAM_PARAMETER_CONTROLLER_H

#include <apps/shared/float_parameter_controller.h>
#include <apps/shared/pop_up_controller.h>
#include <escher/menu_cell_with_editable_text.h>
#include <escher/message_text_view.h>

#include "../store.h"

namespace Statistics {

class HistogramParameterController
    : public Shared::FloatParameterController<double> {
 public:
  HistogramParameterController(Escher::Responder* parentResponder,
                               Store* store);
  static bool AuthorizedBarWidth(double barWidth, double firstDrawnBarAbscissa,
                                 Store* store);

  void viewWillAppear() override;
  const char* title() override;
  int numberOfRows() const override { return 1 + k_numberOfCells; }
  KDCoordinate nonMemoizedRowHeight(int row) override;

 private:
  constexpr static int k_numberOfCells = 2;
  double extractParameterAtIndex(int index);
  bool handleEvent(Ion::Events::Event event) override;
  double parameterAtIndex(int index) override;
  bool setParameterAtIndex(int parameterIndex, double f) override;
  Escher::HighlightCell* reusableParameterCell(int index, int type) override;
  Escher::TextField* textFieldOfCellAtIndex(Escher::HighlightCell* cell,
                                            int index) override;
  int reusableParameterCellCount(int type) const override {
    return k_numberOfCells;
  }
  void buttonAction() override;
  bool authorizedParameters(double tempBarWidth,
                            double tempFirstDrawnBarAbscissa);
  Escher::MenuCellWithEditableText<Escher::MessageTextView,
                                   Escher::MessageTextView>
      m_cells[k_numberOfCells];
  Store* m_store;
  Shared::MessagePopUpController m_confirmPopUpController;
  // Temporary parameters
  double m_tempBarWidth;
  double m_tempFirstDrawnBarAbscissa;
};

}  // namespace Statistics

#endif
