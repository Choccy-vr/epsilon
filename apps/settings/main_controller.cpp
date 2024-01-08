#include "main_controller.h"

#include <apps/global_preferences.h>
#include <apps/i18n.h>
#include <assert.h>

using namespace Poincare;
using namespace Shared;
using namespace Escher;

namespace Settings {

constexpr MessageTree s_modelAngleChildren[3] = {
    MessageTree(I18n::Message::Radian), MessageTree(I18n::Message::Degrees),
    MessageTree(I18n::Message::Gradians)};
constexpr MessageTree s_modelEditionModeChildren[2] = {
    MessageTree(I18n::Message::Edition2D),
    MessageTree(I18n::Message::EditionLinear)};
constexpr MessageTree s_modelFloatDisplayModeChildren[4] = {
    MessageTree(I18n::Message::Decimal), MessageTree(I18n::Message::Scientific),
    MessageTree(I18n::Message::Engineering),
    MessageTree(I18n::Message::SignificantFigures)};
constexpr MessageTree s_modelComplexFormatChildren[3] = {
    MessageTree(I18n::Message::Real), MessageTree(I18n::Message::Cartesian),
    MessageTree(I18n::Message::Exponential)};
constexpr MessageTree s_modelFontChildren[2] = {
    MessageTree(I18n::Message::LargeFont),
    MessageTree(I18n::Message::SmallFont)};
constexpr MessageTree s_modelTestModeMenu[2] = {
    MessageTree(I18n::Message::ExamMode),
    MessageTree(I18n::Message::PressToTest)};
constexpr MessageTree
    s_modelAboutChildren[AboutController::k_totalNumberOfCell] = {
        MessageTree(I18n::Message::Name),
        MessageTree(I18n::Message::SoftwareVersion),
        MessageTree(I18n::Message::SerialNumber),
        MessageTree(I18n::Message::FccId),
#if TERMS_OF_USE
        MessageTree(I18n::Message::TermsOfUse),
#endif
};

MainController::MainController(Responder *parentResponder)
    : SelectableListViewController(parentResponder),
      m_resetButton(&m_selectableListView, I18n::Message::ResetCalculator,
                    Invocation::Builder<MainController>(
                        [](MainController *controller, void *sender) {
                          controller->m_resetController.presentModally();
                          return true;
                        },
                        this)),
      m_preferencesController(this),
      m_displayModeController(this),
      m_localizationController(this, LocalizationController::Mode::Language),
      m_examModeController(this),
      m_pressToTestController(this),
      m_testModeController(this, this),
      m_aboutController(this),
      m_resetController(Invocation::Builder<MainController>(
                            [](MainController *controller, void *sender) {
                              Ion::Reset::core();
                              return true;
                            },
                            this),
                        I18n::Message::ResetCalculatorWarning) {
  // Assert the ExamMode and Press-to-test cells are correctly placed.
  assert(subControllerForCell(messageAtModelIndex(k_indexOfExamModeCell)) ==
         &m_examModeController);
  assert(subControllerForCell(messageAtModelIndex(k_indexOfExamModeCell + 1)) ==
         &m_pressToTestController);
}

bool MainController::handleEvent(Ion::Events::Event event) {
  GlobalPreferences *globalPreferences =
      GlobalPreferences::sharedGlobalPreferences;
  int index = selectedRow();
  int type = typeAtRow(index);
  if (type == k_resetCellType) {
    // Escape now since ButtonCell is not a MenuCell
    return false;
  }

  AbstractMenuCell *cell =
      static_cast<AbstractMenuCell *>(m_selectableListView.cell(index));
  if (!cell->canBeActivatedByEvent(event)) {
    return false;
  }

  if (type == k_popUpCellType) {
    globalPreferences->setShowPopUp(!globalPreferences->showPopUp());
    m_selectableListView.reloadSelectedCell();
  } else if (type == k_brightnessCellType) {
    int delta = Ion::Backlight::MaxBrightness /
                GlobalPreferences::NumberOfBrightnessStates;
    assert(GaugeView::DirectionForEvent(event) != 0);
    globalPreferences->setBrightnessLevel(globalPreferences->brightnessLevel() +
                                          GaugeView::DirectionForEvent(event) *
                                              delta);
    m_selectableListView.reloadSelectedCell();
  } else {
    assert(type == k_defaultCellType);
    int modelIndex = getModelIndex(selectedRow());
    I18n::Message selectedMessage = messageAtModelIndex(modelIndex);

    if (selectedMessage == I18n::Message::Language) {
      m_localizationController.setMode(LocalizationController::Mode::Language);
    } else if (selectedMessage == I18n::Message::Country) {
      m_localizationController.setMode(LocalizationController::Mode::Country);
    }
    pushModel(model()->childAtIndex(modelIndex));
  }
  return true;
}

void MainController::pushModel(const Escher::MessageTree *messageTreeModel) {
  ViewController *selectedSubController =
      subControllerForCell(messageTreeModel->label());
  assert(selectedSubController);
  if (messageTreeModel->numberOfChildren() != 0) {
    static_cast<GenericSubController *>(selectedSubController)
        ->setMessageTreeModel(messageTreeModel);
    static_cast<GenericSubController *>(selectedSubController)
        ->selectableListView()
        ->resetSizeAndOffsetMemoization();
  }
  stackController()->push(selectedSubController);
}

int MainController::numberOfRows() const {
  assert(hasExamModeCell() + hasPressToTestCell() + hasTestModeCell() == 1);
  return model()->numberOfChildren() + hasExamModeCell() +
         hasPressToTestCell() + hasTestModeCell() - 3;
};

KDCoordinate MainController::nonMemoizedRowHeight(int row) {
  switch (typeAtRow(row)) {
    case k_brightnessCellType:
      return protectedNonMemoizedRowHeight(&m_brightnessCell, row);
    case k_popUpCellType:
      return protectedNonMemoizedRowHeight(&m_popUpCell, row);
    case k_resetCellType:
      return protectedNonMemoizedRowHeight(&m_resetButton, row);
    default:
      SubMenuCell tempCell;
      return protectedNonMemoizedRowHeight(&tempCell, row);
  }
}

HighlightCell *MainController::reusableCell(int index, int type) {
  assert(index >= 0);
  if (type == k_defaultCellType) {
    assert(index < k_numberOfSimpleChevronCells);
    return &m_cells[index];
  }
  assert(index == 0);
  if (type == k_popUpCellType) {
    return &m_popUpCell;
  }
  if (type == k_resetCellType) {
    return &m_resetButton;
  }
  assert(type == k_brightnessCellType);
  return &m_brightnessCell;
}

int MainController::reusableCellCount(int type) {
  if (type == k_defaultCellType) {
    return k_numberOfSimpleChevronCells;
  }
  return 1;
}

int MainController::typeAtRow(int row) const {
  switch (messageAtModelIndex(getModelIndex(row))) {
    case I18n::Message::Brightness:
      return k_brightnessCellType;
    case I18n::Message::UpdatePopUp:
    case I18n::Message::BetaPopUp:
      return k_popUpCellType;
    case I18n::Message::ResetCalculator:
      return k_resetCellType;
    default:
      return k_defaultCellType;
  };
}

void MainController::fillCellForRow(HighlightCell *cell, int row) {
  GlobalPreferences *globalPreferences =
      GlobalPreferences::sharedGlobalPreferences;
  Preferences *preferences = Preferences::sharedPreferences;
  int modelIndex = getModelIndex(row);
  I18n::Message title = model()->childAtIndex(modelIndex)->label();
  int type = typeAtRow(row);
  if (type == k_brightnessCellType) {
    assert(&m_brightnessCell == cell);
    m_brightnessCell.label()->setMessage(title);
    m_brightnessCell.accessory()->setLevel(
        (float)globalPreferences->brightnessLevel() /
        (float)Ion::Backlight::MaxBrightness);
    return;
  }
  if (type == k_resetCellType) {
    return;
  }
  static_cast<MessageTextView *>(
      static_cast<AbstractMenuCell *>(cell)->widget(CellWidget::Type::Label))
      ->setMessage(title);
  if (type == k_popUpCellType) {
    assert(cell == &m_popUpCell);
    m_popUpCell.accessory()->setState(globalPreferences->showPopUp());
    return;
  }
  assert(type == k_defaultCellType);
  I18n::Message message = messageAtModelIndex(modelIndex);
  if (message == I18n::Message::Language) {
    int languageIndex = (int)(globalPreferences->language());
    static_cast<SubMenuCell *>(cell)->subLabel()->setMessage(
        I18n::LanguageNames[languageIndex]);
    return;
  }
  if (message == I18n::Message::Country) {
    int countryIndex = (int)(globalPreferences->country());
    static_cast<SubMenuCell *>(cell)->subLabel()->setMessage(
        I18n::CountryNames[countryIndex]);
    return;
  }
  SubMenuCell *myTextCell = static_cast<SubMenuCell *>(cell);
  int childIndex = -1;
  switch (message) {
    case I18n::Message::AngleUnit:
      childIndex = (int)preferences->angleUnit();
      break;
    case I18n::Message::DisplayMode:
      childIndex = (int)preferences->displayMode();
      break;
    case I18n::Message::EditionMode:
      childIndex = (int)preferences->editionMode();
      break;
    case I18n::Message::ComplexFormat:
      childIndex = (int)preferences->complexFormat();
      break;
    case I18n::Message::FontSizes:
      childIndex = GlobalPreferences::sharedGlobalPreferences->font() ==
                           KDFont::Size::Large
                       ? 0
                       : 1;
      break;
    default:
      childIndex = -1;
      break;
  }
  I18n::Message subtitle =
      childIndex >= 0
          ? model()->childAtIndex(modelIndex)->childAtIndex(childIndex)->label()
          : I18n::Message::Default;
  myTextCell->subLabel()->setMessage(subtitle);
}

KDCoordinate MainController::separatorBeforeRow(int row) {
  return typeAtRow(row) == k_brightnessCellType ||
                 typeAtRow(row) == k_resetCellType
             ? k_defaultRowSeparator
             : 0;
}

void MainController::viewWillAppear() {
  ViewController::viewWillAppear();
  m_selectableListView.reloadData();
}

I18n::Message MainController::messageAtModelIndex(int i) const {
  return model()->childAtIndex(i)->label();
}

const MessageTree *MainController::model() { return &s_model; }

StackViewController *MainController::stackController() const {
  return (StackViewController *)parentResponder();
}

ViewController *MainController::subControllerForCell(
    I18n::Message cellMessage) {
  switch (cellMessage) {
    case I18n::Message::AngleUnit:
    case I18n::Message::EditionMode:
    case I18n::Message::ComplexFormat:
    case I18n::Message::FontSizes:
      return &m_preferencesController;
    case I18n::Message::DisplayMode:
      return &m_displayModeController;
    case I18n::Message::Language:
    case I18n::Message::Country:
      return &m_localizationController;
    case I18n::Message::ExamMode:
      return &m_examModeController;
    case I18n::Message::PressToTest:
      return &m_pressToTestController;
    case I18n::Message::TestMode:
      return &m_testModeController;
    case I18n::Message::About:
      return &m_aboutController;
    default:
      return nullptr;
  }
}

bool MainController::hasExamModeCell() const {
  // If only exam modes are available
  return !hasTestModeCell() && m_examModeController.numberOfRows() > 0;
}

bool MainController::hasPressToTestCell() const {
  // If only press to test is available
  return m_examModeController.numberOfRows() == 0;
}

bool MainController::hasTestModeCell() const {
  // If both exam mode and press to test are available
  CountryPreferences::AvailableExamModes examMode =
      GlobalPreferences::sharedGlobalPreferences->availableExamModes();
  return (examMode == CountryPreferences::AvailableExamModes::All ||
          examMode == CountryPreferences::AvailableExamModes::AmericanAll) &&
         Preferences::sharedPreferences->examMode().ruleset() ==
             ExamMode::Ruleset::Off;
}

int MainController::getModelIndex(int index) const {
  /* Return the index of the model from the index of the displayed row.
   * Up until k_indexOfExamModeCell, no cell is hidden, the index is the same.
   * Then, either the exam mode or the press-to-test cell is hidden. */
  assert(index >= 0 && index < numberOfRows());
  if (index > k_indexOfExamModeCell) {
    // 2 of the 3 exam mode cells are hidden.
    index += 2;
  } else if (index == k_indexOfExamModeCell) {
    if (!hasExamModeCell()) {
      // Hidden exam mode cell
      index += 1;
      if (!hasPressToTestCell()) {
        // Hidden press-to-test cell
        assert(hasTestModeCell());
        index += 1;
      }
    }
  }
  assert(index < model()->numberOfChildren());
  return index;
}

}  // namespace Settings
