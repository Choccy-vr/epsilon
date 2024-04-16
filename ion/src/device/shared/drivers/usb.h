#ifndef ION_DEVICE_SHARED_USB_H
#define ION_DEVICE_SHARED_USB_H

#include <config/usb.h>
#include <ion/keyboard.h>
#include <shared/drivers/kernel_header.h>
#include <shared/drivers/userland_header.h>

namespace Ion {
namespace Device {
namespace USB {

void willExecuteDFU();
void didExecuteDFU();
bool shouldInterruptDFU(Keyboard::State exitKeys, bool bubbleUpEvents);
const char* stringDescriptor();

/* When entering DFU mode, we update this SlotInfo with a pointer to the current
 * PlatformInfo of the binary being run. The slot info is located at the very
 * beginning of the sRAM. */
class SlotInfo {
 public:
  SlotInfo() : m_header(Magic), m_footer(Magic) {}
  void updateKernelHeader();
  void updateUserlandHeader();
  KernelHeader* kernelHeader() const { return m_kernelHeaderAddress; }

 private:
  constexpr static uint32_t Magic = 0xEFEEDBBA;
  uint32_t m_header;
  KernelHeader* m_kernelHeaderAddress;
  UserlandHeader* m_userlandHeaderAddress;
  uint32_t m_footer;
};

SlotInfo* slotInfo();

}  // namespace USB
}  // namespace Device
}  // namespace Ion

#endif
