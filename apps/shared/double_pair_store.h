#ifndef SHARED_DOUBLE_PAIR_STORE_H
#define SHARED_DOUBLE_PAIR_STORE_H

#include <kandinsky/color.h>
#include <escher/palette.h>
#include <stdint.h>
#include <assert.h>

namespace Shared {

class DoublePairStore {
public:
  constexpr static int k_numberOfSeries = 3;
  constexpr static int k_numberOfColumnsPerSeries = 2;
  constexpr static int k_maxNumberOfPairs = 100;
  DoublePairStore() :
    m_data{},
    m_validSeries{false, false, false},
    m_numberOfPairs{}
  {}
  // Delete the implicit copy constructor: the object is heavy
  DoublePairStore(const DoublePairStore&) = delete;

  // Get and set data
  double get(int series, int i, int j) const {
    assert(j <  m_numberOfPairs[series]);
    return m_data[series][i][j];
  }
  void set(double f, int series, int i, int j);

  // Counts
  int numberOfPairs() const;
  int numberOfPairsOfSeries(int series) const {
    assert(series >= 0 && series < k_numberOfSeries);
    return m_numberOfPairs[series];
  }

  // Delete and reset
  virtual bool deleteValueAtIndex(int series, int i, int j); // Return True if the pair was deleted.
  void deletePairOfSeriesAtIndex(int series, int j);
  void deleteColumn(int series, int i);
  void resetColumn(int series, int i);
  void deleteAllPairsOfSeries(int series);
  void deleteAllPairs();

  // Series
  bool hasValidSeries() const;
  bool seriesIsValid(int series) const;
  virtual void memoizeValidSeries(int series);
  int numberOfValidSeries() const;
  int indexOfKthValidSeries(int k) const;

  // Calculations
  double sumOfColumn(int series, int i, bool lnOfSeries = false) const;
  bool seriesNumberOfAbscissaeGreaterOrEqualTo(int series, int i) const;
  uint32_t storeChecksum() const;
  uint32_t storeChecksumForSeries(int series) const;

  // Colors
  static KDColor colorOfSeriesAtIndex(size_t i) {
    assert(i < k_numberOfSeries);
    assert(i < Escher::Palette::numberOfDataColors());
    return Escher::Palette::DataColor[i];
  }
  static KDColor colorLightOfSeriesAtIndex(size_t i) {
    assert(i < k_numberOfSeries);
    assert(i < Escher::Palette::numberOfLightDataColors());
    return Escher::Palette::DataColorLight[i];
  }
  double * data() { return reinterpret_cast<double*>(&m_data); }
protected:
  virtual double defaultValue(int series, int i, int j) const;
  double m_data[k_numberOfSeries][k_numberOfColumnsPerSeries][k_maxNumberOfPairs];
  bool m_validSeries[k_numberOfSeries];
private:
  int m_numberOfPairs[k_numberOfSeries];
};

}

#endif
