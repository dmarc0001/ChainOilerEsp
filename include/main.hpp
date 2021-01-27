#pragma once

#include "HardwareInit.hpp"
#include "Prefs.hpp"
#include "ProjectDefaults.hpp"
#include "ledControl.hpp"
#include "webservice.hpp"

void setup();
void loop();
void checkRainSensor();
void checkControlKey();
void checkStartStopWLANService();