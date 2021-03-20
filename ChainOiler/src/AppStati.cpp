#include "AppStati.hpp"

namespace esp32s2
{
  volatile uint32_t AppStati::tachoPulseCount{0};
  volatile uint32_t AppStati::tachoPulseForSpeedCount{0};
  volatile uint32_t AppStati::lastTachoPulse{0};
  volatile bool AppStati::functionSwitchDown{false};
  volatile uint64_t AppStati::lastFunctionSwitchAction{0ULL};
  volatile bool AppStati::rainSwitchDown{false};
  volatile uint64_t AppStati::lastRainSwitchAction{0ULL};
  volatile uint8_t AppStati::pumpCycles{0};

}
