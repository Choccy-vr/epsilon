#include "dynamic_cells_data_source.h"

#include <new>

#include "inference/app.h"

using namespace Escher;

namespace Inference {

template <typename T, int N>
DynamicCellsDataSource<T, N>::~DynamicCellsDataSource() {
  if (m_cells) {
    destroyCells();
  }
}

template <typename T, int N>
void DynamicCellsDataSource<T, N>::createCells() {
  if (m_cells == nullptr) {
    createCellsWithOffset(0);
  }
}

template <typename T, int N>
void DynamicCellsDataSource<T, N>::createCellsWithOffset(size_t offset) {
  assert(m_cells == nullptr);
  static_assert(sizeof(T) * N <= App::k_bufferSize,
                "Inference::App::m_buffer is not large enough");
  assert(offset + sizeof(T) * N <= App::k_bufferSize);
  if (offset == 0) {
    /* The buffer gets entirely clean only when creating cells from the
     * beginning of the buffer. */
    App::app()->cleanBuffer(this);
  }
  m_cells = new (App::app()->buffer(offset)) T[N];
  for (int i = 0; i < N; i++) {
    m_delegate->initCell(T(), &m_cells[i], i);
  }
}

template <typename T, int N>
void DynamicCellsDataSource<T, N>::destroyCells() {
  if (m_cells) {
    /* We manually call T destructor since we cannot use 'delete' due to the
     * placement new.
     * Note Bene: we qualify the destructor call (by prefixing it by its class
     * name) to avoid a compiler warning: T is not a final class and has virtual
     * methods but no virtual destructor; the compiler might think we forgot
     * some virtualization here but we didn't - we don't want to call a derived
     * destructor of children T class. */
    for (int i = 0; i < N; i++) {
      // Make sure not to keep the first responder pointing on a destroyed cell
      Responder* cellResponder = m_cells[i].responder();
      Responder* appFirstResponder = App::app()->firstResponder();
      if (appFirstResponder && appFirstResponder->hasAncestor(cellResponder)) {
        App::app()->setFirstResponder(nullptr);
      }
      m_cells[i].T::~T();
    }
  }
  m_cells = nullptr;
}

template <typename T, int N>
Escher::HighlightCell* DynamicCellsDataSource<T, N>::cell(int i) {
  assert(m_cells);
  return &m_cells[i];
}

// -1 takes the hidden top left cell into account
static_assert(k_homogeneityTableNumberOfReusableHeaderCells ==
                  HomogeneityTableDataSource::k_numberOfReusableColumns +
                      HomogeneityTableDataSource::k_maxNumberOfReusableRows - 1,
              "k_homogeneityTableNumberOfReusableHeaderCells should be updated "
              "with HomogeneityTableDataSource::k_numberOfReusableColumns and "
              "HomogeneityTableDataSource::k_maxNumberOfReusableRows");
static_assert(k_homogeneityTableNumberOfReusableInnerCells ==
                  HomogeneityTableDataSource::k_numberOfReusableCells,
              "k_homogeneityTableNumberOfReusableHeaderCells should be updated "
              "with HomogeneityTableDataSource::k_numberOfReusableCells");
static_assert(k_doubleColumnTableNumberOfReusableCells ==
                  InputGoodnessTableCell::k_numberOfReusableCells,
              "k_doubleColumnTableNumberOfReusableCells should be updated with "
              "InputGoodnessTableCell::k_numberOfReusableCells");
static_assert(k_doubleColumnTableNumberOfReusableCells ==
                  StoreTableCell::k_numberOfReusableCells,
              "k_doubleColumnTableNumberOfReusableCells should be updated with "
              "StoreTableCell::k_numberOfReusableCells");
static_assert(k_inputControllerNumberOfReusableCells ==
                  InputController::k_numberOfReusableCells,
              "k_inputControllerNumberOfReusableCells should be updated with "
              "InputController::k_numberOfReusableCells");
static_assert(k_goodnessContributionsTableNumberOfReusableCells ==
              Escher::Metric::MinimalNumberOfScrollableRowsToFillDisplayHeight(
                  Escher::Metric::SmallEditableCellHeight,
                  Escher::Metric::TabHeight) *
                  3);
template class DynamicCellsDataSource<
    InferenceEvenOddBufferCell, k_homogeneityTableNumberOfReusableHeaderCells>;
template class DynamicCellsDataSource<
    InferenceEvenOddBufferCell, k_homogeneityTableNumberOfReusableInnerCells>;
template class DynamicCellsDataSource<
    InferenceEvenOddEditableCell, k_homogeneityTableNumberOfReusableInnerCells>;
template class DynamicCellsDataSource<
    InferenceEvenOddBufferCell,
    k_goodnessContributionsTableNumberOfReusableCells>;
template class DynamicCellsDataSource<InferenceEvenOddEditableCell,
                                      k_doubleColumnTableNumberOfReusableCells>;
template class DynamicCellsDataSource<
    Escher::MenuCellWithEditableText<Escher::LayoutView,
                                     Escher::MessageTextView>,
    k_maxNumberOfParameterCell>;
template class DynamicCellsDataSource<ResultCell, k_maxNumberOfResultCells>;
}  // namespace Inference
