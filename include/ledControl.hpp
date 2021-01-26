#pragma once

#include "Prefs.hpp"
#include "ProjectDefaults.hpp"

using namespace Preferences;

class LedControl
{
  private:
  static uint32_t lastChanged;

  public:
  static void loop();
};