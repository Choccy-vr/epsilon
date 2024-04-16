#include "localization_controller.h"

#include <apps/apps_container.h>
#include <apps/global_preferences.h>

using namespace Escher;

namespace Shared {

// ContentView
constexpr int
    LocalizationController::ContentView::k_numberOfCountryWarningLines;

LocalizationController::ContentView::ContentView(
    LocalizationController *controller,
    SelectableListViewDataSource *dataSource)
    : m_controller(controller),
      m_selectableListView(controller, controller, dataSource),
      m_countryTitleMessage(I18n::Message::Country),
      m_borderView(Palette::GrayBright) {
  m_countryTitleMessage.setBackgroundColor(Palette::WallScreen);
  m_countryTitleMessage.setAlignment(KDGlyph::k_alignCenter,
                                     KDGlyph::k_alignCenter);
  assert(k_numberOfCountryWarningLines == 2);  // textMessages is not overflowed
  I18n::Message textMessages[k_numberOfCountryWarningLines] = {
      I18n::Message::CountryWarning1, I18n::Message::CountryWarning2};
  for (int i = 0; i < k_numberOfCountryWarningLines; i++) {
    m_countryWarningLines[i].setBackgroundColor(Palette::WallScreen);
    m_countryWarningLines[i].setFont(KDFont::Size::Small);
    m_countryWarningLines[i].setTextColor(Escher::Palette::GrayDark);
    m_countryWarningLines[i].setAlignment(KDGlyph::k_alignCenter,
                                          KDGlyph::k_alignCenter);
    m_countryWarningLines[i].setMessage(textMessages[i]);
  }
}

int LocalizationController::ContentView::numberOfSubviews() const {
  return 1 + m_controller->shouldDisplayTitle() +
         (k_numberOfCountryWarningLines + 1) *
             m_controller->shouldDisplayWarning();
}

View *LocalizationController::ContentView::subviewAtIndex(int i) {
  assert(i < numberOfSubviews());
  /* This relies on the fact that the title is never displayed without the
   * warning. */
  assert((!m_controller->shouldDisplayTitle()) ||
         m_controller->shouldDisplayWarning());
  switch (i) {
    case 0:
      return &m_selectableListView;
    case 3:
      return &m_borderView;
    case 4:
      return &m_countryTitleMessage;
    default:
      return &m_countryWarningLines[i - 1];
  }
}

void LocalizationController::ContentView::modeHasChanged() {
  if (!bounds().isEmpty()) {
    layoutSubviews();
  }
  markWholeFrameAsDirty();
}

void LocalizationController::ContentView::layoutSubviews(bool force) {
  KDCoordinate origin = 0;
  if (m_controller->shouldDisplayTitle()) {
    origin = layoutTitleSubview(force, Metric::CommonMargins.top() + origin);
  }
  if (m_controller->shouldDisplayWarning()) {
    origin =
        layoutWarningSubview(force, Metric::CommonMargins.top() / 2 + origin) +
        Metric::CommonMargins.top() / 2 - 1;
  }
  origin = layoutTableSubview(force, origin);
  assert(origin <= bounds().height());
}

KDCoordinate LocalizationController::ContentView::layoutTitleSubview(
    bool force, KDCoordinate verticalOrigin) {
  KDCoordinate titleHeight = KDFont::GlyphHeight(m_countryTitleMessage.font());
  setChildFrame(&m_countryTitleMessage,
                KDRect(0, verticalOrigin, bounds().width(), titleHeight),
                force);
  return verticalOrigin + titleHeight;
}

KDCoordinate LocalizationController::ContentView::layoutWarningSubview(
    bool force, KDCoordinate verticalOrigin) {
  assert(k_numberOfCountryWarningLines > 0);
  KDCoordinate textHeight =
      KDFont::GlyphHeight(m_countryWarningLines[0].font());
  for (int i = 0; i < k_numberOfCountryWarningLines; i++) {
    setChildFrame(&m_countryWarningLines[i],
                  KDRect(0, verticalOrigin, bounds().width(), textHeight),
                  force);
    verticalOrigin += textHeight;
  }
  return verticalOrigin;
}

KDCoordinate LocalizationController::ContentView::layoutTableSubview(
    bool force, KDCoordinate verticalOrigin) {
  // SelectableListView must be given a width before computing height.
  m_selectableListView.setSize(bounds().size());
  KDCoordinate tableHeight = std::min<KDCoordinate>(
      bounds().height() - verticalOrigin,
      m_selectableListView.minimalSizeForOptimalDisplay().height());

  if (m_controller->shouldDisplayWarning()) {
    setChildFrame(
        &m_borderView,
        KDRect(m_selectableListView.margins()->left(),
               verticalOrigin + m_selectableListView.margins()->top(),
               bounds().width() - m_selectableListView.margins()->width(),
               Metric::CellSeparatorThickness),
        force);
  }
  setChildFrame(&m_selectableListView,
                KDRect(0, verticalOrigin, bounds().width(), tableHeight),
                force);
  return verticalOrigin + tableHeight;
}

// LocalizationController

int LocalizationController::IndexOfCountry(I18n::Country country) {
  /* As we want to order the countries alphabetically in the selected language,
   * the index of a country in the table is the number of other countries that
   * go before it in alphabetical order. */
  int res = 0;
  for (int c = 0; c < I18n::NumberOfCountries; c++) {
    if (country != static_cast<I18n::Country>(c) &&
        strcmp(
            I18n::translate(I18n::CountryNames[static_cast<uint8_t>(country)]),
            I18n::translate(I18n::CountryNames[c])) > 0) {
      res += 1;
    }
  }
  return res;
}

I18n::Country LocalizationController::CountryAtIndex(int i) {
  /* This method is called for each country one after the other, so we could
   * save a lot of computations by memoizing the IndexInTableOfCountry.
   * However, since the number of countries is fairly small, and the country
   * menu is unlikely to be used more than once or twice in the device's
   * lifespan, we skim on memory usage here.*/
  for (int c = 0; c < I18n::NumberOfCountries; c++) {
    I18n::Country country = static_cast<I18n::Country>(c);
    if (i == IndexOfCountry(country)) {
      return country;
    }
  }
  assert(false);
  return (I18n::Country)0;
}

LocalizationController::LocalizationController(
    Responder *parentResponder, LocalizationController::Mode mode)
    : ViewController(parentResponder), m_contentView(this, this), m_mode(mode) {
  setVerticalMargins();
}

void LocalizationController::resetSelection() {
  selectableListView()->deselectTable();
  selectRow(indexOfCellToSelectOnReset());
}

void LocalizationController::setMode(LocalizationController::Mode mode) {
  selectableListView()->deselectTable();
  selectableListView()->resetSizeAndOffsetMemoization();
  m_mode = mode;
  setVerticalMargins();
  m_contentView.modeHasChanged();
}

int LocalizationController::indexOfCellToSelectOnReset() const {
  assert(mode() == Mode::Language);
  return static_cast<int>(
      GlobalPreferences::SharedGlobalPreferences()->language());
}

const char *LocalizationController::title() {
  if (mode() == Mode::Language) {
    return I18n::translate(I18n::Message::Language);
  }
  assert(mode() == Mode::Country);
  return I18n::translate(I18n::Message::Country);
}

void LocalizationController::viewWillAppear() {
  ViewController::viewWillAppear();
  resetSelection();
  selectableListView()->reloadData();
}

bool LocalizationController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    if (mode() == Mode::Language) {
      GlobalPreferences::SharedGlobalPreferences()->setLanguage(
          static_cast<I18n::Language>(selectedRow()));
      AppsContainer::sharedAppsContainer()->reloadTitleBarView();
    } else {
      assert(mode() == Mode::Country);
      GlobalPreferences::SharedGlobalPreferences()->setCountry(
          CountryAtIndex(selectedRow()));
    }
    return true;
  }
  return false;
}

KDCoordinate LocalizationController::nonMemoizedRowHeight(int row) {
  MenuCell<MessageTextView> tempCell;
  return protectedNonMemoizedRowHeight(&tempCell, row);
}

void LocalizationController::fillCellForRow(HighlightCell *cell, int row) {
  if (mode() == Mode::Language) {
    static_cast<MenuCell<MessageTextView> *>(cell)->label()->setMessage(
        I18n::LanguageNames[row]);
    return;
  }
  assert(mode() == Mode::Country);
  static_cast<MenuCell<MessageTextView> *>(cell)->label()->setMessage(
      I18n::CountryNames[static_cast<uint8_t>(CountryAtIndex(row))]);
}

void LocalizationController::setVerticalMargins() {
  KDCoordinate topMargin =
      shouldDisplayWarning() ? 0 : Escher::Metric::CommonMargins.top();
  selectableListView()->margins()->setTop(topMargin);
  // Fit m_selectableListView scroll to content size
  selectableListView()->decorator()->setVerticalMargins(
      {topMargin, Escher::Metric::CommonMargins.bottom()});
}

}  // namespace Shared
