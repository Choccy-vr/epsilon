#ifndef ESCHER_TEXT_VIEW_H
#define ESCHER_TEXT_VIEW_H

#include <escher/glyphs_view.h>

namespace Escher {

class TextView : public GlyphsView {
 public:
  TextView(KDGlyph::Format format = {}, KDCoordinate lineSpacing = 0)
      : GlyphsView(format), m_lineSpacing(lineSpacing) {}
  // View
  void drawRect(KDContext* ctx, KDRect rect) const override;
  KDSize minimalSizeForOptimalDisplay() const override;

  const char* text() const override = 0;  // Must implement
  virtual void setText(const char* text) = 0;

#if ESCHER_VIEW_LOGGING
 protected:
  const char* className() const override { return "TextView"; }
#endif

 private:
  KDCoordinate m_lineSpacing;
};

}  // namespace Escher
#endif
