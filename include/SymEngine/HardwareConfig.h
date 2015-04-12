#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

namespace SymEngine {

struct HardwareConfig {
  int banksNumber;
  int bankWidth;
  int warpSize;
  int cacheLineSize;
};

}

#endif
