#include <shared/drivers/flash_unprivileged.h>
#include <drivers/config/external_flash.h>
#include <assert.h>

namespace Ion {
namespace Device {
namespace Flash {

int TotalNumberOfSectors() {
  return ExternalFlash::Config::NumberOfSectors;
}

int SectorAtAddress(uint32_t address) {
  /* WARNING: this code assumes that the flash sectors are of increasing size:
   * first all 4K sectors, then all 32K sectors, and finally all 64K sectors. */
  address -= ExternalFlash::Config::StartAddress;
  int i = address >> ExternalFlash::Config::NumberOfAddressBitsIn64KbyteBlock;
  if (i > ExternalFlash::Config::NumberOf64KSectors) {
    return -1;
  }
  if (i >= 1) {
    return ExternalFlash::Config::NumberOf4KSectors + ExternalFlash::Config::NumberOf32KSectors + i - 1;
  }
  i = address >> ExternalFlash::Config::NumberOfAddressBitsIn32KbyteBlock;
  if (i >= 1) {
    assert(i >= 0 && i <= ExternalFlash::Config::NumberOf32KSectors);
    i = ExternalFlash::Config::NumberOf4KSectors + i - 1;
    return i;
  }
  i = address >> ExternalFlash::Config::NumberOfAddressBitsIn4KbyteBlock;
  assert(i <= ExternalFlash::Config::NumberOf4KSectors);
  return i;
}

bool IncludesAddress(uint32_t address) {
  return address >= ExternalFlash::Config::StartAddress
    && address <= ExternalFlash::Config::EndAddress;
}

}
}
}
