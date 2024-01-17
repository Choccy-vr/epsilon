#ifndef SEQUENCE_LIST_CONTROLLER_H
#define SEQUENCE_LIST_CONTROLLER_H

#include <apps/i18n.h>
#include <apps/shared/function_list_controller.h>
#include <apps/shared/sequence_store.h>
#include <escher/list_view_data_source.h>

#include "editable_sequence_cell.h"
#include "list_parameter_controller.h"
#include "sequence_cell.h"
#include "sequence_toolbox_data_source.h"
#include "type_parameter_controller.h"

namespace Sequence {

class ListController : public Shared::FunctionListController {
 public:
  ListController(Escher::Responder* parentResponder,
                 Escher::ButtonRowController* header,
                 Escher::ButtonRowController* footer);
  int numberOfExpressionRows() const override;
  KDCoordinate expressionRowHeight(int row) override;
  void selectPreviousNewSequenceCell();
  void editExpression(Ion::Events::Event event) override;
  /* ViewController */
  void viewWillAppear() override;
  void viewDidDisappear() override;
  /* ListViewDataSource */
  Escher::HighlightCell* reusableCell(int index, int type) override;
  int reusableCellCount(int type) override {
    return type > k_expressionCellType ? 1 : maxNumberOfDisplayableRows();
  }
  bool canStoreCellAtRow(int row) override { return false; }
  void fillCellForRow(Escher::HighlightCell* cell, int row) override;
  /* Responder */
  bool handleEvent(Ion::Events::Event event) override;
  void showLastSequence();

  /* MathLayoutFieldDelegate */
  bool layoutFieldDidReceiveEvent(Escher::LayoutField* layoutField,
                                  Ion::Events::Event event) override;
  bool isAcceptableExpression(const Poincare::Expression expression,
                              Poincare::Context* context) override;
  CodePoint defaultXNT() override { return Shared::Sequence::k_sequenceSymbol; }

 private:
  /* Model definitions */
  constexpr static int k_otherDefinition = -1;
  constexpr static int k_sequenceDefinition = 0;
  constexpr static int k_firstInitialCondition = k_sequenceDefinition + 1;
  constexpr static int k_secondInitialCondition = k_firstInitialCondition + 1;
  /* Width and margins */
  constexpr static KDCoordinate k_minTitleColumnWidth = 65;
  constexpr static KDCoordinate k_functionTitleSumOfMargins = 25;
  /* Row numbers */
  constexpr static int k_maxNumberOfRows =
      3 * Shared::SequenceStore::k_maxNumberOfSequences;

  Escher::EditableExpressionModelCell* editableExpressionModelCell() override {
    return m_editableCell.expressionCell();
  }

  void computeTitlesColumnWidth(bool forceMax = false);
  ListParameterController* parameterController() override {
    return &m_parameterController;
  }
  int maxNumberOfDisplayableRows() const override { return k_maxNumberOfRows; }
  Escher::HighlightCell* titleCells(int index);
  Escher::HighlightCell* functionCells(int index) override;
  void willDisplayTitleCellAtIndex(VerticalSequenceTitleCell* cell, int j,
                                   Escher::HighlightCell* expressionCell);
  void willDisplayExpressionCellAtIndex(Escher::HighlightCell* cell,
                                        int j) override;
  int modelIndexForRow(int j) const override;
  int sequenceDefinitionForRow(int j) const;
  KDCoordinate maxFunctionNameWidth();
  void didChangeModelsList() override;
  KDCoordinate baseline(int j, Escher::HighlightCell* cell);
  void addModel() override;
  bool editSelectedRecordWithText(const char* text) override;
  void getTextForSelectedRecord(char* text, size_t size) const override;
  bool removeModelRow(Ion::Storage::Record record) override;
  Shared::SequenceStore* modelStore() const override;
  KDCoordinate nameWidth(int nameLength) const;
  Escher::LayoutField* layoutField() override {
    return m_editableCell.expressionCell()->layoutField();
  }

  SequenceCell m_sequenceCells[k_maxNumberOfRows];
  EditableSequenceCell m_editableCell;
  ListParameterController m_parameterController;
  TypeParameterController m_typeParameterController;
  Escher::StackViewController m_typeStackController;
  SequenceToolboxDataSource m_sequenceToolboxDataSource;
  KDCoordinate m_titlesColumnWidth;
  bool m_parameterColumnSelected;
};

}  // namespace Sequence

#endif
