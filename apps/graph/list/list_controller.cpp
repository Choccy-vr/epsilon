#include "list_controller.h"

#include <apps/i18n.h>
#include <assert.h>
#include <escher/metric.h>
#include <poincare/code_point_layout.h>
#include <poincare/matrix_layout.h>
#include <poincare/symbol_abstract.h>

#include "../app.h"
#include "../shared/function_name_helper.h"

using namespace Shared;
using namespace Escher;
using namespace Poincare;

namespace Graph {

ListController::ListController(
    Responder *parentResponder, ButtonRowController *header,
    ButtonRowController *footer,
    FunctionParameterController *functionParameterController)
    : Shared::FunctionListController(parentResponder, header, footer,
                                     I18n::Message::AddFunction),
      m_editableCell(this, this, &m_modelsStackController),
      m_parameterController(functionParameterController),
      m_modelsParameterController(this, this),
      m_modelsStackController(nullptr, &m_modelsParameterController,
                              StackViewController::Style::PurpleWhite),
      m_parameterColumnSelected(false) {}

/* TableViewDataSource */

HighlightCell *ListController::reusableCell(int index, int type) {
  assert(index >= 0 && index < reusableCellCount(type));
  if (type == k_editableCellType) {
    return &m_editableCell;
  }
  if (type == k_addNewModelCellType) {
    return &m_addNewModelCell;
  }
  assert(type == k_expressionCellType);
  return functionCells(index);
}

int ListController::reusableCellCount(int type) const {
  if (type == k_expressionCellType) {
    return maxNumberOfDisplayableRows();
  }
  return 1;
}

/* ViewController */

void ListController::viewWillAppear() {
  Shared::FunctionListController::viewWillAppear();
  /* FunctionListcontroller::didEnterResponderChain might not be called,
   * (if the list tab is displayed but not selected using Back-Back)
   * therefore we also need to manually reload the table here. */
  selectableListView()->reloadData(false);
  App::app()->defaultToolbox()->setExtraCellsDataSource(this);
}

void ListController::viewDidDisappear() {
  Shared::FunctionListController::viewDidDisappear();
  App::app()->defaultToolbox()->setExtraCellsDataSource(nullptr);
}

// Fills buffer with a default function equation, such as "f(x)=", "y=" or
// "r1(θ)="
void ListController::fillWithDefaultFunctionEquation(char *buffer,
                                                     size_t bufferSize,
                                                     CodePoint symbol) const {
  size_t length;
  if (symbol == ContinuousFunction::k_cartesianSymbol &&
      FunctionModelsParameterController::EquationsPrefered()) {
    length = SerializationHelper::CodePoint(
        buffer, bufferSize, ContinuousFunction::k_ordinateSymbol);
  } else {
    length = FunctionNameHelper::DefaultName(buffer, bufferSize, symbol);
    assert(0 < length && length < bufferSize - 1);
    length += Shared::Function::WithArgument(symbol, buffer + length,
                                             bufferSize - length);
  }
  length +=
      SerializationHelper::CodePoint(buffer + length, bufferSize - length, '=');
}

bool ListController::shouldCompleteEquation(Poincare::Expression expression,
                                            CodePoint symbol) {
  /* We do not want to complete equation if expression is already an
   * (in)equation, a point or a list (of points). */
  return expression.type() != ExpressionNode::Type::Comparison &&
         (expression.type() != ExpressionNode::Type::Point ||
          symbol == Symbol::k_parametricSymbol) &&
         !expression.deepIsList(nullptr);
}

bool ListController::completeEquation(LayoutField *equationField,
                                      CodePoint symbol) {
  equationField->putCursorOnOneSide(OMG::Direction::Left());
  // Retrieve the edited function
  ExpiringPointer<ContinuousFunction> f =
      modelStore()->modelForRecord(modelStore()->recordAtIndex(selectedRow()));
  constexpr size_t k_bufferSize =
      SymbolAbstractNode::k_maxNameSize + sizeof("(θ)≥") - 1;
  char buffer[k_bufferSize];
  if (f->isNull() || f->properties().status() ==
                         ContinuousFunctionProperties::Status::Undefined) {
    // Function is new or undefined, complete the equation with a default name
    // Insert "f(x)=", with f the default function name and x the symbol
    fillWithDefaultFunctionEquation(buffer, k_bufferSize, symbol);
  } else {
    // Insert the name, symbol and equation symbol of the existing function
    size_t nameLength = f->nameWithArgument(buffer, k_bufferSize);
    nameLength += strlcpy(buffer + nameLength, f->properties().equationSymbol(),
                          k_bufferSize - nameLength);
    assert(nameLength < k_bufferSize);
  }
  bool handled = equationField->handleEventWithText(buffer, false, true);
  equationField->putCursorOnOneSide(OMG::Direction::Right());
  return handled;
}

void ListController::layoutFieldDidHandleEvent(LayoutField *layoutField) {
  bool isVisible = m_editableCell.isTemplateButtonVisible();
  bool shouldBeVisible = m_editableCell.templateButtonShouldBeVisible();

  if (shouldBeVisible == isVisible) {
    return;
  }
  if (!shouldBeVisible) {
    m_editableCell.setTemplateButtonHighlighted(false);
  }
  m_editableCell.setTemplateButtonVisible(shouldBeVisible);
}

bool ListController::layoutFieldDidReceiveEvent(LayoutField *layoutField,
                                                Ion::Events::Event event) {
  assert(layoutField == this->layoutField());
  m_parameterColumnSelected = false;

  if (m_editableCell.isTemplateButtonVisible()) {
    bool onTemplateButton = m_editableCell.isTemplateButtonHighlighted();

    if (event == Ion::Events::Right && !onTemplateButton) {
      m_editableCell.setTemplateButtonHighlighted(true);
      return true;
    }
    if (event == Ion::Events::Left && onTemplateButton) {
      m_editableCell.setTemplateButtonHighlighted(false);
      return true;
    }
  }

  if (event.isMoveEvent()) {
    return true;
  }

  return ExpressionModelListController::layoutFieldDidReceiveEvent(layoutField,
                                                                   event);
}

CodePoint ListController::defaultXNT() {
  int selectedFunctionIndex = selectedRow();
  if (selectedFunctionIndex >= 0) {
    assert(selectedFunctionIndex < modelStore()->numberOfModels());
    Ion::Storage::Record record =
        modelStore()->recordAtIndex(selectedFunctionIndex);
    return modelStore()->modelForRecord(record)->symbol();
  }
  return ContinuousFunction::k_cartesianSymbol;
}

void ListController::editExpression(Ion::Events::Event event) {
  ExpressionModelListController::editExpression(event);
  if (m_editableCell.isTemplateButtonHighlighted()) {
    // TODO: should be done in layoutFieldDidStartEditing
    m_editableCell.setTemplateButtonHighlighted(false);
  }
  m_editableCell.setTemplateButtonVisible(
      m_editableCell.templateButtonShouldBeVisible());
  m_editableCell.setHighlighted(true);
}

bool ListController::editSelectedRecordWithText(const char *text) {
  deleteParametricComponentsOfSelectedModel();
  bool result = FunctionListController::editSelectedRecordWithText(text);
  storeParametricComponentsOfSelectedModel();
  return result;
}

/* Responder */

KDCoordinate ListController::expressionRowHeight(int row) {
  assert(typeAtRow(row) == k_expressionCellType);
  FunctionCell tempCell;
  return protectedNonMemoizedRowHeight(&tempCell, row);
}

KDCoordinate ListController::editableRowHeight() {
  return m_editableCell.minimalSizeForOptimalDisplay().height();
}

bool ListController::handleEvent(Ion::Events::Event event) {
  // Here we handle an additional parameter column, within FunctionCell's button
  if (selectedRow() >= 0 && selectedRow() <= numberOfRows() &&
      !isAddEmptyRow(selectedRow())) {
    // Selected row is a function cell
    if (m_parameterColumnSelected) {
      // Parameter column is selected
      if (event == Ion::Events::OK || event == Ion::Events::EXE) {
        // Open function parameter menu
        m_parameterController->setUseColumnTitle(false);
        configureFunction(
            modelStore()->recordAtIndex(modelIndexForRow(selectedRow())));
        return true;
      }
      if (event == Ion::Events::Left) {
        // Leave parameter column
        m_parameterColumnSelected = false;
        selectableListView()->reloadData(true, false);
        return true;
      }
    } else if (event == Ion::Events::Right) {
      // Enter parameter column
      m_parameterColumnSelected = true;
      selectableListView()->reloadData(true, false);
      return true;
    }
  }
  if (event == Ion::Events::Up) {
    if (selectedRow() == -1) {
      footer()->setSelectedButton(-1);
      selectableListView()->selectCell(numberOfRows() - 1);
      App::app()->setFirstResponder(selectableListView());
      return true;
    }
    selectableListView()->deselectTable();
    assert(selectedRow() == -1);
    tabController()->selectTab();
    return true;
  }
  if (selectedRow() < 0) {
    return false;
  }
  if (event == Ion::Events::Down) {
    selectableListView()->deselectTable();
    footer()->setSelectedButton(0);
    return true;
  }
  return handleEventOnExpression(event);
}

Shared::ListParameterController *ListController::parameterController() {
  return m_parameterController;
}

bool ListController::removeModelRow(Ion::Storage::Record record) {
  deleteParametricComponentsOfSelectedModel();
  return FunctionListController::removeModelRow(record);
}

int ListController::maxNumberOfDisplayableRows() const {
  return k_maxNumberOfDisplayableRows;
}

Poincare::Layout ListController::extraCellLayoutAtRow(int row) {
  assert(row < k_numberOfToolboxExtraCells);
  constexpr CodePoint codepoints[k_numberOfToolboxExtraCells] = {
      UCodePointInferiorEqual, UCodePointSuperiorEqual};
  return CodePointLayout::Builder(codepoints[row]);
}

HighlightCell *ListController::functionCells(int row) {
  assert(row >= 0 && row < k_maxNumberOfDisplayableRows);
  return &m_expressionCells[row];
}

void ListController::fillCellForRow(HighlightCell *cell, int row) {
  assert(cell != nullptr);
  EvenOddCell *evenOddCell = static_cast<EvenOddCell *>(cell);
  evenOddCell->setEven(row % 2 == 0);
  int type = typeAtRow(row);
  if (type == k_addNewModelCellType) {
    evenOddCell->reloadCell();
    return;
  }
  assert(type == k_expressionCellType || type == k_editableCellType);
  ExpiringPointer<ContinuousFunction> f =
      modelStore()->modelForRecord(modelStore()->recordAtIndex(row));
  if (type == k_expressionCellType) {
    FunctionCell *functionCell = static_cast<FunctionCell *>(cell);
    functionCell->expressionCell()->setLayout(f->layout());
    functionCell->setMessage(f->properties().caption());
    KDColor textColor = f->isActive() ? KDColorBlack : Palette::GrayDark;
    functionCell->expressionCell()->setTextColor(textColor);
    static_cast<FunctionCell *>(functionCell)
        ->setParameterSelected(m_parameterColumnSelected);
  }

  // f can be null if the entry was just created and is still empty.
  KDColor functionColor =
      (!f->isNull() && f->isActive()) ? f->color() : Palette::GrayDark;
  static_cast<AbstractFunctionCell *>(cell)->setColor(functionColor);
  cell->reloadCell();
}

void ListController::addNewModelAction() {
  Ion::Storage::Record::ErrorStatus error =
      App::app()->functionStore()->addEmptyModel();
  if (error == Ion::Storage::Record::ErrorStatus::NotEnoughSpaceAvailable) {
    return;
  }
  assert(error == Ion::Storage::Record::ErrorStatus::None);

  editExpression(Ion::Events::OK);
}

ContinuousFunctionStore *ListController::modelStore() const {
  return App::app()->functionStore();
}

static void deleteParametricComponent(char *baseName, size_t baseNameLength,
                                      size_t bufferSize, bool first) {
  FunctionNameHelper::AddSuffixForParametricComponent(baseName, baseNameLength,
                                                      bufferSize, first);
  Ion::Storage::Record record =
      Ion::Storage::FileSystem::sharedFileSystem->recordBaseNamedWithExtension(
          baseName, Ion::Storage::parametricComponentExtension);
  record.destroy();
}

static void storeParametricComponent(char *baseName, size_t baseNameLength,
                                     size_t bufferSize, const Expression &e,
                                     bool first) {
  assert(!e.isUninitialized() && e.type() == ExpressionNode::Type::Point &&
         e.numberOfChildren() == 2);
  Expression child = e.childAtIndex(first ? 0 : 1).clone();
  FunctionNameHelper::AddSuffixForParametricComponent(baseName, baseNameLength,
                                                      bufferSize, first);
  child.storeWithNameAndExtension(baseName,
                                  Ion::Storage::parametricComponentExtension);
}

void ListController::DeleteParametricComponentsWithBaseName(
    char *baseName, size_t baseNameLength, size_t bufferSize) {
  deleteParametricComponent(baseName, baseNameLength, bufferSize, true);
  deleteParametricComponent(baseName, baseNameLength, bufferSize, false);
}

void ListController::deleteParametricComponentsOfSelectedModel() {
  ExpiringPointer<ContinuousFunction> f =
      modelStore()->modelForRecord(modelStore()->recordAtIndex(selectedRow()));
  if (!f->properties().isEnabledParametric()) {
    return;
  }
  constexpr size_t bufferSize = SymbolAbstractNode::k_maxNameSize;
  char buffer[bufferSize];
  size_t length = f->name(buffer, bufferSize);
  DeleteParametricComponentsWithBaseName(buffer, length, bufferSize);
}

void ListController::storeParametricComponentsOfSelectedModel() {
  ExpiringPointer<ContinuousFunction> f =
      modelStore()->modelForRecord(modelStore()->recordAtIndex(selectedRow()));
  if (!f->properties().isEnabledParametric()) {
    return;
  }
  Expression e = f->expressionClone();
  constexpr size_t bufferSize = SymbolAbstractNode::k_maxNameSize;
  char buffer[bufferSize];
  size_t length = f->name(buffer, bufferSize);
  assert(FunctionNameHelper::ParametricComponentsNamesAreFree(buffer, length,
                                                              bufferSize));
  storeParametricComponent(buffer, length, bufferSize, e, true);
  storeParametricComponent(buffer, length, bufferSize, e, false);
}

bool ListController::isValidExpressionModel(Expression expression) {
  ExpiringPointer<ContinuousFunction> f =
      modelStore()->modelForRecord(modelStore()->recordAtIndex(selectedRow()));
  if (FunctionNameHelper::ParametricComponentsNameError(expression,
                                                        f.pointer())) {
    App::app()->displayWarning(I18n::Message::ParametricNameError);
    return false;
  }
  return true;
}

}  // namespace Graph
