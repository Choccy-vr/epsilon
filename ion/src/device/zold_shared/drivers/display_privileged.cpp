#include "display_privileged.h"
#include "svcall.h"
#include <ion/display.h>
#include <ion/timing.h>
#include <drivers/config/display.h>
#include <assert.h>

#define USE_DMA_FOR_PUSH_PIXELS 0
#define USE_DMA_FOR_PUSH_COLOR 0

#define USE_DMA (USE_DMA_FOR_PUSH_PIXELS|USE_DMA_FOR_PUSH_COLOR)

namespace Ion {
namespace Display {

using namespace Device::Display;

/* TODO Hugo : Factorize the next 4 functions and their SVC equivalent.
 * Maybe they pushRectSVC() should just call pushRect() with retrieved params */

void pushRect(KDRect r, const KDColor * pixels) {
#if USE_DMA
  waitForPendingDMAUploadCompletion();
#endif
  setDrawingArea(r, Orientation::Landscape);
  pushPixels(pixels, r.width() * r.height());
}

void pushRectUniform(KDRect r, KDColor c) {
#if USE_DMA
  waitForPendingDMAUploadCompletion();
#endif
  setDrawingArea(r, Orientation::Portrait);
  pushColor(c, r.width() * r.height());
}

void pullRect(KDRect r, KDColor * pixels) {
#if USE_DMA
  waitForPendingDMAUploadCompletion();
#endif
  setDrawingArea(r, Orientation::Landscape);
  pullPixels(pixels, r.width()*r.height());
}

void POSTPushMulticolor(int rootNumberTiles, int tileSize) {
  for (int i = 0; i < rootNumberTiles; i++) {
    for (int j = 0; j < rootNumberTiles; j++) {
      uint16_t k = (i+j) % 16;
      uint16_t color = 1 << k;
      setDrawingArea(KDRect(i*tileSize,j*tileSize,tileSize, tileSize), Orientation::Landscape);
      pushColorAndContraryPixels(color, tileSize*tileSize);
    }
  }
}


}
}


namespace Ion {
namespace Device {
namespace Display {

using namespace Regs;

void pushRectSVC() {
  // Load rect and pixels
  void * args[2];
  getSvcallArgs(2, args);
  if (args[0] == nullptr || args[1] == nullptr) {
    return;
  }
  KDRect r = *static_cast<KDRect *>(args[0]);
  const KDColor * pixels = *static_cast<const KDColor **>(args[1]);

#if USE_DMA
  waitForPendingDMAUploadCompletion();
#endif
  setDrawingArea(r, Orientation::Landscape);
  pushPixels(pixels, r.width() * r.height());
}

void pushRectUniformSVC() {
  // Load rect and color
  void * args[2];
  getSvcallArgs(2, args);
  if (args[0] == nullptr || args[1] == nullptr) {
    return;
  }
  KDRect r = *static_cast<KDRect *>(args[0]);
  KDColor c = *static_cast<KDColor *>(args[1]);

#if USE_DMA
  waitForPendingDMAUploadCompletion();
#endif
  setDrawingArea(r, Orientation::Portrait);
  pushColor(c, r.width() * r.height());
}

void pullRectSVC() {
  // Load rect and pixels
  void * args[2];
  getSvcallArgs(2, args);
  if (args[0] == nullptr || args[1] == nullptr) {
    return;
  }
  KDRect r = *static_cast<KDRect *>(args[0]);
  KDColor * pixels = *static_cast<KDColor **>(args[1]);

#if USE_DMA
  waitForPendingDMAUploadCompletion();
#endif
  setDrawingArea(r, Orientation::Landscape);
  pullPixels(pixels, r.width()*r.height());
}

void POSTPushMulticolorSVC() {
  // Load rootNumberTiles and tileSize
  void * args[2];
  getSvcallArgs(2, args);
  if (args[0] == nullptr || args[1] == nullptr) {
    return;
  }
  int rootNumberTiles = *static_cast<int *>(args[0]);
  int tileSize = *static_cast<int *>(args[1]);

  for (int i = 0; i < rootNumberTiles; i++) {
    for (int j = 0; j < rootNumberTiles; j++) {
      uint16_t k = (i+j) % 16;
      uint16_t color = 1 << k;
      setDrawingArea(KDRect(i*tileSize,j*tileSize,tileSize, tileSize), Orientation::Landscape);
      pushColorAndContraryPixels(color, tileSize*tileSize);
    }
  }
}

static inline void send_data(uint16_t d) {
  *DataAddress = d;
}

static inline uint16_t receive_data() {
  return *DataAddress;
}

template<typename... Args>
static inline void send_data(uint16_t d, Args... other) {
  send_data(d);
  send_data(other...);
}

static inline void send_command(Command c) {
  *CommandAddress = c;
}

template<typename... Args>
static inline void send_command(Command c, Args... d) {
  send_command(c);
  send_data(d...);
}

static void send_long_command(Command c, int length, const uint8_t * data) {
  send_command(c);
  for (int i=0; i<length; i++) {
    send_data(data[i]);
  }
}

void init() {
#if USE_DMA
  initDMA();
#endif
  initGPIO();
  initFSMC();
  initPanel();
}

void shutdown() {
  shutdownPanel();
  shutdownFSMC();
  shutdownGPIO();
}

#if USE_DMA
void initDMA() {
  // Only DMA2 can perform memory-to-memory transfers
  //assert(Config::DMAEngine == DMA2);

  /* In memory-to-memory transfers, the "peripheral" is the source and the
   * "memory" is the destination. In other words, memory is copied from address
   * DMA_SxPAR to address DMA_SxM0AR. */

  Config::DMAEngine.SCR(Config::DMAStream)->setDIR(DMA::SCR::Direction::MemoryToMemory);
  Config::DMAEngine.SM0AR(Config::DMAStream)->set((uint32_t)DataAddress);
  Config::DMAEngine.SCR(Config::DMAStream)->setMSIZE(DMA::SCR::DataSize::HalfWord);
  Config::DMAEngine.SCR(Config::DMAStream)->setPSIZE(DMA::SCR::DataSize::HalfWord);
  Config::DMAEngine.SCR(Config::DMAStream)->setMBURST(DMA::SCR::Burst::Incremental4);
  Config::DMAEngine.SCR(Config::DMAStream)->setPBURST(DMA::SCR::Burst::Incremental4);
  Config::DMAEngine.SCR(Config::DMAStream)->setMINC(false);
}

static inline void startDMAUpload(const KDColor * src, bool incrementSrc, uint16_t length) {
  // Reset interruption markers
  Config::DMAEngine.LIFCR()->set(0xF7D0F7D);

  Config::DMAEngine.SNDTR(Config::DMAStream)->set(length);
  Config::DMAEngine.SPAR(Config::DMAStream)->set((uint32_t)src);
  Config::DMAEngine.SCR(Config::DMAStream)->setPINC(incrementSrc);
  Config::DMAEngine.SCR(Config::DMAStream)->setEN(true);
}
#endif

void initGPIO() {
  constexpr GPIO::OSPEEDR::OutputSpeed FSMCPinsSpeed = GPIO::OSPEEDR::OutputSpeed::Medium;
  // All the FSMC GPIO pins use the alternate function number 12
  for(const GPIOPin & g : Config::FSMCPins) {
    g.group().OSPEEDR()->setOutputSpeed(g.pin(), FSMCPinsSpeed);
    g.group().MODER()->setMode(g.pin(), GPIO::MODER::Mode::AlternateFunction);
    g.group().AFR()->setAlternateFunction(g.pin(), GPIO::AFR::AlternateFunction::AF12);
  }

  // Turn on the power
  Config::PowerPin.group().MODER()->setMode(Config::PowerPin.pin(), GPIO::MODER::Mode::Output);
  Config::PowerPin.group().ODR()->set(Config::PowerPin.pin(), true);

  // Turn on the reset pin
  Config::ResetPin.group().MODER()->setMode(Config::ResetPin.pin(), GPIO::MODER::Mode::Output);
  Config::ResetPin.group().ODR()->set(Config::ResetPin.pin(), true);

  // Turn on the extended command pin
  Config::ExtendedCommandPin.group().MODER()->setMode(Config::ExtendedCommandPin.pin(), GPIO::MODER::Mode::Output);
  Config::ExtendedCommandPin.group().ODR()->set(Config::ExtendedCommandPin.pin(), true);

  // Turn on the Tearing Effect pin
  Config::TearingEffectPin.group().OSPEEDR()->setOutputSpeed(Config::TearingEffectPin.pin(), FSMCPinsSpeed);
  Config::TearingEffectPin.group().MODER()->setMode(Config::TearingEffectPin.pin(), GPIO::MODER::Mode::Input);
  Config::TearingEffectPin.group().PUPDR()->setPull(Config::TearingEffectPin.pin(), GPIO::PUPDR::Pull::None);

  Timing::msleep(120);
}

void shutdownGPIO() {
  // All the FSMC GPIO pins use the alternate function number 12
  for(const GPIOPin & g : Config::FSMCPins) {
    g.group().OSPEEDR()->setOutputSpeed(g.pin(), GPIO::OSPEEDR::OutputSpeed::Low);
    g.group().MODER()->setMode(g.pin(), GPIO::MODER::Mode::Analog);
    g.group().PUPDR()->setPull(g.pin(), GPIO::PUPDR::Pull::None);
  }

  Config::ResetPin.group().MODER()->setMode(Config::ResetPin.pin(), GPIO::MODER::Mode::Analog);
  Config::ResetPin.group().PUPDR()->setPull(Config::ResetPin.pin(), GPIO::PUPDR::Pull::None);

  Config::PowerPin.group().MODER()->setMode(Config::PowerPin.pin(), GPIO::MODER::Mode::Analog);
  Config::PowerPin.group().PUPDR()->setPull(Config::PowerPin.pin(), GPIO::PUPDR::Pull::None);

  Config::ExtendedCommandPin.group().MODER()->setMode(Config::ExtendedCommandPin.pin(), GPIO::MODER::Mode::Analog);
  Config::ExtendedCommandPin.group().PUPDR()->setPull(Config::ExtendedCommandPin.pin(), GPIO::PUPDR::Pull::None);

  Config::TearingEffectPin.group().OSPEEDR()->setOutputSpeed(Config::TearingEffectPin.pin(), GPIO::OSPEEDR::OutputSpeed::Low);
  Config::TearingEffectPin.group().MODER()->setMode(Config::TearingEffectPin.pin(), GPIO::MODER::Mode::Analog);
}

constexpr static int nsToCycles(int nanoseconds) {
  return (nanoseconds*Config::HCLKFrequencyInMHz)/1000 + 1;
}

void initFSMC() {
  /* Set up the FSMC control registers.
   * We address the LCD panel as if it were an SRAM module, using a 16bits wide
   * bus, non-multiplexed.
   * The STM32 FSMC supports two kinds of memory access modes (see Reference
   * Manual) :
   *  - Base modes (1 and 2), which use the same timings for reads and writes
   *  - Extended modes (named A to D), which can be customized further.
   *  The LCD panel can be written to faster than it can be read from, therefore
   *  we want to use one of the extended modes. */
  FSMC.BCR(FSMCMemoryBank)->setEXTMOD(true);
  FSMC.BCR(FSMCMemoryBank)->setWREN(true);
  FSMC.BCR(FSMCMemoryBank)->setMWID(FSMC::BCR::MWID::SIXTEEN_BITS);
  FSMC.BCR(FSMCMemoryBank)->setMTYP(FSMC::BCR::MTYP::SRAM);
  FSMC.BCR(FSMCMemoryBank)->setMUXEN(false);
  FSMC.BCR(FSMCMemoryBank)->setMBKEN(true);

  /* We now need to set the actual timings. First, the FSMC and LCD specs don't
   * use the same names. Here's the mapping:
   *
   * FSMC | LCD
   * -----+-----
   *  NOE | RDX
   *  NWE | WRX
   *  NE1 | CSX
   *  A16 | D/CX
   *  Dn  | Dn
   *
   * We need to set the values of the BTR and BWTR which give the timings for
   * reading and writing. Note that the STM32 datasheet doesn't take into
   * account the time needed to actually switch from one logic state to another,
   * whereas the ST7789V one does, so we'll add T(edge) as needed.
   * Last but not least, timings on the STM32 have to be expressed in terms of
   * HCLK.
   *  - We'll pick Mode A because at the start of a read operation, NOE/RDX is
   *  high before the data is sent, just like in the LCD's datasheet.
   *  - The DATAST parameter is measured between a falling edge on NWE/NOE, and
   *  the rising edge on the same pin. In terms of LCD datasheet, that gives:
   *    t(DATAST) = t(WRL/RDLFM) + t(edge)
   *  - According to the STM32's datasheet, the total time of a read transaction
   *  is ADDSET + DATAST cycles, and ADDSET + DATAST + 1 cycles for a write one.
   *  This value is given explicitely as t(WC) and t(RCFM) in the LCD datasheet.
   *    t(ADDSET).w = t(WC) - t(DATAST).w - 1
   *    t(ADDSET).r = t(RCFM) - t(DATAST).r
   *  - BUSTURN is the delay between a read and a write transaction. The LCD
   *  datasheet asks for no such delay, so BUSTURN = 0.
   */

  constexpr int tedge = 15;
  constexpr int twc = 66;
  constexpr int trcfm = 450;
  constexpr int twrl = 15;
  constexpr int trdlfm = 355;

  constexpr int trdatast = trdlfm + tedge;
  constexpr int twdatast = twrl + tedge;

  // Read timing from the LCD
  FSMC.BTR(FSMCMemoryBank)->setADDSET(nsToCycles(trcfm-trdatast));
  FSMC.BTR(FSMCMemoryBank)->setADDHLD(0);
  FSMC.BTR(FSMCMemoryBank)->setDATAST(nsToCycles(trdatast));
  FSMC.BTR(FSMCMemoryBank)->setBUSTURN(0);
  FSMC.BTR(FSMCMemoryBank)->setACCMOD(FSMC::BTR::ACCMOD::A);

  // Write timings for the LCD
  FSMC.BWTR(FSMCMemoryBank)->setADDSET(nsToCycles(twc-twdatast)-1);
  FSMC.BWTR(FSMCMemoryBank)->setADDHLD(0);
  FSMC.BWTR(FSMCMemoryBank)->setDATAST(nsToCycles(twdatast));
  FSMC.BWTR(FSMCMemoryBank)->setBUSTURN(0);
  FSMC.BWTR(FSMCMemoryBank)->setACCMOD(FSMC::BWTR::ACCMOD::A);
}

void shutdownFSMC() {
}

void initPanel() {
  send_command(Command::Reset);
  Timing::msleep(5);

  send_command(Command::SleepOut);
  Timing::msleep(5);

  send_command(Command::PixelFormatSet, 0x05);
  send_command(Command::TearingEffectLineOn, 0x00);
  send_command(Command::FrameRateControl, 0x1E); // 40 Hz frame rate

  // Calibration
  const uint8_t * gammaCalibration = nullptr;
  uint32_t panelId = panelIdentifier();
  if (panelId == 0x4E4101 || panelId == 0x4E4801) {
    /* Don't forget the "static" qualifier, otherwise this array can be deleted
     * before reaching send_long_command. */
    static const uint8_t calibration[] = {0xA2, 0xA, 0x11, 0xA, 0xC, 0x1A, 0x34, 0x22, 0x4D, 0x28, 0x15, 0x13, 0x29, 0x2D};
    gammaCalibration = calibration;
  }
  if (gammaCalibration != nullptr) {
    send_long_command(Command::PositiveVoltageGammaControl, 14, gammaCalibration);
    send_long_command(Command::NegativeVoltageGammaControl, 14, gammaCalibration);
  }

  if (Config::DisplayInversion) {
    send_command(Command::DisplayInversionOn);
  }
  send_command(Command::DisplayOn);
}

void shutdownPanel() {
  send_command(Command::DisplayOff);
  send_command(Command::SleepIn);
  Timing::msleep(5);
}

uint32_t panelIdentifier() {
  send_command(Command::ReadDisplayID);
  receive_data(); // Dummy read, per datasheet
  uint8_t id1 = receive_data();
  uint8_t id2 = receive_data();
  uint8_t id3 = receive_data();

  return (id1 << 16) | (id2 << 8) | id3;
}

void pushColorAndContraryPixels(uint16_t value, int count) {
  send_command(Command::MemoryWrite);
  uint16_t color = value;
  while (count-- > 0) {
    send_data(color);
    color ^= 0xFFFF;
  }
}


void setDrawingArea(KDRect r, Orientation o) {
  uint16_t x_start, x_end, y_start, y_end;

  if (o == Orientation::Landscape) {
    send_command(Command::MemoryAccessControl, 0xA0);
    x_start = r.x();
    x_end = r.x() + r.width() - 1;
    y_start = r.y();
    y_end = r.y() + r.height() - 1;
  } else {
    send_command(Command::MemoryAccessControl, 0x00);
    x_start = r.y();
    x_end = r.y() + r.height() - 1;
    y_start = Ion::Display::Width - (r.x() + r.width());
    y_end = Ion::Display::Width - r.x() - 1;
  }

  send_command(
    Command::ColumnAddressSet,
    (x_start >> 8),
    (x_start & 0xFF),
    (x_end >> 8),
    (x_end & 0xFF)
  );

  send_command(
    Command::PageAddressSet,
    (y_start >> 8),
    (y_start & 0xFF),
    (y_end >> 8),
    (y_end & 0xFF)
  );
}

void pushPixels(const KDColor * pixels, size_t numberOfPixels) {
  send_command(Command::MemoryWrite);
  /* Theoretically, we should not be able to use DMA here. Indeed, we have no
   * guarantee that the content at "pixels" will remain valid once we exit this
   * function call. In practice, we might be able to use DMA here because most
   * of the time we push pixels from static locations. */
#if USE_DMA_FOR_PUSH_PIXELS
  startDMAUpload(pixels, true, numberOfPixels);
#else
  while (numberOfPixels > 8) {
    send_data(*pixels++);
    send_data(*pixels++);
    send_data(*pixels++);
    send_data(*pixels++);
    send_data(*pixels++);
    send_data(*pixels++);
    send_data(*pixels++);
    send_data(*pixels++);
    numberOfPixels -= 8;
  }
  while (numberOfPixels--) {
    send_data(*pixels++);
  }
#endif
}

void pushColor(KDColor color, size_t numberOfPixels) {
  send_command(Command::MemoryWrite);
#if USE_DMA_FOR_PUSH_COLOR
  /* The "color" variable lives on the stack. We cannot take its address because
   * it will stop being valid as soon as we return. An easy workaround is to
   * duplicate the content in a static variable, whose value is guaranteed to be
   * kept until the next pushColor call. */
  static KDColor staticColor;
  staticColor = color;
  startDMAUpload(&staticColor, false, (numberOfPixels > 64000 ? 64000 : numberOfPixels));
#else
  while (numberOfPixels--) {
    send_data(color);
  }
#endif
}

void pullPixels(KDColor * pixels, size_t numberOfPixels) {
  if (numberOfPixels == 0) {
    return;
  }
  send_command(Command::PixelFormatSet, 0x06);
  send_command(Command::MemoryRead);

  receive_data(); // First read is dummy data, per datasheet
  while (true) {
    if (numberOfPixels == 0) {
      break;
    }
    uint16_t one = receive_data();
    uint16_t two = receive_data();
    uint16_t firstPixel  = (one & 0xF800) | (one & 0xFC) << 3 | (two & 0xF800) >> 11;
    *pixels++ = KDColor::RGB16(firstPixel);
    numberOfPixels--;

    if (numberOfPixels == 0) {
      break;
    }
    uint16_t three = receive_data();
    uint16_t secondPixel = (two & 0xF8) << 8 | (three & 0xFC00) >> 5 | (three & 0xF8) >> 3;
    *pixels++ = KDColor::RGB16(secondPixel);
    numberOfPixels--;
  }
  send_command(Command::PixelFormatSet, 0x05);
}

#if USE_DMA
void waitForPendingDMAUploadCompletion() {
  // Loop until DMA engine available
  while (Config::DMAEngine.SCR(Config::DMAStream)->getEN()) {
  }
}
#endif

}
}
}
