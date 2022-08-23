#ifndef CALCULATION_SCROLLABLE_THREE_EXPRESSIONS_CELL_H
#define CALCULATION_SCROLLABLE_THREE_EXPRESSIONS_CELL_H

#include "../../shared/scrollable_multiple_expressions_view.h"
#include "../calculation.h"
#include "expression_with_equal_sign_view.h"
#include <escher/table_cell.h>

namespace Calculation {

/* TODO There is factorizable code between this and Calculation::HistoryViewCell
 * (at least setCalculation). */

class ScrollableThreeExpressionsView : public Shared::AbstractScrollableMultipleExpressionsView {
public:
  static constexpr KDCoordinate k_margin = Escher::Metric::CommonSmallMargin;
  ScrollableThreeExpressionsView(Responder * parentResponder) : Shared::AbstractScrollableMultipleExpressionsView(parentResponder, &m_contentCell) {
    setMargins(k_margin, k_margin, k_margin, k_margin); // Left Right margins are already added by TableCell
    setBackgroundColor(KDColorWhite);
  }
  void resetMemoization();
  void setCalculation(Calculation * calculation, Poincare::Context * context, bool canChangeDisplayOutput);
  void subviewFrames(KDRect * leftFrame, KDRect * centerFrame, KDRect * approximateSignFrame, KDRect * rightFrame) {
    return m_contentCell.subviewFrames(leftFrame, centerFrame, approximateSignFrame, rightFrame);
  }
private:
  class ContentCell : public Shared::AbstractScrollableMultipleExpressionsView::ContentCell {
  public:
    using Shared::AbstractScrollableMultipleExpressionsView::ContentCell::ContentCell;
    KDColor backgroundColor() const override { return KDColorWhite; }
    void setEven(bool even) override { return; }
    Escher::ExpressionView * leftExpressionView() const override { return const_cast<ExpressionWithEqualSignView *>(&m_leftExpressionView); }
  private:
    ExpressionWithEqualSignView m_leftExpressionView;
  };

  ContentCell *  contentCell() override { return &m_contentCell; };
  const ContentCell * constContentCell() const override { return &m_contentCell; };
  ContentCell m_contentCell;
};

class ScrollableThreeExpressionsCell : public Escher::TableCell, public Escher::Responder {
public:
  static KDCoordinate Height(Calculation * calculation, Poincare::Context * context);
  ScrollableThreeExpressionsCell() :
    Responder(nullptr),
    m_view(this) {}

  // Cell
  Poincare::Layout layout() const override { return m_view.layout(); }

  // Responder cell
  Escher::Responder * responder() override {
    return this;
  }
  void didBecomeFirstResponder() override;

  // Table cell
  const View * labelView() const override { return &m_view; }

  void setHighlighted(bool highlight) override { m_view.evenOddCell()->setHighlighted(highlight); }
  void resetMemoization() { m_view.resetMemoization(); }
  void setCalculation(Calculation * calculation, Poincare::Context * context, bool canChangeDisplayOutput = false);
  void setDisplayCenter(bool display);
  ScrollableThreeExpressionsView::SubviewPosition selectedSubviewPosition() { return m_view.selectedSubviewPosition(); }
  void setSelectedSubviewPosition(ScrollableThreeExpressionsView::SubviewPosition subviewPosition) { m_view.setSelectedSubviewPosition(subviewPosition); }

  void reinitSelection();
  void subviewFrames(KDRect * leftFrame, KDRect * centerFrame, KDRect * approximateSignFrame, KDRect * rightFrame) {
    return m_view.subviewFrames(leftFrame, centerFrame, approximateSignFrame, rightFrame);
  }
private:
  ScrollableThreeExpressionsView m_view;
};

}

#endif
