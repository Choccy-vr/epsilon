#include "lcd_timing_test_controller.h"
#include <ion/backlight.h>
#include <ion/display.h>
#include <ion/post_and_hardware_tests.h>
#include <poincare/print_int.h>

using namespace Poincare;

namespace HardwareTest {

void LCDTimingTestController::runTest() {
  int pixelFailures = Ion::POSTAndHardwareTests::LCDTimingGlyphFailures();
  m_testSuccessful = pixelFailures <= k_errorLimit;
  m_view.setStatus(m_testSuccessful, pixelFailures);
}

bool LCDTimingTestController::handleEvent(Ion::Events::Event event) {
  // Do not handle OnOff event to let the apps container redraw the screen
  if (event == Ion::Events::OnOff) {
    return false;
  }
  if (event == Ion::Events::OK && m_testSuccessful) {
    // Handled in WizardViewController
    return false;
  }
  return true;
}

void LCDTimingTestController::viewWillAppear() {
  Ion::Display::pushRectUniform(KDRect(KDPointZero, Ion::Display::Width, Ion::Display::Height), KDColorWhite);
  Ion::Backlight::setBrightness(0);
  runTest();
  Ion::Backlight::setBrightness(Ion::Backlight::MaxBrightness);
}

LCDTimingTestController::ContentView::ContentView() :
  SolidColorView(KDColorWhite),
  m_lcdTimingStateView(KDFont::LargeFont, KDContext::k_alignCenter, KDContext::k_alignCenter),
  m_lcdNumberGlyphFailuresView(KDFont::SmallFont, KDContext::k_alignCenter, KDContext::k_alignCenter)
{
}

void LCDTimingTestController::ContentView::setStatus(bool success, int numberOfErrors) {
  KDColor backgroundColor = (success ? KDColorGreen : KDColorRed);
  m_lcdTimingStateView.setBackgroundColor(backgroundColor);
  m_lcdNumberGlyphFailuresView.setBackgroundColor(backgroundColor);
  m_lcdTimingStateView.setText(success ? k_lcdTimingPassTest : k_lcdTimingFailTest);
  constexpr int bufferSize = 20;
  char buffer[bufferSize] = {0};
  Poincare::PrintInt::Left(numberOfErrors, buffer, bufferSize);
  m_lcdNumberGlyphFailuresView.setText(buffer);
}

void LCDTimingTestController::ContentView::layoutSubviews(bool force) {
  m_lcdTimingStateView.setFrame(KDRect(0, 0, Ion::Display::Width, Ion::Display::Height), force);
  m_lcdNumberGlyphFailuresView.setFrame(KDRect(10, 10, Ion::Display::Width, 20), force);
}

}
