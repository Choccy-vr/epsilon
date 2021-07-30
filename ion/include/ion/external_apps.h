#ifndef ION_EXTERNAL_APPS_H
#define ION_EXTERNAL_APPS_H

#include <stdint.h>

namespace Ion {
namespace ExternalApps {

class App {
public:
  App(uint8_t * a);
  const char * name() const;
  uint32_t iconSize() const;
  const uint8_t * iconData() const;
  void * entryPoint() const;
  static bool appAtAddress(uint8_t * address);
private:
  uint8_t * addressAtIndexInAppInfo(int index) const;
  static constexpr uint32_t k_magic = 0xDEC0BEBA;
  uint8_t * m_startAddress;
};

class AppIterator {
public:
  AppIterator(uint8_t * address) : m_currentAddress(address) {}
  App operator*() { return App(m_currentAddress); }
  AppIterator& operator++();
  bool operator!=(const AppIterator& it) const { return m_currentAddress != it.m_currentAddress; }
private:
  uint8_t * m_currentAddress;
};

class Apps {
public:
  AppIterator begin() const;
  AppIterator end() const { return AppIterator(nullptr); };
};

void setVisible(bool visible);
int numberOfApps();

}
}

#endif

