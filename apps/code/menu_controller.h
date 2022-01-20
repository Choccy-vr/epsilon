#ifndef CODE_MENU_CONTROLLER_H
#define CODE_MENU_CONTROLLER_H

#include "console_controller.h"
#include "editor_controller.h"
#include "script_name_cell.h"
#include "script_parameter_controller.h"
#include "script_store.h"
#include <escher/button_row_controller.h>
#include <escher/even_odd_cell_with_ellipsis.h>
#include <escher/even_odd_message_text_cell.h>

namespace Code {

class ScriptParameterController;

class MenuController : public Escher::ViewController, public Escher::TableViewDataSource, public Escher::SelectableTableViewDataSource, public Escher::SelectableTableViewDelegate, public Escher::TextFieldDelegate, public Escher::ButtonRowDelegate {
public:
  MenuController(Escher::Responder * parentResponder, App * pythonDelegate, ScriptStore * scriptStore, Escher::ButtonRowController * footer);
  ConsoleController * consoleController();
  Escher::StackViewController * stackViewController();
  void willExitResponderChain(Escher::Responder * nextFirstResponder) override;
  void renameSelectedScript();
  void deleteScript(Script script);
  void reloadConsole();
  void openConsoleWithScript(Script script);
  void scriptContentEditionDidFinish();
  void willExitApp();
  int editedScriptIndex() const { return m_editorController.scriptIndex(); }

  /* ViewController */
  Escher::View * view() override { return &m_selectableTableView; }
  ViewController::TitlesDisplay titlesDisplay() override { return TitlesDisplay::NeverDisplayOwnTitle; }
  bool handleEvent(Ion::Events::Event event) override;
  void didBecomeFirstResponder() override;
  void viewWillAppear() override;
  TELEMETRY_ID("Menu");

  /* TableViewDataSource */
  int numberOfRows() const override;
  int numberOfColumns() const override { return 2; }
  void willDisplayCellAtLocation(Escher::HighlightCell * cell, int i, int j) override;
  KDCoordinate columnWidth(int i) override;
  KDCoordinate rowHeight(int j) override { return Escher::Metric::StoreRowHeight; }
  KDCoordinate cumulatedWidthFromIndex(int i) override;
  KDCoordinate cumulatedHeightFromIndex(int j) override;
  int indexFromCumulatedWidth(KDCoordinate offsetX) override;
  int indexFromCumulatedHeight(KDCoordinate offsetY) override;
  Escher::HighlightCell * reusableCell(int index, int type) override;
  int reusableCellCount(int type) override;
  int typeAtLocation(int i, int j) override;
  void willDisplayScriptTitleCellForIndex(Escher::HighlightCell * cell, int index);

  /* SelectableTableViewDelegate */
  void tableViewDidChangeSelection(Escher::SelectableTableView * t, int previousSelectedCellX, int previousSelectedCellY, bool withinTemporarySelection) override;

  /* TextFieldDelegate */
  bool textFieldShouldFinishEditing(Escher::TextField * textField, Ion::Events::Event event) override;
  bool textFieldDidReceiveEvent(Escher::TextField * textField, Ion::Events::Event event) override { return false; }
  bool textFieldDidFinishEditing(Escher::TextField * textField, const char * text, Ion::Events::Event event) override;
  bool textFieldDidAbortEditing(Escher::TextField * textField) override {
    return privateTextFieldDidAbortEditing(textField, true);
  }
  bool textFieldDidHandleEvent(Escher::TextField * textField, bool returnValue, bool textSizeDidChange) override;

  /* ButtonRowDelegate */
  int numberOfButtons(Escher::ButtonRowController::Position position) const override { return 1; }
  Escher::Button * buttonAtIndex(int index, Escher::ButtonRowController::Position position) const override {
    assert(index == 0);
    return const_cast<Escher::Button *>(&m_consoleButton);
  }

private:
  static constexpr int k_maxNumberOfDisplayableScriptCells = 5; // = 240/50
  static constexpr int k_parametersColumnWidth = Escher::Metric::EllipsisCellWidth;
  static constexpr int AddScriptCellType = 0;
  static constexpr int ScriptCellType = 1;
  static constexpr int ScriptParameterCellType = 2;
  static constexpr int EmptyCellType = 3;
  void addScript();
  void configureScript();
  void editScriptAtIndex(int scriptIndex);
  void numberedDefaultScriptName(char * buffer);
  void updateAddScriptRowDisplay();
  bool privateTextFieldDidAbortEditing(Escher::TextField * textField, bool menuControllerStaysInResponderChain);
  ScriptStore * m_scriptStore;
  ScriptNameCell m_scriptCells[k_maxNumberOfDisplayableScriptCells];
  Escher::EvenOddCellWithEllipsis m_scriptParameterCells[k_maxNumberOfDisplayableScriptCells];
  Escher::EvenOddMessageTextCell m_addNewScriptCell;
  Escher::EvenOddCell m_emptyCell;
  Escher::Button m_consoleButton;
  Escher::SelectableTableView m_selectableTableView;
  ScriptParameterController m_scriptParameterController;
  EditorController m_editorController;
  bool m_reloadConsoleWhenBecomingFirstResponder;
  bool m_shouldDisplayAddScriptRow;
};

}

#endif
