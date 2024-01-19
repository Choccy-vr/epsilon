#include "double_pair_store.h"

#include <assert.h>
#include <ion.h>
#include <ion/storage/file_system.h>
#include <poincare/helpers.h>
#include <poincare/list.h>
#include <stddef.h>

#include <algorithm>
#include <cmath>

#include "double_pair_store_preferences.h"
#include "poincare_helpers.h"

using namespace Poincare;
using namespace Ion::Storage;

namespace Shared {

DoublePairStore::DoublePairStore(GlobalContext *context,
                                 DoublePairStorePreferences *preferences)
    : m_storePreferences(preferences), m_context(context) {}

void DoublePairStore::initListsInPool() {
  // Initialize empty list in the pool
  for (int s = 0; s < k_numberOfSeries; s++) {
    for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
      m_dataLists[s][i] = FloatList<double>::Builder();
    }
  }
}

void DoublePairStore::initListsFromStorage(bool delayUpdate) {
  initListsInPool();
  char listName[k_columnNamesLength + 1];
  for (int s = 0; s < k_numberOfSeries; s++) {
    for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
      // Get the data of X1, Y1, X2, Y2, V1, V2, etc. from storage
      fillColumnName(s, i, listName);
      Record r = Record(listName, listExtension);
      Record::Data listData = r.value();
      if (listData.size == 0) {
        continue;
      }
      Expression e =
          Expression::ExpressionFromAddress(listData.buffer, listData.size);
      if (e.type() != ExpressionNode::Type::List) {
        continue;
      }
      setList(static_cast<List &>(e), s, i, true);
    }
    updateSeries(s, delayUpdate);
  }
}

void DoublePairStore::tidy() {
  for (int serie = 0; serie < k_numberOfSeries; serie++) {
    for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
      m_dataLists[serie][i] = Poincare::FloatList<double>();
    }
  }
}

size_t DoublePairStore::fillColumnName(int series, int column,
                                       char *buffer) const {
  assert(series >= 0 && series < k_numberOfSeries);
  assert(column >= 0 && column < k_numberOfColumnsPerSeries);
  buffer[0] = columnNamePrefixAtIndex(column);
  buffer[1] = static_cast<char>('1' + series);
  buffer[2] = 0;
  return 2;
}

bool DoublePairStore::isColumnName(const char *name, int nameLen,
                                   int *returnSeries, int *returnColumn) {
  if (nameLen != 2 || name[1] < '1' || name[1] >= '1' + k_numberOfSeries) {
    return false;
  }
  for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
    if (name[0] == columnNamePrefixAtIndex(i)) {
      if (returnSeries != nullptr) {
        *returnSeries = name[1] - '1';
      }
      if (returnColumn != nullptr) {
        *returnColumn = i;
      }
      return true;
    }
  }
  return false;
}

double DoublePairStore::get(int series, int i, int j) const {
  assert(j < numberOfPairsOfSeries(series));
  return m_dataLists[series][i].valueAtIndex(j);
}

bool DoublePairStore::set(double f, int series, int i, int j, bool delayUpdate,
                          bool setOtherColumnToDefaultIfEmpty) {
  assert(series >= 0 && series < k_numberOfSeries);
  if (j >= k_maxNumberOfPairs) {
    return false;
  }
  assert(j <= numberOfPairsOfSeries(series));
  if (j >= lengthOfColumn(series, i)) {
    for (int k = lengthOfColumn(series, i); k < j; k++) {
      m_dataLists[series][i].addValueAtIndex(NAN, k);
    }
    m_dataLists[series][i].addValueAtIndex(f, j);
  } else {
    m_dataLists[series][i].replaceValueAtIndex(f, j);
  }
  int otherI = i == 0 ? 1 : 0;
  if (setOtherColumnToDefaultIfEmpty && j >= lengthOfColumn(series, otherI)) {
    set(defaultValue(series, otherI, j), series, otherI, j, true, false);
  }
  return updateSeries(series, delayUpdate);
}

bool DoublePairStore::setList(List &list, int series, int i, bool delayUpdate,
                              bool setOtherColumnToDefaultIfEmpty) {
  /* Approximate the list to turn it into list of doubles since we do not
   * want to work with exact expressions in Regression and Statistics.*/
  assert(series >= 0 && series < k_numberOfSeries);
  assert(i == 0 || i == 1);
  int newListLength =
      std::max(list.numberOfChildren(), lengthOfColumn(series, i));
  for (int j = 0; j < newListLength; j++) {
    if (j >= list.numberOfChildren()) {
      m_dataLists[series][i].removeValueAtIndex(list.numberOfChildren());
      continue;
    }
    double evaluation = PoincareHelpers::ApproximateToScalar<double>(
        list.childAtIndex(j), m_context);
    set(evaluation, series, i, j, true, setOtherColumnToDefaultIfEmpty);
  }
  return updateSeries(series, delayUpdate);
}

int DoublePairStore::numberOfPairs() const {
  int result = 0;
  for (int i = 0; i < k_numberOfSeries; i++) {
    result += numberOfPairsOfSeries(i);
  }
  return result;
}

bool DoublePairStore::deleteValueAtIndex(int series, int i, int j,
                                         bool authorizeNonEmptyRowDeletion,
                                         bool delayUpdate) {
  assert(series >= 0 && series < k_numberOfSeries);
  assert(j >= 0 && j < numberOfPairsOfSeries(series));
  int otherI = (i + 1) % k_numberOfColumnsPerSeries;
  bool willDeletePair = std::isnan(get(series, otherI, j));
  set(NAN, series, i, j, delayUpdate);
  return willDeletePair;
}

void DoublePairStore::deletePairOfSeriesAtIndex(int series, int j,
                                                bool delayUpdate) {
  set(NAN, series, 0, j, true);
  set(NAN, series, 1, j, true);
  updateSeries(series, delayUpdate);
}

void DoublePairStore::deleteAllPairsOfSeries(int series, bool delayUpdate) {
  assert(series >= 0 && series < k_numberOfSeries);
  for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
    deleteColumn(series, i, true);
  }
  updateSeries(series, delayUpdate);
}

void DoublePairStore::deleteAllPairs(bool delayUpdate) {
  for (int i = 0; i < k_numberOfSeries; i++) {
    deleteAllPairsOfSeries(i, delayUpdate);
  }
}

void DoublePairStore::deleteColumn(int series, int i, bool delayUpdate) {
  assert(series >= 0 && series < k_numberOfSeries);
  assert(i == 0 || i == 1);
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    deleteValueAtIndex(series, i, k, false, true);
  }
  updateSeries(series, delayUpdate);
}

void DoublePairStore::resetColumn(int series, int i, bool delayUpdate) {
  assert(series >= 0 && series < k_numberOfSeries);
  assert(i == 0 || i == 1);
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    set(defaultValue(series, i, k), series, i, k, true);
  }
  updateSeries(series, delayUpdate);
}

void DoublePairStore::setSeriesHidden(int series, bool hidden) {
  assert(series >= 0 && series < k_numberOfSeries);
  return m_storePreferences->setSeriesHidden(series, hidden);
}

bool DoublePairStore::seriesIsActive(int series) const {
  assert(series >= 0 && series < k_numberOfSeries);
  return m_storePreferences->seriesIsActive(series);
}

bool DoublePairStore::seriesIsValid(int series) const {
  assert(series >= 0 && series < k_numberOfSeries);
  return m_storePreferences->seriesIsValid(series);
}

void DoublePairStore::updateSeriesValidity(int series) {
  assert(series >= 0 && series < k_numberOfSeries);
  int numberOfPairs = numberOfPairsOfSeries(series);
  if (numberOfPairs == 0 ||
      lengthOfColumn(series, 0) != lengthOfColumn(series, 1)) {
    m_storePreferences->setSeriesValid(series, false);
    return;
  }
  for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
    for (int j = 0; j < numberOfPairs; j++) {
      if (!valueValidInColumn(get(series, i, j), i)) {
        m_storePreferences->setSeriesValid(series, false);
        return;
      }
    }
  }
  m_storePreferences->setSeriesValid(series, true);
}

bool DoublePairStore::hasActiveSeries(ActiveSeriesTest activeSeriesTest) const {
  for (int i = 0; i < k_numberOfSeries; i++) {
    if (activeSeriesTest(this, i)) {
      return true;
    }
  }
  return false;
}

int DoublePairStore::numberOfActiveSeries(
    ActiveSeriesTest activeSeriesTest) const {
  int nonEmptySeriesCount = 0;
  for (int i = 0; i < k_numberOfSeries; i++) {
    if (activeSeriesTest(this, i)) {
      nonEmptySeriesCount++;
    }
  }
  return nonEmptySeriesCount;
}

int DoublePairStore::seriesIndexFromActiveSeriesIndex(
    int activeSeriesIndex, ActiveSeriesTest activeSeriesTest) const {
  assert(activeSeriesIndex >= 0 && activeSeriesIndex < numberOfActiveSeries());
  int activeSeriesCount = 0;
  for (int i = 0; i < k_numberOfSeries; i++) {
    if (activeSeriesTest(this, i)) {
      if (activeSeriesCount == activeSeriesIndex) {
        return i;
      }
      activeSeriesCount++;
    }
  }
  assert(false);
  return 0;
}

int DoublePairStore::activeSeriesIndexFromSeriesIndex(
    int seriesIndex, ActiveSeriesTest activeSeriesTest) const {
  assert(0 <= seriesIndex && seriesIndex < k_numberOfSeries);
  assert(activeSeriesTest(this, seriesIndex));
  int activeSeriesCount = 0;
  for (int i = 0; i < seriesIndex; i++) {
    activeSeriesCount += activeSeriesTest(this, i);
  }
  assert(activeSeriesCount < numberOfActiveSeries());
  return activeSeriesCount;
}

static void swapRows(int a, int b, void *ctx, int numberOfElements) {
  // Swap X and Y values
  void **pack = reinterpret_cast<void **>(ctx);
  DoublePairStore *store = reinterpret_cast<DoublePairStore *>(pack[0]);
  int *series = reinterpret_cast<int *>(pack[1]);
  double dataAx = store->get(*series, 0, a);
  double dataAy = store->get(*series, 1, a);
  store->set(store->get(*series, 0, b), *series, 0, a, true);
  store->set(store->get(*series, 1, b), *series, 1, a, true);
  store->set(dataAx, *series, 0, b, true);
  store->set(dataAy, *series, 1, b, true);
};

static bool compare(int a, int b, void *ctx, int numberOfElements) {
  void **pack = reinterpret_cast<void **>(ctx);
  const DoublePairStore *store =
      reinterpret_cast<const DoublePairStore *>(pack[0]);
  int *series = reinterpret_cast<int *>(pack[1]);
  int *column = reinterpret_cast<int *>(pack[2]);
  double dataA = store->get(*series, *column, a);
  double dataB = store->get(*series, *column, b);
  return dataA >= dataB || std::isnan(dataA);
};

double DoublePairStore::CalculationOptions::transformValue(double value,
                                                           int i) const {
  value *= oppositeOfValue(i) ? -1.0 : 1.0;
  return lnOfValue(i) ? std::log(value) : value;
}

void DoublePairStore::sortColumn(int series, int column, bool delayUpdate) {
  assert(column == 0 || column == 1);

  void *context[] = {const_cast<DoublePairStore *>(this), &series, &column};
  Poincare::Helpers::Sort(swapRows, compare, context,
                          numberOfPairsOfSeries(series));
  updateSeries(series, delayUpdate);
}

void DoublePairStore::sortIndexByColumn(uint8_t *sortedIndex, int series,
                                        int column, int startIndex,
                                        int endIndex) const {
  assert(startIndex < endIndex);
  void *pack[] = {const_cast<DoublePairStore *>(this), sortedIndex + startIndex,
                  &series, &column};
  Poincare::Helpers::Sort(
      [](int i, int j, void *ctx, int n) {  // Swap method
        void **pack = reinterpret_cast<void **>(ctx);
        uint8_t *sortedIndex = reinterpret_cast<uint8_t *>(pack[1]);
        uint8_t t = sortedIndex[i];
        sortedIndex[i] = sortedIndex[j];
        sortedIndex[j] = t;
      },
      [](int i, int j, void *ctx, int n) {  // Comparison method
        void **pack = reinterpret_cast<void **>(ctx);
        const DoublePairStore *store =
            reinterpret_cast<const DoublePairStore *>(pack[0]);
        uint8_t *sortedIndex = reinterpret_cast<uint8_t *>(pack[1]);
        int series = *reinterpret_cast<int *>(pack[2]);
        int column = *reinterpret_cast<int *>(pack[3]);
        return store->get(series, column, sortedIndex[i]) >=
               store->get(series, column, sortedIndex[j]);
      },
      pack, endIndex - startIndex);
}

double DoublePairStore::sumOfColumn(int series, int i,
                                    CalculationOptions options) const {
  assert(series >= 0 && series < k_numberOfSeries);
  assert(i == 0 || i == 1);
  double result = 0;
  for (int k = 0; k < numberOfPairsOfSeries(series); k++) {
    result += options.transformValue(get(series, i, k), i);
  }
  return result;
}

uint32_t DoublePairStore::storeChecksum() const {
  uint32_t checkSumPerSeries[k_numberOfSeries];
  for (int i = 0; i < k_numberOfSeries; i++) {
    checkSumPerSeries[i] = storeChecksumForSeries(i);
  }
  return Ion::crc32DoubleWord(checkSumPerSeries, k_numberOfSeries);
}

/* TODO: This function is temporary. We want to create a function with a
 * similar behaviour in Ion in a near future.
 * This was copy pasted from the crc32EatByte function in the kernel. */
constexpr uint32_t polynomialForCrc = 0x04C11DB7;
uint32_t crc32EatByte(uint32_t crc, uint8_t data) {
  crc ^= data << 24;
  for (int i = 8; i--;) {
    crc = crc & 0x80000000 ? ((crc << 1) ^ polynomialForCrc) : (crc << 1);
  }
  return crc;
}

uint32_t DoublePairStore::storeChecksumForSeries(int series) const {
  /* Since the pool is not packed, it's noisy and we cannot just compute
   * the CRC32 of the expressionNode in the pool.
   * So we have to build it from the values of the columns. */
  uint32_t crc = 0;
  /* If serie is not valid, it can mean it has been hidden
   * thus checksum must change. */
  if (numberOfPairsOfSeries(series) == 0 || !seriesIsActive(series)) {
    return crc;
  }
  for (int j = 0; j < numberOfPairsOfSeries(series); j++) {
    for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
      double value = get(series, i, j);
      for (size_t index = 0; index < sizeof(double) / sizeof(uint8_t);
           index++) {
        crc = crc32EatByte(crc, *(reinterpret_cast<uint8_t *>(&value) + index));
      }
    }
  }
  return crc;
}

double DoublePairStore::defaultValue(int series, int i, int j) const {
  return (i == 0 && j > 1) ? 2 * get(series, i, j - 1) - get(series, i, j - 2)
                           : defaultValueForColumn1();
}

bool DoublePairStore::updateSeries(int series, bool delayUpdate) {
  assert(series >= 0 && series < k_numberOfSeries);
  if (delayUpdate) {
    return true;
  }
  deleteTrailingUndef(series, 0);
  deleteTrailingUndef(series, 1);
  deletePairsOfUndef(series);
  bool success = storeColumn(series, 0);
  success = success && storeColumn(series, 1);
  if (!success) {
    /* Column couldn't be updated in the store. Revert lists from storage state
     * and make sure updateSeries isn't called again. */
    initListsFromStorage(true);
  } else {
    updateSeriesValidity(series);
  }
  return success;
}

bool DoublePairStore::storeColumn(int series, int i) const {
  char name[k_columnNamesLength + 1];
  size_t nameLength = fillColumnName(series, i, name);
  if (lengthOfColumn(series, i) == 0) {
    Record(name, listExtension).destroy();
    return true;
  }
  Symbol listSymbol = Symbol::Builder(name, nameLength);
  return m_context->setExpressionForSymbolAbstract(m_dataLists[series][i],
                                                   listSymbol);
}

void DoublePairStore::deleteTrailingUndef(int series, int i) {
  int columnLength = lengthOfColumn(series, i);
  for (int j = columnLength - 1; j >= 0; j--) {
    if (!std::isnan(get(series, i, j))) {
      return;
    }
    m_dataLists[series][i].removeValueAtIndex(j);
  }
}

void DoublePairStore::deletePairsOfUndef(int series) {
  int j = 0;
  int numberOfPairs = numberOfPairsOfSeries(series);
  while (j < numberOfPairs) {
    if (std::isnan(get(series, 0, j)) && std::isnan(get(series, 1, j))) {
      for (int i = 0; i < k_numberOfColumnsPerSeries; i++) {
        if (j < lengthOfColumn(series, i)) {
          m_dataLists[series][i].removeValueAtIndex(j);
        }
      }
      j--;
      numberOfPairs--;
    }
    j++;
  }
}

}  // namespace Shared
