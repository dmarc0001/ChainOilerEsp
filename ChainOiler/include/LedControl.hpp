#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/ledc.h>
#include <driver/dedic_gpio.h>
#include <esp_log.h>
#
#include "driver/rmt.h"
#include "led_strip.h"
#
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{
  constexpr rmt_channel_t RMT_TX_CHANNEL = RMT_CHANNEL_0;
  constexpr int NUM_OF_LED_PER_STRIP = 1;
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
    static const char *tag;
    static volatile int64_t pumpLedSwitchedOff;         //! wan soll die punken-LED aus?
    static volatile int64_t controlLedSwitchedOffDelta; // wann soll die Control LED wieder aus?
    static volatile int64_t controlLedSwitchedOff;      // wann soll die Control LED wieder aus?
    static volatile int64_t rainLedSwitchedOff;         // wann soll die Control LED wieder aus?
    static volatile int64_t apModeLedSwitchOff;         // wan soll ap-mode ausgeschakltet werden?
    static esp_timer_handle_t timerHandle;              //! da handle zum timer für die LED
    static dedic_gpio_bundle_handle_t ledBundle;        //! gebündeltes GPIO Array

  public:
    static void
    init();
    static void loop();
    static void allOff();               //! alles ausschalten
    static void setRainLED(bool);       //! LED für Regen einschalten
    static void setControlLED(int64_t); //! LED für ..ms ein
    static void setPumpLed(int64_t);    //! pump led ein ....ms
    static void setAPModeLED(int64_t);  //! LEDs für AP Mide ein

  private:
    static void startTimer();
    static void timerCallback(void *);
    LedControl(){};
  };
}
