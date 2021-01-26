#pragma once
#include <Arduino.h>
#include "Prefs.hpp"

void initHardware();
ICACHE_RAM_ATTR void tachoPulse();
ICACHE_RAM_ATTR void functionSwitch();