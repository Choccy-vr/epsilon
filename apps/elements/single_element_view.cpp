#include "single_element_view.h"

#include <poincare/print_int.h>

#include "app.h"

namespace Elements {

void SingleElementView::drawRect(KDContext* ctx, KDRect rect) const {
  ctx->fillRect(bounds(), m_backgroundColor);

  ElementsViewDataSource* dataSource = App::app()->elementsViewDataSource();
  AtomicNumber z = dataSource->selectedElement();
  assert(ElementsDataBase::IsElement(z));
  DataField::ColorPair colors = dataSource->field()->getColors(z);

  KDRect bgRect((bounds().width() - k_totalSize) / 2 + k_borderSize,
                (bounds().height() - k_totalSize) / 2 + k_borderSize,
                k_cellSize, k_cellSize);
  ctx->fillRect(bgRect, colors.bg());

  KDRect borders[4] = {
      KDRect(bgRect.x() - k_borderSize, bgRect.y() - k_borderSize, k_totalSize,
             k_borderSize),  // Top
      KDRect(bgRect.x() - k_borderSize, bgRect.y() + k_cellSize, k_totalSize,
             k_borderSize),  // Bottom
      KDRect(bgRect.x() - k_borderSize, bgRect.y(), k_borderSize,
             k_cellSize),  // Left
      KDRect(bgRect.x() + bgRect.width(), bgRect.y(), k_borderSize,
             k_cellSize),  // Right
  };
  for (KDRect r : borders) {
    ctx->fillRect(r, colors.fg());
  }

  const char* symbol = ElementsDataBase::Symbol(z);
  KDCoordinate symbolXOffset = k_symbolZAMargin;
  KDSize symbolSize = KDFont::Font(k_symbolFont)->stringSize(symbol);

  constexpr size_t k_bufferSize = 4;
  char zBuffer[k_bufferSize];
  int zLength = Poincare::PrintInt::Left(z, zBuffer, k_bufferSize - 1);
  zBuffer[zLength] = 0;
  KDSize zSize = KDFont::Font(k_numbersFont)->stringSize(zBuffer);

  uint16_t a = ElementsDataBase::NumberOfMass(z);
  char aBuffer[k_bufferSize];
  KDSize aSize = KDSizeZero;
  if (a == ElementData::k_AUnknown) {
    symbolXOffset += zSize.width();
    aBuffer[0] = 0;
  } else {
    assert(a < 1000);
    int aLength = Poincare::PrintInt::Left(a, aBuffer, k_bufferSize - 1);
    aBuffer[aLength] = 0;
    aSize = KDFont::Font(k_numbersFont)->stringSize(aBuffer);
    assert(aSize.width() >= zSize.width());  // since A >= Z
    symbolXOffset += aSize.width();
  }

  KDPoint symbolOrigin(
      bgRect.x() + (k_cellSize - symbolSize.width() - symbolXOffset) / 2 +
          symbolXOffset,
      bgRect.y() + (k_cellSize - symbolSize.height()) / 2);
  KDGlyph::Style numbersStyle{.glyphColor = colors.fg(),
                              .backgroundColor = colors.bg(),
                              .font = k_numbersFont};
  KDGlyph::Style symbolStyle{.glyphColor = colors.fg(),
                             .backgroundColor = colors.bg(),
                             .font = k_symbolFont};
  ctx->drawString(symbol, symbolOrigin, symbolStyle);
  ctx->drawString(
      zBuffer,
      KDPoint(symbolOrigin.x() - zSize.width() - k_symbolZAMargin,
              symbolOrigin.y() + symbolSize.height() - k_ZVerticalOffset),
      numbersStyle);
  ctx->drawString(
      aBuffer,
      KDPoint(symbolOrigin.x() - aSize.width() - k_symbolZAMargin,
              symbolOrigin.y() - aSize.height() + k_AVerticalOffset),
      numbersStyle);
}

}  // namespace Elements
