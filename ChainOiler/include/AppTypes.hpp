#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/pcnt.h>

namespace esp32s2
{
  //! Struktur für den Zählerbaustein
  using pcnt_evt_t = struct
  {
    // pulse counter queue structur
    pcnt_unit_t unit;          //! the PCNT unit that originated an interrupt
    pcnt_evt_type_t evt_type;  //! INT type
    int16_t value;             //! value
    uint16_t meters;           //! anzahl Meter
  };

  //! Alias für den Zeitstempel
  using deltaTimeTenMeters_us = uint64_t;  //! zeitstempel für 10 Meter
}  // namespace esp32s2

namespace Prefs
{
  //! Bitfeld für die LED
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

}  // namespace Prefs
