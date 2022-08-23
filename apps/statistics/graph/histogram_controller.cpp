#include "histogram_controller.h"
#include <apps/shared/poincare_helpers.h>
#include "../app.h"
#include <poincare/ieee754.h>
#include <poincare/preferences.h>
#include <poincare/print.h>
#include <algorithm>
#include <cmath>
#include <assert.h>
#include <float.h>

using namespace Poincare;
using namespace Shared;
using namespace Escher;

namespace Statistics {

HistogramController::HistogramController(Responder * parentResponder, InputEventHandlerDelegate * inputEventHandlerDelegate, ButtonRowController * header, Responder * tabController, Escher::StackViewController * stackViewController, Escher::ViewController * typeViewController, Store * store, uint32_t * storeVersion) :
  MultipleDataViewController(parentResponder, tabController, header, stackViewController, typeViewController, store),
  m_view(store, &m_histogramRange),
  m_histogramRange(store),
  m_storeVersion(storeVersion),
  m_histogramParameterController(nullptr, inputEventHandlerDelegate, store),
  m_parameterButton(this, I18n::Message::StatisticsGraphSettings, Invocation([](void * context, void * sender) {
    HistogramController * histogramController = static_cast<HistogramController * >(context);
    histogramController->stackController()->push(histogramController->histogramParameterController());
    return true;
  }, this), KDFont::SmallFont)
{
}

Button * HistogramController::buttonAtIndex(int index, ButtonRowController::Position position) const {
  return index == 0 ? GraphButtonRowDelegate::buttonAtIndex(index, position) : const_cast<Button *>(&m_parameterButton);
}

bool HistogramController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::Toolbox) {
    stackController()->push(histogramParameterController());
    return true;
  }
  return MultipleDataViewController::handleEvent(event);
}

void HistogramController::viewWillAppearBeforeReload() {
  multipleDataView()->setDisplayBanner(true);
  multipleDataView()->selectDataView(m_selectedSeries);
  highlightSelection();

  uint32_t storeChecksum = m_store->storeChecksum();
  if (*m_storeVersion != storeChecksum) {
    *m_storeVersion = storeChecksum;
    initBarParameters();
  }

  initRangeParameters();
  sanitizeSelectedIndex();
}

void HistogramController::highlightSelection() {
  HistogramView * selectedHistogramView = static_cast<HistogramView *>(m_view.dataViewAtIndex(m_selectedSeries));
  selectedHistogramView->setHighlight(m_store->startOfBarAtIndex(m_selectedSeries, m_selectedIndex), m_store->endOfBarAtIndex(m_selectedSeries, m_selectedIndex));
  // if the selectedBar was outside of range, we need to scroll
  if (m_histogramRange.scrollToSelectedBarIndex(m_selectedSeries, m_selectedIndex)) {
    multipleDataView()->reload();
  }
}

bool HistogramController::reloadBannerView() {
  if (m_selectedSeries < 0) {
    return false;
  }
  KDCoordinate previousHeight = m_view.bannerView()->minimalSizeForOptimalDisplay().height();

  int precision = Preferences::sharedPreferences()->numberOfSignificantDigits();
  Poincare::Preferences::PrintFloatMode displayMode = Poincare::Preferences::sharedPreferences()->displayMode();
  // With 7 = KDFont::SmallFont->glyphSize().width()
  constexpr static int k_bufferSize = 1 + Ion::Display::Width / 7;
  char buffer[k_bufferSize] = "";

  // Display series name
  StoreController::FillSeriesName(m_selectedSeries, buffer, false);
  m_view.bannerView()->seriesName()->setText(buffer);

  // Display interval
  double lowerBound = m_store->startOfBarAtIndex(m_selectedSeries, m_selectedIndex);
  double upperBound = m_store->endOfBarAtIndex(m_selectedSeries, m_selectedIndex);
  Poincare::Print::customPrintf(
    buffer,
    k_bufferSize,
    "%s%s[%*.*ed;%*.*ed[",
    I18n::translate(I18n::Message::Interval),
    I18n::translate(I18n::Message::StatisticsColonConvention),
    lowerBound, displayMode, precision,
    upperBound, displayMode, precision);
  m_view.bannerView()->intervalView()->setText(buffer);

  // Display frequency
  double size = m_store->heightOfBarAtIndex(m_selectedSeries, m_selectedIndex);
  Poincare::Print::customPrintf(
    buffer,
    k_bufferSize,
    "%s%s%*.*ed",
    I18n::translate(I18n::Message::Frequency),
    I18n::translate(I18n::Message::StatisticsColonConvention),
    size, displayMode, precision);
  m_view.bannerView()->frequencyView()->setText(buffer);

  // Display relative frequency
  double relativeFrequency = size/m_store->sumOfOccurrences(m_selectedSeries);
  Poincare::Print::customPrintf(
    buffer,
    k_bufferSize,
    "%s%s%*.*ed",
    I18n::translate(I18n::Message::RelativeFrequency),
    I18n::translate(I18n::Message::StatisticsColonConvention),
    relativeFrequency, displayMode, precision);
  m_view.bannerView()->relativeFrequencyView()->setText(buffer);

  m_view.bannerView()->reload();
  return previousHeight != m_view.bannerView()->minimalSizeForOptimalDisplay().height();
}

bool HistogramController::moveSelectionHorizontally(int deltaIndex) {
  int numberOfBars = m_store->numberOfBars(m_selectedSeries);
  int newSelectedBarIndex = m_selectedIndex;
  do {
    newSelectedBarIndex += deltaIndex;
  } while (m_store->heightOfBarAtIndex(m_selectedSeries, newSelectedBarIndex) == 0 && newSelectedBarIndex >= 0 && newSelectedBarIndex < numberOfBars);

  if (newSelectedBarIndex >= 0 && newSelectedBarIndex < numberOfBars && m_selectedIndex != newSelectedBarIndex) {
    m_selectedIndex = newSelectedBarIndex;
    m_view.dataViewAtIndex(m_selectedSeries)->setHighlight(m_store->startOfBarAtIndex(m_selectedSeries, m_selectedIndex), m_store->endOfBarAtIndex(m_selectedSeries, m_selectedIndex));
    return true;
  }
  return false;
}

void HistogramController::preinitXRangeParameters(double * xMin, double * xMax) {
  /* Compute m_store's min and max values, hold them temporarily in the
   * CurveViewRange, for later use by initRangeParameters and
   * initBarParameters. Indeed, initRangeParameters will anyway alter the
   * CurveViewRange. The CurveViewRange setter methods take care of the case
   * where minValue >= maxValue. Moreover they compute the xGridUnit, which is
   * used by initBarParameters. */
  double minValue = DBL_MAX;
  double maxValue = -DBL_MAX;
  for (int i = 0; i < Store::k_numberOfSeries; i ++) {
    if (m_store->seriesIsValid(i)) {
      minValue = std::min<double>(minValue, m_store->minValue(i));
      maxValue = std::max<double>(maxValue, m_store->maxValue(i));
    }
  }
  assert(xMin != nullptr);
  *xMin = minValue;
  if (xMax != nullptr) {
    *xMax = maxValue;
  }
  m_histogramRange.setHistogramXMin(minValue, false);
  m_histogramRange.setHistogramXMax(maxValue, true);
}

void HistogramController::initRangeParameters() {
  assert(m_store->seriesIsValid(m_selectedSeries));
  double barWidth = m_store->barWidth();
  double xMin;
  preinitXRangeParameters(&xMin);
  double xMax = m_histogramRange.xMax() + barWidth;
  /* if a bar is represented by less than one pixel, we cap xMax */
  if ((xMax - xMin)/barWidth > k_maxNumberOfBarsPerWindow) {
    xMax = xMin + k_maxNumberOfBarsPerWindow*barWidth;
  }
  m_histogramRange.setHistogramXMin(xMin - HistogramRange::k_displayLeftMarginRatio*(xMax-xMin), false);
  m_histogramRange.setHistogramXMax(xMax + HistogramRange::k_displayRightMarginRatio*(xMax-xMin), true);

  initYRangeParameters(m_selectedSeries);
}

void HistogramController::initYRangeParameters(int series) {
  assert(m_store->seriesIsValid(series));
  /* Height of drawn bar are relative to the maximal bar of the series, so all
   * displayed series need the same range of [0,1]. */
  m_histogramRange.setYMax(1.0f+HistogramRange::k_displayTopMarginRatio);

  /* Compute YMin:
   *    ratioFloatPixel*(0-yMin) = bottomMargin
   *    ratioFloatPixel*(yMax-yMin) = viewHeight
   *
   *    -ratioFloatPixel*yMin = bottomMargin
   *    ratioFloatPixel*yMax-ratioFloatPixel*yMin = viewHeight
   *
   *    ratioFloatPixel = (viewHeight - bottomMargin)/yMax
   *    yMin = -bottomMargin/ratioFloatPixel = yMax*bottomMargin/(bottomMargin - viewHeight)
   * */
  float bottomMargin = static_cast<float>(HistogramRange::k_bottomMargin);
  // viewHeight should be equal for each valid series
  float viewHeight = static_cast<float>(m_view.dataViewAtIndex(series)->bounds().height());
  m_histogramRange.setYMin(m_histogramRange.yMax() * bottomMargin / (bottomMargin - viewHeight));
}

void HistogramController::initBarParameters() {
  assert(m_store->seriesIsValid(m_selectedSeries));
  double xMin;
  double xMax;
  preinitXRangeParameters(&xMin, &xMax);
  m_store->setFirstDrawnBarAbscissa(xMin);
  double barWidth = m_histogramRange.xGridUnit();
  if (barWidth <= 0.0) {
    barWidth = 1.0;
  } else {
    // Round the bar width, as we convert from float to double
    const double precision = 7.0; // FLT_EPS ~= 1e-7
    const double logBarWidth = IEEE754<double>::exponentBase10(barWidth);
    const double truncateFactor = std::pow(10.0, precision - logBarWidth);
    barWidth = std::round(barWidth * truncateFactor) / truncateFactor;
  }
  if (std::ceil((xMax - xMin) / barWidth) > HistogramRange::k_maxNumberOfBars) {
    // Use k_maxNumberOfBars - 1 for extra margin in case of a loss of precision
    barWidth = (xMax - xMin) / (HistogramRange::k_maxNumberOfBars - 1);
  }
  assert(barWidth > 0.0 && std::ceil((xMax - xMin) / barWidth) <= HistogramRange::k_maxNumberOfBars);
  m_store->setBarWidth(barWidth);
}

void HistogramController::sanitizeSelectedIndex() {
  assert(m_store->seriesIsValid(m_selectedSeries));
  if (m_store->heightOfBarAtIndex(m_selectedSeries, m_selectedIndex) != 0) {
    return;
  }
  // Select the first visible bar
  m_selectedIndex = 0;
  int numberOfBars = m_store->numberOfBars(m_selectedSeries);
  while (m_store->heightOfBarAtIndex(m_selectedSeries, m_selectedIndex) == 0 && m_selectedIndex < numberOfBars) {
    m_selectedIndex++;
  }
  assert(m_selectedIndex < numberOfBars);
  m_histogramRange.scrollToSelectedBarIndex(m_selectedSeries, m_selectedIndex);
}

}
