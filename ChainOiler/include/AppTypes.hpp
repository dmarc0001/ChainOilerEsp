#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace Prefs
{
  enum whichLed : uint8_t
  {
    WICH_LED_NONE = 0,
    WICH_LED_CONTROL = 0x01,
    WICH_LED_RAIN = 0x02,
    WICH_LED_PUMP = 0x04,
    WICH_LED_REED = 0x08,
    WICH_LED_ALL = 0xff
  };

  //! Typ des Tastendrucks (Kein, kurz,lang)
  enum fClick : uint8_t
  {
    NONE,
    SHORT,
    LONG
  };

  //! Betriebsart des Ölers (aufwecken,normal,normal mit Achtung,regen, cross, accesspoint, test)
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
