#ifndef SHARED_FUNCTION_ZOOM_AND_PAN_CURVE_VIEW_CONTROLLER_H
#define SHARED_FUNCTION_ZOOM_AND_PAN_CURVE_VIEW_CONTROLLER_H

#include <escher/metric.h>
#include <apps/i18n.h>
#include <escher/key_view.h>
#include <escher/message_text_view.h>
#include "zoom_and_pan_curve_view_controller.h"
#include "banner_view.h"

namespace Shared {

class FunctionZoomAndPanCurveViewController : public ZoomAndPanCurveViewController {
public:
  FunctionZoomAndPanCurveViewController(Responder * parentResponder, InteractiveCurveViewRange * interactiveCurveViewRange, CurveView * curveView);
  const char * title() override;
  Escher::View * view() override { return &m_contentView; }
  void viewWillAppear() override;
  DisplayParameter displayParameter() override { return DisplayParameter::DisplayNoTitle; }
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  TELEMETRY_ID("Zoom");
private:
  constexpr static KDCoordinate k_standardViewHeight = 174;

  class ContentView : public Escher::View {
  public:
    constexpr static const KDFont * k_legendFont = KDFont::SmallFont;
    constexpr static KDCoordinate k_legendHeight = 2 * Escher::Metric::BannerTextMargin + 14; // k_legendFont->glyphSize().height() = 14
    ContentView(CurveView * curveView);
    void layoutSubviews(bool force = false) override;
    CurveView * curveView();
    bool displayLegend() const { return m_displayLegend; }
    void setDisplayLegend(bool v) { m_displayLegend = v; }
  private:
    class LegendView : public Escher::View {
    public:
      LegendView();
      void drawRect(KDContext * ctx, KDRect rect) const override;
    private:
      constexpr static int k_numberOfLegends = 3;
      constexpr static int k_numberOfTokens = 6;
      constexpr static KDCoordinate k_tokenWidth = 10;
      static constexpr KDColor BackgroundColor() { return BannerView::BackgroundColor(); }
      void layoutSubviews(bool force = false) override;
      int numberOfSubviews() const override;
      Escher::View * subviewAtIndex(int index) override;
      Escher::MessageTextView m_legends[k_numberOfLegends];
      Escher::KeyView m_legendPictograms[k_numberOfTokens];
    };
    int numberOfSubviews() const override;
    Escher::View * subviewAtIndex(int index) override;
    CurveView * m_curveView;
    LegendView m_legendView;
    bool m_displayLegend;
  };

  /* Warning : these methods do not reload the curve view. It should be done
   * manually after calling either or both. */
  void adaptRangeForHeaders(bool viewWillAppear);
  /* Returns true if the legend visibility has changed. */
  bool setLegendVisible(bool legendWillAppear);

  // ZoomAndPanCurveViewController
  InteractiveCurveViewRange * interactiveCurveViewRange() override { return m_interactiveRange; }
  CurveView * curveView() override { return m_contentView.curveView(); }

  ContentView m_contentView;
  InteractiveCurveViewRange * m_interactiveRange;
};

}

#endif
