#pragma once

#include "Prefs.hpp"
#include "ProjectDefaults.hpp"

using namespace Preferences;

class LedControl
{
  private:
  static uint32_t lastChanged;
  static uint32_t pumpSwitchedOn;

  public:
  static void loop();
  static void setRainLED( bool );
  static void setPumpLED( bool );
  static void showAttention();
};