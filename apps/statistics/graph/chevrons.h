#ifndef STATISTICS_CHEVRONS_H
#define STATISTICS_CHEVRONS_H

#include <kandinsky/coordinate.h>

namespace Statistics {

class Chevrons {
public:
  static constexpr KDCoordinate k_chevronHeight = 8;
  static constexpr KDCoordinate k_chevronWidth = 10;
  static const uint8_t DownChevronMask[k_chevronHeight][k_chevronWidth];
  static const uint8_t UpChevronMask[k_chevronHeight][k_chevronWidth];
};

}

#endif
