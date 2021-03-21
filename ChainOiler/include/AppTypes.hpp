#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace Prefs
{

  //! Typ des Tastendrucks (Kein, kurz,lang)
  enum fClick : uint8_t
  {
    NONE,
    SHORT,
    LONG
  };

  //! Betriebsart des Ölers (normal, regen, cross, accesspoint, test)
  enum opMode : uint8_t
  {
    AWAKE,
    NORMAL,
    RAIN,
    CROSS,
    APMODE,
    TEST
  };
}
