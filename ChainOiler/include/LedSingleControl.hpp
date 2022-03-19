#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <driver/dedic_gpio.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{
  //
  // gpio group funktion, definitionen dafür
  //
  // {LED_REED_CONTROL, LED_CONTROL, LED_RAIN, LED_PUMP};
  constexpr uint32_t G_LED_REED_CONTROL_MASK = 0b00001;
  constexpr uint32_t G_LED_CONTROL_MASK = 0b00010;
  constexpr uint32_t G_LED_RAIN_MASK = 0b00100;
  constexpr uint32_t G_LED_PUMP_MASK = 0b01000;

  class LedControl
  {
  private:
    static const char *tag;                      //! Hinweistext für logger
    static dedic_gpio_bundle_handle_t ledBundle; //! gebündeltes GPIO Array
    static uint32_t ctrlLEDMask;                 //! Maske für zu bearbeitende Werte
    static uint32_t ctrlLedValue;                //! Werte

  public:
    static void init();                   //! hardware/values initialisieren
    static void allOff();                 //! alles ausschalten
    static void setRainLED(bool);         //! LED für Regen einschalten
    static void setControlLED(bool);      //! LED für ..ms ein
    static void setControlCrossLED(bool); //! LED für ..ms ein
    static void setPumpLED(bool);         //! pump led ein ....ms
    static void setAttentionLED(bool);    //! Achtung LED ein oder AUS
    static void setAPModeLED(bool);       //! LEDs für AP Mide ein
    static void makeChange();             //! Veränderungen setzen

  private:
    LedControl(){};
  };
}
