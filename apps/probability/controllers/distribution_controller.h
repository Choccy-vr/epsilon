#ifndef PROBABILITY_DISTRIBUTION_CONTROLLER_H
#define PROBABILITY_DISTRIBUTION_CONTROLLER_H

#include <escher/selectable_list_view_controller.h>

#include <new>

#include "probability/gui/cell.h"
#include "probability/models/distribution/distribution.h"
#include "parameters_controller.h"

namespace Probability {

class DistributionController : public Escher::SelectableListViewController {
 public:
  DistributionController(Escher::Responder * parentResponder, Distribution * m_distribution,
                         ParametersController * parametersController);
  Escher::View * view() override { return &m_contentView; }
  const char * title() override { return "Probability distributions"; }
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
  void viewWillAppear() override;
  TELEMETRY_ID("Distribution");
  int numberOfRows() const override { return k_totalNumberOfModels; }
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;
  KDCoordinate nonMemoizedRowHeight(int j) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override { return k_numberOfCells; }

 private:
  class ContentView : public Escher::View {
   public:
    ContentView(Escher::SelectableTableView * selectableTableView)
        : m_titleView(KDFont::SmallFont, I18n::Message::ChooseDistribution, 0.5f, 0.5f, Escher::Palette::GrayDark,
                      Escher::Palette::WallScreen),
          m_selectableTableView(selectableTableView),
          m_borderView(Escher::Palette::GrayBright) {}
    // Removing a pixel to skew title's baseline downward.
    constexpr static KDCoordinate k_titleMargin = Escher::Metric::CommonTopMargin - 1;

   private:
    int numberOfSubviews() const override { return 3; }
    Escher::View * subviewAtIndex(int index) override;
    void layoutSubviews(bool force = false) override;
    Escher::MessageTextView m_titleView;
    Escher::SelectableTableView * m_selectableTableView;
    Escher::SolidColorView m_borderView;
  };
  void setDistributionAccordingToIndex(int index);
  constexpr static int k_totalNumberOfModels = 9;
  constexpr static int k_numberOfCells =
      (Ion::Display::Height - Escher::Metric::TitleBarHeight - 14 - ContentView::k_titleMargin) /
          Escher::TableCell::k_minimalLargeFontCellHeight +
      2;  // Remaining cell can be above and below so we add +2, 14 for the small font height
  Cell m_cells[k_numberOfCells];
  ContentView m_contentView;
  Distribution * m_distribution;
  ParametersController * m_parametersController;
};

}  // namespace Probability

#endif
