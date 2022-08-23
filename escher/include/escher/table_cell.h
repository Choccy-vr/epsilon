#ifndef ESCHER_TABLE_CELL_H
#define ESCHER_TABLE_CELL_H

#include <escher/bordered.h>
#include <escher/highlight_cell.h>

namespace Escher {

class TableCell : public Bordered, public HighlightCell {
public:
  /* TableCells are layout as follow :
   * -Accessory is vertically centered on the right of the cell
   * -Label is vertically centered, aligned on the left of the cell
   * -If it fits, SubLabel is vertically centered between Label and Accessory,
      aligned on the right of the available space.
   * -Otherwise, SubLabel is placed below label, aligned on the left of the cell
   * With :
   *     * = As much space as available (or nothing)
   *     TM / BM / LM / RM : Cell[Top / Bottom / Left / Right]Margin
   *     HM / VM : Cell[Horizontal / Vertical]ElementMargin
   * First configuration : SubLabel fits
   * -------------------------------------------------
   * |                     TM                        |
   * | *     *    *   *     *      *       *      *  |
   * | LM  Label  HM  *  SubLabel  HM  Accessory  RM |
   * | *     *    *   *     *      *       *      *  |
   * |                     BM                        |
   * -------------------------------------------------
   * Second configuration : SubLabel does not fit
   * -------------------------------------------------
   * |                     TM                        |
   * | *      *      *             *      *       *  |
   * | LM  Label     *             HM             RM |
   * | LM    VM      *             HM  Accessory  RM |
   * | LM  SubLabel  *             HM             RM |
   * | *      *      *             *      *       *  |
   * |                     BM                        |
   * -------------------------------------------------
   * When isAccessoryAlignedRight returns false, Accessory can be placed on the
   * left of the Cell. Label and SubLabel also take the two configurations
   * depending on the fit.
   * When giveAccessoryAllWidth returns true, Accessory will take all available
   * horizontal space, forcing label and subLabel to layout over two rows.
   */
  TableCell();
  virtual const View * labelView() const { return nullptr; }
  virtual const View * subLabelView() const { return nullptr; }
  virtual const View * accessoryView() const { return nullptr; }
  void drawRect(KDContext * ctx, KDRect rect) const override;
  KDSize minimalSizeForOptimalDisplay() const override;
  virtual bool giveAccessoryAllWidth() const { return false; }

  static KDCoordinate minimalHeightForOptimalDisplay(const View * label, const View * subLabel, const View * accessory, KDCoordinate minAccessoryWidth, KDCoordinate width);
  static constexpr KDCoordinate k_minimalLargeFontCellHeight = Metric::CellSeparatorThickness + Metric::CellTopMargin + 18 + Metric::CellTopMargin; // KDFont::LargeFont->glyphSize().height() = 18
  static constexpr KDCoordinate k_minimalSmallFontCellHeight = Metric::CellSeparatorThickness + Metric::CellTopMargin + 14 + Metric::CellTopMargin; // KDFont::SmallFont->glyphSize().height() = 14
protected:
  virtual KDColor backgroundColor() const { return KDColorWhite; }
  int numberOfSubviews() const override;
  View * subviewAtIndex(int index) override;
  void layoutSubviews(bool force = false) override;
  virtual bool shouldAlignLabelAndAccessory() const { return false; }
  virtual bool shouldAlignSublabelRight() const { return true; }
  virtual KDCoordinate accessoryMinimalWidthOverridden() const { return -1; }
  virtual bool shouldHideSublabel() { return false; }
  // This method is only used to assert that no subview overlaps after layouting
  virtual bool subviewsCanOverlap() const { return false; }

  bool singleRowMode() const;

private:
  static bool singleRowMode(KDCoordinate width,
                            const View * labelView,
                            const View * sublabelView,
                            KDCoordinate accessoryWidth);
  static KDRect setFrameIfViewExists(View * v, KDRect rect, bool force);
};
}
#endif
