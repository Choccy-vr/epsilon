#include "console_controller.h"

#include <apps/apps_container.h>
#include <apps/global_preferences.h>
#include <apps/i18n.h>
#include <assert.h>
#include <escher/metric.h>
#include <ion/storage/file_system.h>
#include <python/port/helpers.h>

#include <algorithm>

#include "app.h"
#include "python_variable_box_controller.h"
#include "script.h"

extern "C" {
#include <stdlib.h>
}

using namespace Escher;

namespace Code {

constexpr static const char *sStandardPromptText = ">>> ";

ConsoleController::ConsoleController(Responder *parentResponder,
                                     App *pythonDelegate
#if EPSILON_GETOPT
                                     ,
                                     bool lockOnConsole
#endif
                                     )
    : ViewController(parentResponder),
      SelectableListViewDataSource(),
      TextFieldDelegate(),
      MicroPython::ExecutionEnvironment(),
      m_pythonDelegate(pythonDelegate),
      m_importScriptsWhenViewAppears(false),
      m_selectableListView(this, this, this, this),
      m_editCell(this, this),
      m_sandboxController(this),
      m_inputRunLoopActive(false)
#if EPSILON_GETOPT
      ,
      m_locked(lockOnConsole)
#endif
{
  m_selectableListView.setMargins({Metric::TitleBarExternHorizontalMargin,
                                   Metric::CommonMargins.right(), 0, 0});
  m_selectableListView.setBackgroundColor(KDColorWhite);
  m_editCell.setPrompt(sStandardPromptText);
  for (int i = 0; i < k_numberOfLineCells; i++) {
    m_cells[i].setParentResponder(&m_selectableListView);
  }
}

bool ConsoleController::loadPythonEnvironment() {
  if (!m_pythonDelegate->isPythonUser(this)) {
    ScriptStore::ClearConsoleFetchInformation();
    emptyOutputAccumulationBuffer();
    m_pythonDelegate->initPythonWithUser(this);
    MicroPython::registerScriptProvider(&m_scriptStore);
    m_importScriptsWhenViewAppears = m_autoImportScripts;
  }
  return true;
}

void ConsoleController::unloadPythonEnvironment() {
  if (!m_pythonDelegate->isPythonUser(nullptr)) {
    m_consoleStore.startNewSession();
    m_pythonDelegate->deinitPython();
  }
}

void ConsoleController::autoImport() {
  int numberOfScripts = ScriptStore::NumberOfScripts();
  for (int i = 0; i < numberOfScripts; i++) {
    autoImportScript(ScriptStore::ScriptAtIndex(i));
  }
}

void ConsoleController::runAndPrintForCommand(const char *command) {
  const char *storedCommand = m_consoleStore.pushCommand(command);
  assert(m_outputAccumulationBuffer[0] == '\0');

  // Draw the console before running the code
  m_editCell.setText("");
  m_editCell.setPrompt("");
  refreshPrintOutput();

  runCode(storedCommand);

  m_editCell.setText("");
  m_editCell.setPrompt(sStandardPromptText);
  m_editCell.setEditing(true);

  flushOutputAccumulationBufferToStore();
  m_consoleStore.deleteLastLineIfEmpty();
}

void ConsoleController::terminateInputLoop() {
  assert(m_inputRunLoopActive);
  m_inputRunLoopActive = false;
  interrupt();
}

const char *ConsoleController::inputText(const char *prompt) {
  AppsContainer *appsContainer = AppsContainer::sharedAppsContainer();
  m_inputRunLoopActive = true;

  // Hide the sandbox if it is displayed
  hideAnyDisplayedViewController();

  const char *promptText = prompt;
  char *s = const_cast<char *>(prompt);

  if (promptText != nullptr) {
    /* Set the prompt text. If the prompt text has a '\n', put the prompt text
     * in the history until the last '\n', and put the remaining prompt text in
     * the edit cell's prompt. */
    char *lastCarriageReturn = nullptr;
    while (*s != 0) {
      if (*s == '\n') {
        lastCarriageReturn = s;
      }
      s++;
    }
    if (lastCarriageReturn != nullptr) {
      printText(prompt, lastCarriageReturn - prompt + 1);
      promptText = lastCarriageReturn + 1;
    }
  }

  m_editCell.setPrompt(promptText);
  m_editCell.setText("");

  // Reload the history
  reloadData();
  appsContainer->redrawWindow();

  // Launch a new input loop
  appsContainer->runWhile(
      [](void *a) {
        ConsoleController *c = static_cast<ConsoleController *>(a);
        return c->inputRunLoopActive();
      },
      this);

  // Print the prompt and the input text
  if (promptText != nullptr) {
    printText(promptText, s - promptText);
  }
  const char *text = m_editCell.text();
  size_t textSize = strlen(text);
  printText(text, textSize);
  flushOutputAccumulationBufferToStore();
  refreshPrintOutput();
  return text;
}

void ConsoleController::viewWillAppear() {
  ViewController::viewWillAppear();
  loadPythonEnvironment();
  if (m_importScriptsWhenViewAppears) {
    m_importScriptsWhenViewAppears = false;
    autoImport();
  }

  reloadData();
}

void ConsoleController::didBecomeFirstResponder() {
  if (!isDisplayingViewController()) {
    App::app()->setFirstResponder(&m_editCell);
  } else {
    /* A view controller might be displayed: for example, when pushing the
     * console on the stack controller, we auto-import scripts during the
     * 'viewWillAppear' and then we set the console as first responder. The
     * sandbox or the matplotlib controller might have been pushed in the
     * auto-import. */
    App::app()->setFirstResponder(stackViewController()->topViewController());
  }
}

bool ConsoleController::handleEvent(Ion::Events::Event event) {
  if (event == Ion::Events::OK || event == Ion::Events::EXE) {
    if (m_consoleStore.numberOfLines() > 0 &&
        typeAtRow(selectedRow()) == k_lineCellType) {
      const char *text = m_consoleStore.lineAtIndex(selectedRow()).text();
      m_editCell.setEditing(true);
      m_selectableListView.selectCell(m_consoleStore.numberOfLines());
      App::app()->setFirstResponder(&m_editCell);
      return m_editCell.insertText(text);
    }
  } else if (event == Ion::Events::Clear) {
    m_selectableListView.deselectTable();
    m_consoleStore.clear();
    m_selectableListView.reloadData();
    m_selectableListView.selectCell(m_consoleStore.numberOfLines());
    return true;
  } else if (event == Ion::Events::Backspace) {
    int rowToDelete = selectedRow();
    assert(typeAtRow(rowToDelete) == k_lineCellType);
    m_selectableListView.deselectTable();
    int firstDeletedLineIndex =
        m_consoleStore.deleteCommandAndResultsAtIndex(rowToDelete);
    m_selectableListView.reloadData();
    m_selectableListView.selectCell(firstDeletedLineIndex);
    return true;
  }
#if EPSILON_GETOPT

  if (m_locked && (event == Ion::Events::USBEnumeration ||
                   event == Ion::Events::Home || event == Ion::Events::Back)) {
    if (m_inputRunLoopActive) {
      terminateInputLoop();
    }
    return true;
  }
#endif
  return false;
}

int ConsoleController::numberOfRows() const { return editableCellRow() + 1; }

KDCoordinate ConsoleController::defaultRowHeight() {
  return KDFont::GlyphHeight(
      GlobalPreferences::sharedGlobalPreferences->font());
}

HighlightCell *ConsoleController::reusableCell(int index, int type) {
  assert(index >= 0);
  if (type == k_lineCellType) {
    assert(index < k_numberOfLineCells);
    return m_cells + index;
  } else {
    assert(type == k_editCellType);
    assert(index == 0);
    return &m_editCell;
  }
}

int ConsoleController::reusableCellCount(int type) {
  if (type == k_lineCellType) {
    return k_numberOfLineCells;
  } else {
    return 1;
  }
}

int ConsoleController::typeAtRow(int row) const {
  assert(row >= 0);
  if (row < editableCellRow()) {
    return k_lineCellType;
  }
  assert(row == editableCellRow());
  return k_editCellType;
}

void ConsoleController::fillCellForRow(HighlightCell *cell, int row) {
  if (typeAtRow(row) == k_lineCellType) {
    static_cast<ConsoleLineCell *>(cell)->setLine(
        m_consoleStore.lineAtIndex(row));
  }
}

void ConsoleController::listViewDidChangeSelectionAndDidScroll(
    Escher::SelectableListView *l, int previousSelectedRow,
    KDPoint previousOffset, bool withinTemporarySelection) {
  assert(l == &m_selectableListView);
  if (withinTemporarySelection) {
    return;
  }
  if (selectedRow() == editableCellRow()) {
    m_editCell.setEditing(true);
    return;
  }
  if (selectedRow() > -1) {
    if (previousSelectedRow > -1 &&
        typeAtRow(previousSelectedRow) == k_lineCellType) {
      // Reset the scroll of the previous cell
      ConsoleLineCell *previousCell =
          (ConsoleLineCell *)(l->cell(previousSelectedRow));
      if (previousCell) {
        previousCell->reloadCell();
      }
    }
    ConsoleLineCell *selectedCell = (ConsoleLineCell *)(l->selectedCell());
    if (selectedCell) {
      selectedCell->reloadCell();
    }
  }
}

bool ConsoleController::textFieldShouldFinishEditing(
    AbstractTextField *textField, Ion::Events::Event event) {
  assert(textField->isEditing());
  return textField->draftTextLength() > 0 &&
         TextFieldDelegate::textFieldShouldFinishEditing(textField, event);
}

bool ConsoleController::textFieldDidReceiveEvent(AbstractTextField *textField,
                                                 Ion::Events::Event event) {
  if (event == Ion::Events::Var) {
    prepareVariableBox();
  }
  if (m_inputRunLoopActive &&
      (event == Ion::Events::Up || event == Ion::Events::OK ||
       event == Ion::Events::EXE)) {
    m_inputRunLoopActive = false;
    /* We need to return true here because we want to actually exit from the
     * input run loop, which requires ending a dispatchEvent cycle. */
    return true;
  }
  if (event == Ion::Events::Up) {
    if (m_consoleStore.numberOfLines() > 0 &&
        typeAtRow(selectedRow()) == k_editCellType) {
      m_editCell.setEditing(false);
      m_selectableListView.selectCell(m_consoleStore.numberOfLines() - 1);
      return true;
    }
  }
  return App::app()->textInputDidReceiveEvent(textField, event);
}

bool ConsoleController::textFieldDidFinishEditing(AbstractTextField *textField,
                                                  Ion::Events::Event event) {
  if (m_inputRunLoopActive) {
    m_inputRunLoopActive = false;
    return false;
  }
  char *text = textField->draftText();
  telemetryReportEvent("Console", text);
  runAndPrintForCommand(text);
  if (!isDisplayingViewController()) {
    reloadData();
  }
  return true;
}

void ConsoleController::textFieldDidAbortEditing(AbstractTextField *textField) {
  if (m_inputRunLoopActive) {
    m_inputRunLoopActive = false;
  } else {
#if EPSILON_GETOPT
    /* In order to lock the console controller, we disable poping controllers
     * below the console controller included. The stack should only hold:
     * - the menu controller
     * - the console controller
     * The depth of the stack controller must always be above or equal to 2. */
    if (!m_locked || stackViewController()->depth() > 2) {
#endif
      stackViewController()->pop();
#if EPSILON_GETOPT
    } else {
      textField->setEditing(true);
    }
#endif
  }
}

void ConsoleController::prepareVariableBox() {
  PythonVariableBoxController *varBox = App::app()->variableBox();
  // Subtitle display status must be set before as it alter loaded node order
  varBox->setDisplaySubtitles(false);
  varBox->loadVariablesImportedFromScripts();
  varBox->setTitle(I18n::Message::FunctionsAndVariables);
}

void ConsoleController::resetSandbox() {
  if (stackViewController()->topViewController() != sandbox()) {
    return;
  }
  m_sandboxController.reset();
}

void ConsoleController::displayViewController(ViewController *controller) {
  if (stackViewController()->topViewController() == controller) {
    return;
  }
  hideAnyDisplayedViewController();
  stackViewController()->push(controller);
}

void ConsoleController::hideAnyDisplayedViewController() {
  if (!isDisplayingViewController()) {
    return;
  }
  stackViewController()->pop();
}

bool ConsoleController::isDisplayingViewController() {
  /* The StackViewController model state is the best way to know wether the
   * console is displaying a View Controller (Sandbox or Matplotlib). Indeed,
   * keeping a boolean or a pointer raises the issue of when updating it - when
   * 'viewWillAppear' or when 'didEnterResponderChain' - in both cases, the
   * state would be wrong at some point... */
  return stackViewController()->depth() > 2;
}

void ConsoleController::refreshPrintOutput() {
  if (!isDisplayingViewController()) {
    reloadData();
    AppsContainer::sharedAppsContainer()->redrawWindow();
  }
}

void ConsoleController::reloadData() {
  m_selectableListView.reloadData();
  m_selectableListView.selectCell(m_consoleStore.numberOfLines());
}

/* printText is called by the Python machine.
 * The text argument is not always null-terminated. */
void ConsoleController::printText(const char *text, size_t length) {
  size_t textCutIndex = firstNewLineCharIndex(text, length);
  if (textCutIndex >= length) {
    /* If there is no new line in text, just append it to the output
     * accumulation buffer. */
    appendTextToOutputAccumulationBuffer(text, length);
  } else {
    if (textCutIndex < length - 1) {
      /* If there is a new line in the middle of the text, we have to store at
       * least two new console lines in the console store. */
      printText(text, textCutIndex + 1);
      printText(&text[textCutIndex + 1], length - (textCutIndex + 1));
      return;
    }
    /* There is a new line at the end of the text, we have to store the line in
     * the console store. */
    assert(textCutIndex == length - 1);
    appendTextToOutputAccumulationBuffer(text, length - 1);
    flushOutputAccumulationBufferToStore();
    micropython_port_vm_hook_refresh_print();
  }
}

void ConsoleController::autoImportScript(Script script, bool force) {
  /* The sandbox might be displayed, for instance if we are auto-importing
   * several scripts that draw at importation. In this case, we want to remove
   * the sandbox. */
  hideAnyDisplayedViewController();

  if (script.autoImportation() || force) {
    // Step 1 - Create the command "from scriptName import *".

    assert(strlen(k_importCommand1) + script.name().baseNameLength +
               strlen(k_importCommand2) + 1 <=
           k_maxImportCommandSize);
    char command[k_maxImportCommandSize];

    // Copy "from "
    size_t currentChar =
        strlcpy(command, k_importCommand1, k_maxImportCommandSize);
    Ion::Storage::Record::Name scriptName = script.name();

    /* Copy the script name without the extension ".py". The '.' is overwritten
     * by the null terminating char. */
    int copySizeWithNullTerminatingZero = std::min(
        k_maxImportCommandSize - currentChar, scriptName.baseNameLength + 1);
    assert(copySizeWithNullTerminatingZero >= 0);
    assert(copySizeWithNullTerminatingZero <=
           static_cast<int>(k_maxImportCommandSize - currentChar));
    strlcpy(command + currentChar, scriptName.baseName,
            copySizeWithNullTerminatingZero);
    currentChar += copySizeWithNullTerminatingZero - 1;

    // Copy " import *"
    assert(k_maxImportCommandSize >= currentChar);
    strlcpy(command + currentChar, k_importCommand2,
            k_maxImportCommandSize - currentChar);

    // Step 2 - Run the command
    runAndPrintForCommand(command);
  }
  if (!isDisplayingViewController() && force) {
    reloadData();
  }
}

void ConsoleController::flushOutputAccumulationBufferToStore() {
  m_consoleStore.pushResult(m_outputAccumulationBuffer);
  emptyOutputAccumulationBuffer();
}

void ConsoleController::appendTextToOutputAccumulationBuffer(const char *text,
                                                             size_t length) {
  constexpr static int k_maxLength = k_outputAccumulationBufferSize - 1;
  size_t lengthOfAccumulatedText = strlen(m_outputAccumulationBuffer);
  size_t spaceLeft = k_maxLength - lengthOfAccumulatedText;
  if (length <= spaceLeft) {
    memcpy(&m_outputAccumulationBuffer[lengthOfAccumulatedText], text, length);
    m_outputAccumulationBuffer[lengthOfAccumulatedText + length] = 0;
    return;
  }

  /* If the text is too long for the accumulation buffer, truncate it
   * and add "..." at the end */
  memcpy(&m_outputAccumulationBuffer[lengthOfAccumulatedText], text, spaceLeft);
  m_outputAccumulationBuffer[k_maxLength] = 0;
  constexpr static size_t k_strLenOfDots = 3 * sizeof('.');
  int indexOfDots = k_maxLength - k_strLenOfDots;
  while (UTF8Decoder::IsInTheMiddleOfACodePoint(
      m_outputAccumulationBuffer[indexOfDots])) {
    indexOfDots--;
  }
  assert(indexOfDots > 0);
  for (int i = indexOfDots; i < k_maxLength; i++) {
    m_outputAccumulationBuffer[i] = '.';
  }
}

// TODO: is it really needed? Maybe discard to optimize?
void ConsoleController::emptyOutputAccumulationBuffer() {
  for (int i = 0; i < k_outputAccumulationBufferSize; i++) {
    m_outputAccumulationBuffer[i] = 0;
  }
}

size_t ConsoleController::firstNewLineCharIndex(const char *text,
                                                size_t length) {
  size_t index = 0;
  while (index < length) {
    if (text[index] == '\n') {
      return index;
    }
    index++;
  }
  return index;
}

StackViewController *ConsoleController::stackViewController() {
  return static_cast<StackViewController *>(parentResponder());
}

}  // namespace Code
