#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/ledc.h>
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

  class LedControl
  {
  private:
    static const char *tag;
    static volatile int64_t lastChanged;
    static volatile int64_t pumpLedSwitchedOff;    //! wan soll die punken-LED aus?
    static volatile int64_t controlLedSwitchedOff; // wann soll die Control LED wieder aus?
    static volatile int64_t apModeLedSwitchOff;    // wan soll ap-mode ausgeschakltet werden?
    static esp_timer_handle_t timerHandle;

  public:
    static void init();
    static void loop();
    static void allOff();
    static void showAttention();
    static void setRainLED(bool);
    static void initWroverLED();
    // static void setPumpLED(bool);
    static void setControlLED(int64_t); //! LED für ..ms ein
    static void setAPModeLED(int64_t);  //! LEDs für AP Mide ein

  private:
    static void startTimer();
    static void timerCallback(void *);
    LedControl(){};
  };
}
