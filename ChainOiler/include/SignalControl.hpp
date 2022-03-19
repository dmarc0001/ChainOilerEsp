#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <driver/dedic_gpio.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#ifdef RAWLED
#include "LedSingleControl.hpp"
#endif
#ifdef LEDSTRIPE
#include "LedStripeControl.hpp"
#endif

namespace esp32s2
{
  class SignalControl
  {
  private:
    static const char *tag;
    static volatile int64_t pumpLedSwitchedOff;         //! wan soll die punken-LED aus?
    static volatile int64_t controlLedSwitchedOffDelta; // wann soll die Control LED wieder aus?
    static volatile int64_t controlLedSwitchedOff;      // wann soll die Control LED wieder aus?
    static volatile int64_t rainLedSwitchedOff;         // wann soll die Control LED wieder aus?
    static volatile int64_t apModeLedSwitchOff;         // wan soll ap-mode ausgeschakltet werden?
    static esp_timer_handle_t timerHandle;              //! da handle zum timer für die LED

  public:
    static void init();            //! initialisiere das Objekt
    static void allOff();          //! alle LED aus
    static void flashControlLED(); //! control LED blitzen

  private:
    SignalControl(){};                 //! verbotener Konstruktor (Objekt ist nur statisch)
    static void startTimer();          //! Timer starten
    static void timerCallback(void *); //! Callback für den Timer
  };
}
