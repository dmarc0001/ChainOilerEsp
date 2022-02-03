#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/ledc.h>
#include <esp_log.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{
  class LedControl
  {
  private:
    static const char *tag;
    static uint64_t lastChanged;
    static uint64_t pumpLedSwitchedOff;    //! wan soll die punken-LED aus?
    static uint64_t controlLedSwitchedOff; // wann soll die Control LED wieder aus?
    static uint64_t apModeLedSwitchOff;    // wan soll ap-mode ausgeschakltet werden?
    static esp_timer_handle_t timerHandle;

  public:
    static void init();
    static void loop();
    static void allOff();
    static void showAttention();
    static void setRainLED(bool);
    static void setPumpLED(bool);
    static void setControlLED(uint32_t); //! LED für ..ms ein
    static void setAPModeLED(uint32_t);  //! LEDs für AP Mide ein

  private:
    static void startTimer();
    static void timerCallback(void *);
    LedControl(){};
  };
}
