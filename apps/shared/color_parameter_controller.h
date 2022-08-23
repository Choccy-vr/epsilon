#ifndef SHARED_COLOR_PARAMETER_CONTROLLER_H
#define SHARED_COLOR_PARAMETER_CONTROLLER_H

#include <escher/selectable_list_view_controller.h>
#include "color_cell.h"
#include "color_names.h"
#include "function_store.h"

namespace Shared {

class ColorParameterController : public Escher::SelectableListViewController<Escher::SimpleListViewDataSource> {
public:
  ColorParameterController(Escher::Responder * parentResponder) :
    SelectableListViewController<SimpleListViewDataSource>(parentResponder)
  {
  }

  // ViewController
  const char * title() override { return I18n::translate(I18n::Message::Color); }
  void viewWillAppear() override;

  // Responder
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;

  // SimpleListViewDataSource
  Escher::HighlightCell * reusableCell(int index) override { return &m_cells[index]; }
  int reusableCellCount() const override { return k_numberOfCells; }
  int numberOfRows() const override { return ColorNames::Count; }
  void willDisplayCellForIndex(Escher::HighlightCell * cell, int index) override;

  // ColorParameterController
  void setRecord(Ion::Storage::Record record) { m_record = record; }
private:
  ExpiringPointer<Function> function();
  constexpr static int k_numberOfCells = ((Ion::Display::Height - Escher::Metric::TitleBarHeight - Escher::Metric::TabHeight - 2*Escher::Metric::StackTitleHeight) / Escher::TableCell::k_minimalLargeFontCellHeight) + 2; // Remaining cell can be above and below so we add +2
  Ion::Storage::Record m_record;
  ColorCell m_cells[k_numberOfCells];
};

}

#endif
