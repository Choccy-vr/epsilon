#include "history_view_cell.h"

#include <apps/constant.h>
#include <assert.h>
#include <poincare/exception_checkpoint.h>
#include <string.h>

#include <algorithm>

#include "app.h"
#include "calculation_selectable_list_view.h"

using namespace Escher;
using namespace Poincare;

namespace Calculation {

/* HistoryViewCell */

void HistoryViewCell::ComputeCalculationHeights(Calculation *calculation,
                                                Context *context) {
  HistoryViewCell cell(nullptr);
  cell.setNewCalculation(calculation, false, context, true);
  KDCoordinate unExpandedHeight = cell.minimalHeightForOptimalDisplay();
  KDCoordinate expandedHeight = cell.updateExpanded(true)
                                    ? cell.minimalHeightForOptimalDisplay()
                                    : unExpandedHeight;
  calculation->setHeights(unExpandedHeight, expandedHeight);
}

HistoryViewCell::HistoryViewCell(Responder *parentResponder)
    : Responder(parentResponder),
      m_calculationCRC32(0),
      m_calculationDisplayOutput(Calculation::DisplayOutput::Unknown),
      m_hasEllipsis(false),
      m_inputView(this, k_inputViewHorizontalMargin,
                  k_inputOutputViewsVerticalMargin),
      m_scrollableOutputView(this),
      m_calculationExpanded(TrinaryBoolean::Unknown),
      m_calculationSingleLine(false) {}

void HistoryViewCell::setEven(bool even) {
  EvenOddCell::setEven(even);
  m_scrollableOutputView.evenOddCell()->setEven(even);
  m_ellipsis.setEven(even);
}

void HistoryViewCell::setHighlighted(bool highlight) {
  if (isHighlighted() == highlight) {
    return;
  }
  setHighlightedWithoutReload(highlight);
  reloadSubviewHighlight();
  // Re-layout as the ellispsis subview might have appear/disappear
  layoutSubviews();
}

void HistoryViewCell::updateSubviewsBackgroundAfterChangingState() {
  m_inputView.setBackgroundColor(backgroundColor());
  m_scrollableOutputView.setBackgroundColor(backgroundColor());
}

void HistoryViewCell::reloadSubviewHighlight() {
  assert(m_dataSource);
  m_inputView.setExpressionBackgroundColor(backgroundColor());
  m_scrollableOutputView.evenOddCell()->setHighlighted(false);
  m_ellipsis.setHighlighted(false);
  if (isHighlighted()) {
    HistoryViewCellDataSource::SubviewType type =
        m_dataSource->selectedSubviewType();
    switch (type) {
      case HistoryViewCellDataSource::SubviewType::Input:
        m_inputView.setExpressionBackgroundColor(Palette::Select);
        return;
      case HistoryViewCellDataSource::SubviewType::Output:
        m_scrollableOutputView.evenOddCell()->setHighlighted(true);
        return;
      case HistoryViewCellDataSource::SubviewType::Ellipsis:
        m_ellipsis.setHighlighted(true);
        return;
      default:
        assert(type == HistoryViewCellDataSource::SubviewType::None);
        return;
    }
  }
}

Layout HistoryViewCell::layout() const {
  assert(m_dataSource);
  HistoryViewCellDataSource::SubviewType type =
      m_dataSource->selectedSubviewType();
  switch (type) {
    case HistoryViewCellDataSource::SubviewType::Input:
      return m_inputView.layout();
    case HistoryViewCellDataSource::SubviewType::Output:
      return m_scrollableOutputView.layout();
    default:
      assert(type == HistoryViewCellDataSource::SubviewType::Ellipsis);
      return Layout();
  }
}

void HistoryViewCell::reloadScroll() {
  m_inputView.resetScroll();
  m_scrollableOutputView.reloadScroll();
}

void HistoryViewCell::reloadOutputSelection(
    HistoryViewCellDataSource::SubviewType previousType) {
  assert(m_calculationDisplayOutput != Calculation::DisplayOutput::Unknown);
  /* Select the right output according to the calculation display output. This
   * will reload the scroll to display the selected output. */
  bool selectExactOutput =
      m_calculationDisplayOutput ==
          Calculation::DisplayOutput::ExactAndApproximate &&
      previousType != HistoryViewCellDataSource::SubviewType::Ellipsis;
  m_scrollableOutputView.setSelectedSubviewPosition(
      selectExactOutput ? ScrollableTwoLayoutsView::SubviewPosition::Center
                        : ScrollableTwoLayoutsView::SubviewPosition::Right);
}

void HistoryViewCell::cellDidSelectSubview(
    HistoryViewCellDataSource::SubviewType type,
    HistoryViewCellDataSource::SubviewType previousType) {
  // Init output selection
  if (type == HistoryViewCellDataSource::SubviewType::Output) {
    reloadOutputSelection(previousType);
  }

  /* The selected subview has changed. The displayed outputs might have changed.
   * For example, for the calculation 1.2+2 --> 3.2, selecting the output would
   * display 1.2+2 --> 16/5 = 3.2. */
  updateExpanded(type == HistoryViewCellDataSource::SubviewType::Output);

  /* The displayed outputs have changed. We need to re-layout the cell
   * and re-initialize the scroll. */
  layoutSubviews();
  reloadScroll();
}

View *HistoryViewCell::subviewAtIndex(int index) {
  /* The order of the subviews should not matter here as they don't overlap.
   * However, the order determines the order of redrawing as well. For several
   * reasons listed after, changing subview selection often redraws the entire
   * m_scrollableOutputView even if it seems unecessary:
   * - Before feeding new Layouts to LayoutViews, we reset the hold layouts
   *   in order to empty the Poincare pool and have more space to compute new
   *   layouts.
   * - Even if we did not do that, LayoutView::setLayout doesn't avoid
   *   redrawing when the previous expression is identical (for reasons
   *   explained in layout_view.cpp)
   * - Because of the toggling burger view, LayoutViews often have the same
   *   absolute frame but a different relative frame which leads to redrawing
   *   them anyway.
   * All these reasons cause a blinking which can be avoided if we redraw the
   * output view before the input view (starting with redrawing the more
   * complex view enables to redraw it before the vblank thereby preventing
   * blinking).
   * TODO: this is a dirty hack which should be fixed! */
  View *views[3] = {&m_scrollableOutputView, &m_inputView, &m_ellipsis};
  return views[index];
}

bool HistoryViewCell::ViewsCanBeSingleLine(KDCoordinate inputViewWidth,
                                           KDCoordinate outputViewWidth,
                                           bool ellipsis) {
  // k_margin is the separation between the input and output.
  return (inputViewWidth + k_margin + outputViewWidth) <
         Ion::Display::Width - (ellipsis ? Metric::EllipsisCellWidth : 0);
}

void HistoryViewCell::layoutSubviews(bool force) {
  KDRect frameBounds = bounds();
  if (bounds().width() <= 0 || bounds().height() <= 0) {
    /* TODO Make this behaviour in a non-virtual layoutSublviews, and all layout
     * subviews should become privateLayoutSubviews */
    return;
  }
  KDRect ellipsisFrame = KDRectZero;
  KDRect inputFrame = KDRectZero;
  KDRect outputFrame = KDRectZero;
  computeSubviewFrames(frameBounds.width(), frameBounds.height(),
                       &ellipsisFrame, &inputFrame, &outputFrame);

  /* Required even if ellipsisFrame is KDRectZero, to mark previous rect as
   * dirty */
  setChildFrame(&m_ellipsis, ellipsisFrame, force);
  setChildFrame(&m_inputView, inputFrame, force);
  setChildFrame(&m_scrollableOutputView, outputFrame, force);
}

void HistoryViewCell::computeSubviewFrames(KDCoordinate frameWidth,
                                           KDCoordinate frameHeight,
                                           KDRect *ellipsisFrame,
                                           KDRect *inputFrame,
                                           KDRect *outputFrame) {
  assert(ellipsisFrame && inputFrame && outputFrame);

  if (isDisplayingEllipsis()) {
    *ellipsisFrame = KDRect(frameWidth - Metric::EllipsisCellWidth, 0,
                            Metric::EllipsisCellWidth, frameHeight);
    frameWidth -= Metric::EllipsisCellWidth;
  } else {
    *ellipsisFrame = KDRectZero;
  }

  KDSize inputSize = m_inputView.minimalSizeForOptimalDisplay();
  KDSize outputSize = m_scrollableOutputView.minimalSizeForOptimalDisplay();

  /* To compute if the calculation is on a single line, use the expanded width
   * if there is both an exact and an approximate layout. */
  m_calculationSingleLine = ViewsCanBeSingleLine(
      inputSize.width(),
      m_scrollableOutputView.minimalSizeForOptimalDisplayFullSize().width(),
      hasEllipsis());

  KDCoordinate inputY = k_margin;
  KDCoordinate outputY = k_margin;
  if (m_calculationSingleLine && !m_inputView.layout().isUninitialized()) {
    KDCoordinate inputBaseline =
        m_inputView.layout().baseline(m_inputView.font());
    KDCoordinate outputBaseline = m_scrollableOutputView.baseline();
    KDCoordinate baselineDifference = outputBaseline - inputBaseline;
    if (baselineDifference > 0) {
      inputY += baselineDifference;
    } else {
      outputY += -baselineDifference;
    }
  } else {
    KDCoordinate totalHeight =
        2 * k_margin + outputSize.height() + inputSize.height();
    if (totalHeight < k_maxCellHeight) {
      outputY += inputSize.height();
    } else {
      /* This makes it so the output is showed in priority if the calculation is
       * too large for the cell. */
      outputY = k_maxCellHeight - outputSize.height() - k_margin;
      inputY = outputY - inputSize.height();
    }
  }

  *inputFrame = KDRect(0, inputY, std::min(frameWidth, inputSize.width()),
                       inputSize.height());
  *outputFrame =
      KDRect(std::max(0, frameWidth - outputSize.width()), outputY,
             std::min(frameWidth, outputSize.width()), outputSize.height());
}

void HistoryViewCell::resetMemoization() {
  /* Clean the layouts to make room in the pool
   * TODO: maybe do this only when the layout won't change to avoid blinking */
  m_inputView.setLayout(Layout());
  m_scrollableOutputView.resetLayouts();
  m_calculationCRC32 = 0;
}

void HistoryViewCell::setCalculation(Calculation *calculation, bool expanded,
                                     Context *context,
                                     bool canChangeDisplayOutput) {
  uint32_t newCalculationCRC =
      Ion::crc32Byte((const uint8_t *)calculation,
                     ((char *)calculation->next()) - ((char *)calculation));
  if (newCalculationCRC == m_calculationCRC32) {
    if (updateExpanded(expanded)) {
      reloadScroll();
    }
    return;
  }

  // TODO: maybe do this only when the layout won't change to avoid blinking
  resetMemoization();

  // Memoization
  m_calculationCRC32 = newCalculationCRC;

  setNewCalculation(calculation, expanded, context, canChangeDisplayOutput);

  /* The displayed input and outputs have changed. We need to re-layout the cell
   * and re-initialize the scroll. */
  layoutSubviews();
  reloadScroll();
}

void HistoryViewCell::setNewCalculation(Calculation *calculation, bool expanded,
                                        Poincare::Context *context,
                                        bool canChangeDisplayOutput) {
  // Memoization
  m_hasEllipsis = calculation->additionalResultsType().isNotEmpty();
  m_inputView.setLayout(calculation->createInputLayout());

  /* All expressions have to be updated at the same time. Otherwise,
   * when updating one layout, if the second one still points to a deleted
   * layout, calling to layoutSubviews() would fail. */
  Layout exactOutputLayout, approximateOutputLayout;
  KDFont::Size font = m_scrollableOutputView.font();
  KDCoordinate maxVisibleWidth =
      Ion::Display::Width -
      (m_scrollableOutputView.margins()->width() +
       2 * KDFont::GlyphWidth(font));  // > arrow and = sign
  calculation->createOutputLayouts(&exactOutputLayout, &approximateOutputLayout,
                                   context, canChangeDisplayOutput,
                                   maxVisibleWidth, font);

  /* Update m_calculationDisplayOutput. Must be done after createOutputLayouts
   * because calculation->displayOutput can change. */
  m_calculationDisplayOutput = calculation->displayOutput(context);

  /* Update m_scrollableOutputView. Must be done once m_calculationDisplayOutput
   * has been updated. We must set which subviews are displayed before
   * setLayouts to mark the right rectangle as dirty. */
  m_scrollableOutputView.setDisplayableCenter(
      m_calculationDisplayOutput ==
          Calculation::DisplayOutput::ExactAndApproximate ||
      m_calculationDisplayOutput ==
          Calculation::DisplayOutput::ExactAndApproximateToggle);
  m_scrollableOutputView.setLayouts(Layout(), exactOutputLayout,
                                    approximateOutputLayout);
  m_scrollableOutputView.setExactAndApproximateAreStriclyEqual(
      calculation->equalSign(context) == Calculation::EqualSign::Equal);
  updateExpanded(expanded);
}

void HistoryViewCell::didBecomeFirstResponder() {
  assert(m_dataSource);
  if (m_dataSource->selectedSubviewType() ==
      HistoryViewCellDataSource::SubviewType::Input) {
    App::app()->setFirstResponder(&m_inputView);
  } else if (m_dataSource->selectedSubviewType() ==
             HistoryViewCellDataSource::SubviewType::Output) {
    App::app()->setFirstResponder(&m_scrollableOutputView);
  }
}

KDCoordinate HistoryViewCell::minimalHeightForOptimalDisplay() {
  KDRect ellipsisFrame = KDRectZero;
  KDRect inputFrame = KDRectZero;
  KDRect outputFrame = KDRectZero;
  computeSubviewFrames(Ion::Display::Width, k_maxCellHeight, &ellipsisFrame,
                       &inputFrame, &outputFrame);
  KDCoordinate result =
      k_margin + inputFrame.unionedWith(outputFrame).height() + k_margin;
  if (result < 0 || result > k_maxCellHeight) {
    // if result < 0, result overflowed KDCOORDINATE_MAX
    return k_maxCellHeight;
  }
  return result;
}

bool HistoryViewCell::handleEvent(Ion::Events::Event event) {
  assert(m_dataSource != nullptr);
  HistoryViewCellDataSource::SubviewType type =
      m_dataSource->selectedSubviewType();
  assert(type != HistoryViewCellDataSource::SubviewType::None);
  HistoryViewCellDataSource::SubviewType otherSubviewType =
      HistoryViewCellDataSource::SubviewType::None;
  if (m_calculationSingleLine) {
    static_assert(
        static_cast<int>(HistoryViewCellDataSource::SubviewType::None) == 0 &&
            static_cast<int>(HistoryViewCellDataSource::SubviewType::Input) ==
                1 &&
            static_cast<int>(HistoryViewCellDataSource::SubviewType::Output) ==
                2 &&
            static_cast<int>(
                HistoryViewCellDataSource::SubviewType::Ellipsis) == 3,
        "The array types is not well-formed anymore");
    HistoryViewCellDataSource::SubviewType types[] = {
        HistoryViewCellDataSource::SubviewType::None,
        HistoryViewCellDataSource::SubviewType::Input,
        HistoryViewCellDataSource::SubviewType::Output,
        isDisplayingEllipsis()
            ? HistoryViewCellDataSource::SubviewType::Ellipsis
            : HistoryViewCellDataSource::SubviewType::None,
        HistoryViewCellDataSource::SubviewType::None,
    };
    if (event == Ion::Events::Right || event == Ion::Events::Left) {
      otherSubviewType = types[static_cast<int>(type) +
                               (event == Ion::Events::Right ? 1 : -1)];
    }
  } else if ((event == Ion::Events::Down &&
              type == HistoryViewCellDataSource::SubviewType::Input) ||
             (event == Ion::Events::Left &&
              type == HistoryViewCellDataSource::SubviewType::Ellipsis)) {
    otherSubviewType = HistoryViewCellDataSource::SubviewType::Output;
  } else if (event == Ion::Events::Up &&
             type == HistoryViewCellDataSource::SubviewType::Output) {
    otherSubviewType = HistoryViewCellDataSource::SubviewType::Input;
  } else if (event == Ion::Events::Right &&
             type != HistoryViewCellDataSource::SubviewType::Ellipsis &&
             isDisplayingEllipsis()) {
    otherSubviewType = HistoryViewCellDataSource::SubviewType::Ellipsis;
  }
  if (otherSubviewType == HistoryViewCellDataSource::SubviewType::None) {
    return false;
  }
  m_dataSource->setSelectedSubviewType(otherSubviewType, true);
  return true;
}

bool HistoryViewCell::updateExpanded(bool expanded) {
  assert(m_calculationDisplayOutput != Calculation::DisplayOutput::Unknown);
  TrinaryBoolean calculationExpanded = BinaryToTrinaryBool(
      m_calculationDisplayOutput ==
          Calculation::DisplayOutput::ExactAndApproximate ||
      (expanded && m_calculationDisplayOutput ==
                       Calculation::DisplayOutput::ExactAndApproximateToggle));
  if (m_calculationExpanded == calculationExpanded) {
    return false;
  }
  m_calculationExpanded = calculationExpanded;
  m_scrollableOutputView.setDisplayCenter(
      TrinaryToBinaryBool(m_calculationExpanded));
  return true;
}

}  // namespace Calculation
