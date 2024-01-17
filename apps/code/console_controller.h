#ifndef CODE_CONSOLE_CONTROLLER_H
#define CODE_CONSOLE_CONTROLLER_H

#include <escher/list_view_data_source.h>
#include <python/port/port.h>

#include "console_edit_cell.h"
#include "console_line_cell.h"
#include "console_store.h"
#include "python_variable_box_controller.h"
#include "sandbox_controller.h"
#include "script_store.h"

namespace Code {

class App;

class ConsoleController : public Escher::ViewController,
                          public Escher::RegularListViewDataSource,
                          public Escher::SelectableListViewDataSource,
                          public Escher::SelectableListViewDelegate,
                          public Escher::TextFieldDelegate,
                          public MicroPython::ExecutionEnvironment {
 public:
  ConsoleController(Escher::Responder* parentResponder, App* pythonDelegate
#if EPSILON_GETOPT
                    ,
                    bool m_lockOnConsole
#endif
  );

  bool loadPythonEnvironment();
  void unloadPythonEnvironment();

  void setAutoImport(bool autoImport) { m_autoImportScripts = autoImport; }
  void autoImport();
  void autoImportScript(Script script, bool force = false);
  void runAndPrintForCommand(const char* command);
  bool inputRunLoopActive() const { return m_inputRunLoopActive; }
  void terminateInputLoop();

  // ViewController
  Escher::View* view() override { return &m_selectableListView; }
  void viewWillAppear() override;
  void didBecomeFirstResponder() override;
  bool handleEvent(Ion::Events::Event event) override;
  Escher::ViewController::TitlesDisplay titlesDisplay() override {
    return Escher::ViewController::TitlesDisplay::DisplayNoTitle;
  }
  TELEMETRY_ID("Console");

  // ListViewDataSource
  int numberOfRows() const override;
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) const override;
  int typeAtRow(int row) const override;
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;

  // SelectableTableViewDelegate
  void listViewDidChangeSelectionAndDidScroll(
      Escher::SelectableListView* l, int previousSelectedRow,
      KDPoint previousOffset, bool withinTemporarySelection) override;

  // TextFieldDelegate
  bool textFieldShouldFinishEditing(Escher::AbstractTextField* textField,
                                    Ion::Events::Event event) override;
  bool textFieldDidReceiveEvent(Escher::AbstractTextField* textField,
                                Ion::Events::Event event) override;
  bool textFieldDidFinishEditing(Escher::AbstractTextField* textField,
                                 Ion::Events::Event event) override;
  void textFieldDidAbortEditing(Escher::AbstractTextField* textField) override;

  // MicroPython::ExecutionEnvironment
  Escher::ViewController* sandbox() override { return &m_sandboxController; }
  void resetSandbox() override;
  void displayViewController(Escher::ViewController* controller) override;
  void hideAnyDisplayedViewController() override;
  void refreshPrintOutput() override;
  void printText(const char* text, size_t length) override;
  const char* inputText(const char* prompt) override;

#if EPSILON_GETOPT
  bool locked() const { return m_locked; }
#endif
 private:
  constexpr static const char* k_importCommand1 = "from ";
  constexpr static const char* k_importCommand2 = " import *";
  /* strlen(k_importCommand1) + strlen(k_importCommand2) +
   * TextField::maxBufferSize() */
  constexpr static size_t k_maxImportCommandSize =
      5 + 9 + Escher::TextField::MaxBufferSize();
  constexpr static int k_lineCellType = 0;
  constexpr static int k_editCellType = 1;
  constexpr static int k_numberOfLineCells =
      Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
          KDFont::GlyphHeight(KDFont::Size::Small));
  constexpr static int k_outputAccumulationBufferSize = 1000;
  static_assert(ConsoleStore::k_historySize > k_outputAccumulationBufferSize,
                "Accumulation buffer of console is larger than history");
  static_assert(k_outputAccumulationBufferSize <
                    KDCOORDINATE_MAX / KDFont::GlyphWidth(KDFont::Size::Large),
                "Accumulation buffer of console is too long for large font");

  int editableCellRow() const { return m_consoleStore.numberOfLines(); }

  // RegularListViewDataSource
  KDCoordinate defaultRowHeight() override;
  bool isDisplayingViewController();
  void reloadData();
  void flushOutputAccumulationBufferToStore();
  void appendTextToOutputAccumulationBuffer(const char* text, size_t length);
  void emptyOutputAccumulationBuffer();
  size_t firstNewLineCharIndex(const char* text, size_t length);
  Escher::StackViewController* stackViewController();
  void prepareVariableBox();
  App* m_pythonDelegate;
  bool m_importScriptsWhenViewAppears;
  ConsoleStore m_consoleStore;
  Escher::SelectableListView m_selectableListView;
  ConsoleLineCell m_cells[k_numberOfLineCells];
  ConsoleEditCell m_editCell;
  char m_outputAccumulationBuffer[k_outputAccumulationBufferSize];
  /* The Python machine might call printText several times to print a single
   * string. We thus use m_outputAccumulationBuffer to store and concatenate the
   * different strings until a new line char appears in the text. When this
   * happens, or when m_outputAccumulationBuffer is full, we create a new
   * ConsoleLine in the ConsoleStore and empty m_outputAccumulationBuffer. */
  ScriptStore m_scriptStore;
  SandboxController m_sandboxController;
  bool m_inputRunLoopActive;
  bool m_autoImportScripts;
#if EPSILON_GETOPT
  bool m_locked;
#endif
};
}  // namespace Code

#endif
