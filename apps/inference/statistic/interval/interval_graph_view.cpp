#include "interval_graph_view.h"

namespace Inference {

void IntervalGraphView::drawRect(KDContext * ctx, KDRect rect) const {
  /* We draw a background wall screen to avoid noisy screen when switching on
   * and off while the results are computed. */
  ctx->fillRect(KDRect(0, m_frame.height() - k_conclusionViewHeight, m_frame.width(), k_conclusionViewHeight), Escher::Palette::WallScreen);
}

void IntervalGraphView::reload(double center, double marginOfError) {
  m_conclusionView.setInterval(center, marginOfError);
  layoutSubviews();
  m_curveView.reload();
}

void IntervalGraphView::layoutSubviews(bool force) {
  int availableHeight = m_frame.height();
  int availableWidth = m_frame.width();
  int curveViewHeight = availableHeight - k_conclusionViewHeight;
  m_curveView.setFrame(KDRect(KDPointZero, KDSize(availableWidth, curveViewHeight)), force);
  m_conclusionView.setFrame(KDRect(KDPoint(0, curveViewHeight), KDSize(availableWidth, k_conclusionViewHeight)), force);
}

Escher::View * IntervalGraphView::subviewAtIndex(int i) {
  assert(i < numberOfSubviews());
  Escher::View * subviews[] = {&m_curveView, &m_conclusionView};
  return subviews[i];
}

}  // namespace Inference

