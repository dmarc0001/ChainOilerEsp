#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <driver/dedic_gpio.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "SignalControlAbstract.hpp"

#ifdef RAWLED
#if defined(LEDSTRIPE) || defined(LEDPWM)
#error Only ONE Signal hardware allowed
#endif
#include "LedSingleControl.hpp"
#endif

#ifdef LEDSTRIPE
#if defined(RAWLED) || defined(LEDPWM)
#error Only ONE Signal hardware allowed
#endif
#include "LedStripeControl.hpp"
#endif

#ifdef LEDPWM
#if defined(LEDSTRIPE) || defined(RAWLED)
#error Only ONE Signal hardware allowed
#endif
#include "LedPwmControl.hpp"
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
    static SignalControlAbstract *ledObject;            //! polymorphing mit den Anzeigen

  public:
    static void init();            //! initialisiere das Objekt
    static void allOff();          //! alle LED aus
    static void flashControlLED(); //! control LED blitzen
    static void flashCrossLED();   //! cross LED blitzen

  private:
    SignalControl(){};                 //! verbotener Konstruktor (Objekt ist nur statisch)
    static void startTimer();          //! Timer starten
    static void timerCallback(void *); //! Callback für den Timer
  };
}
