#include <escher/table_cell.h>
#include <escher/palette.h>
#include <escher/metric.h>
#include <algorithm>
#include <ion/display.h>

namespace Escher {

TableCell::TableCell() :
  Bordered(),
  HighlightCell()
{
}

int TableCell::numberOfSubviews() const {
  return (labelView() != nullptr) + (subLabelView()!= nullptr) + (accessoryView()!= nullptr);
}

View * TableCell::subviewAtIndex(int index) {
  if (index == 0) {
    return const_cast<View *>(labelView());
  }
  if (index == 1 && subLabelView() != nullptr) {
    return  const_cast<View *>(subLabelView());
  }
  assert(index == 2 || (index == 1 && subLabelView() == nullptr));
  return  const_cast<View *>(accessoryView());
}

KDCoordinate TableCell::minimalHeightForOptimalDisplay(const View * label, const View * subLabel, const View * accessory, KDCoordinate width, bool giveAccessoryAllWidth) {
  KDSize labelSize = label ? label->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize subLabelSize = subLabel ? subLabel->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize accessorySize = accessory ? accessory->minimalSizeForOptimalDisplay() : KDSizeZero;

  bool singleRow = singleRowMode(width, label, subLabel, accessory);

  KDCoordinate labelHeight = labelSize.height();
  KDCoordinate subLabelHeight = subLabelSize.height();
  // Compute minimal Height for Label and subLabel
  KDCoordinate labelsHeight =
      singleRow ? std::max<KDCoordinate>(labelHeight, subLabelHeight)
                : labelHeight + Metric::CellVerticalElementMargin + subLabelHeight;
  /* Space required for bottom separator is not accounted for as it overlaps
   * with neighbor cells. It is added to the frame in TableView, and exploited
   * when layouting subviews. */
  return k_separatorThickness + Metric::CellTopMargin + std::max<KDCoordinate>(labelsHeight, accessorySize.height()) + Metric::CellBottomMargin;
}

KDSize TableCell::minimalSizeForOptimalDisplay() const {
  // TableCell's available width is necessary to compute its minimal height.
  KDCoordinate expectedWidth = m_frame.width();
  assert(expectedWidth > 0);
  return KDSize(expectedWidth, minimalHeightForOptimalDisplay(labelView(), subLabelView(), accessoryView(), expectedWidth, giveAccessoryAllWidth()));
}

KDCoordinate cropIfOverflow(KDCoordinate value, KDCoordinate max) {
  return std::min<KDCoordinate>(max, value);
}

void TableCell::layoutSubviews(bool force) {
  KDCoordinate width = bounds().width();
  KDCoordinate height = bounds().height();
  if (width == 0 || height == 0) {
    return;
  }

  View * label = const_cast<View *>(labelView());
  View * subLabel = const_cast<View *>(subLabelView());
  View * accessory = const_cast<View *>(accessoryView());

  KDSize labelSize = label ? label->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize subLabelSize = subLabel ? subLabel->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize accessorySize = accessory ? accessory->minimalSizeForOptimalDisplay() : KDSizeZero;

  KDCoordinate y = 0;
  KDCoordinate x = 0;
  /* Apply margins and separators on every side. At this point, we assume cell's
   * frame has been updated to add bottom and right overlapping borders. */
  // TODO : improve overlapping borders so that we don't need to assume that.
  constexpr KDCoordinate leftOffset = k_separatorThickness + Metric::CellLeftMargin;
  x += leftOffset;
  width -= leftOffset + Metric::CellRightMargin + k_separatorThickness;
  constexpr KDCoordinate topOffset = k_separatorThickness + Metric::CellTopMargin;
  y += topOffset;
  height -= topOffset + Metric::CellBottomMargin + k_separatorThickness;

  assert(width > 0 && height > 0);
  KDCoordinate labelHeight = cropIfOverflow(labelSize.height(), height);
  KDCoordinate labelWidth = cropIfOverflow(labelSize.width(), width);
  KDCoordinate subLabelHeight = cropIfOverflow(subLabelSize.height(), height);
  KDCoordinate subLabelWidth = cropIfOverflow(subLabelSize.width(), width);
  KDCoordinate accessoryHeight = cropIfOverflow(accessorySize.height(), height);
  KDCoordinate accessoryWidth = cropIfOverflow(accessorySize.width(), width);

  bool singleRow = singleRowMode(bounds().width(), label, subLabel, accessory);

  if (singleRow) {  // Single row -> align vertically each view
    KDCoordinate maxHeight = std::max<KDCoordinate>(
        labelHeight, std::max<KDCoordinate>(subLabelHeight, accessoryHeight));

    // Label on the left, aligned vertically
    setFrameIfViewExists(
        label, KDRect(x, y + (height - labelHeight) / 2, labelWidth, labelHeight), force);

    x += labelWidth + Metric::CellHorizontalElementMargin;
    if (isSublabelAlignedRight() & !giveAccessoryAllWidth()) {
      // Align SubLabel right
      x = width - accessoryWidth - Metric::CellHorizontalElementMargin - subLabelWidth;
    }
    setFrameIfViewExists(
        subLabel,
        KDRect(x, y + (height - subLabelHeight) / 2, subLabelWidth, subLabelHeight),
        force);
    x += subLabelWidth + Metric::CellHorizontalElementMargin;

    KDCoordinate accessoryY = y + (height - accessoryHeight) / 2;
    if (giveAccessoryAllWidth()) {
      setFrameIfViewExists(
          accessory,
          KDRect(x, accessoryY, width - x + Metric::CellHorizontalElementMargin, accessoryHeight),
          force);
    } else {
      setFrameIfViewExists(
          accessory,
          KDRect(leftOffset + width - accessoryWidth, accessoryY, accessoryWidth, accessoryHeight),
          force);
    }

  } else {  // Two rows
    setFrameIfViewExists(label, KDRect(x, y, labelWidth, labelHeight), force);
    setFrameIfViewExists(
        subLabel,
        KDRect(
            x, y + labelHeight + Metric::CellVerticalElementMargin, subLabelWidth, subLabelHeight),
        force);

    KDCoordinate accessoryY = y;
    accessoryY += alignLabelAndAccessory() ? 0 : (height - accessoryHeight) / 2;
    if (giveAccessoryAllWidth()) {
      KDCoordinate maxX =
          leftOffset + Metric::CellVerticalElementMargin +
          (alignLabelAndAccessory() ? labelWidth : std::max(labelWidth, subLabelWidth));
      setFrameIfViewExists(
          accessory, KDRect(maxX, accessoryY, width - maxX, accessoryHeight), force);
    } else {
      setFrameIfViewExists(
          accessory,
          KDRect(leftOffset + width - accessoryWidth, accessoryY, accessoryWidth, accessoryHeight),
          force);
    }
  }
}

bool TableCell::singleRowMode(KDCoordinate width,
                              const View * labelView,
                              const View * sublabelView,
                              const View * accessoryView) {
  KDSize labelSize = labelView ? labelView->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize subLabelSize = sublabelView ? sublabelView->minimalSizeForOptimalDisplay() : KDSizeZero;
  KDSize accessorySize = accessoryView ? accessoryView->minimalSizeForOptimalDisplay() : KDSizeZero;

  constexpr KDCoordinate leftOffset = k_separatorThickness + Metric::CellLeftMargin;
  width -= leftOffset + Metric::CellRightMargin + k_separatorThickness;
  KDCoordinate labelWidth = cropIfOverflow(labelSize.width(), width);
  KDCoordinate subLabelWidth = cropIfOverflow(subLabelSize.width(), width);
  KDCoordinate accessoryWidth = cropIfOverflow(accessorySize.width(), width);

  bool singleRow = !(labelView && sublabelView &&
                     labelWidth + subLabelWidth + accessorySize.width() +
                             2 * Metric::CellHorizontalElementMargin >
                         width);
  return singleRow;
}

void TableCell::drawRect(KDContext * ctx, KDRect rect) const {
  KDColor backColor = isHighlighted() ? Palette::Select : backgroundColor();
  drawInnerRect(ctx, bounds(), backColor);
  drawBorderOfRect(ctx, bounds(), Palette::GrayBright);
}

void TableCell::setFrameIfViewExists(View * v, KDRect rect, bool force) {
  if (v) {
    v->setFrame(rect, force);
  }
}

}
