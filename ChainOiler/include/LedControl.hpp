#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/ledc.h>
#include <esp_log.h>
#include "ProjectDefaults.hpp"
#include "AppStati.hpp"

namespace esp32s2
{
  class LedControl
  {
  private:
    static const char *tag;
    static uint64_t lastChanged;
    static uint64_t pumpLedSwitchedOn;
    static esp_timer_handle_t timerHandle;

  public:
    static void init();
    static void loop();
    static void allOff();
    static void showAttention();
    static void setRainLED(bool);
    static void setPumpLED(bool);

  private:
    static void startTimer();
    static void timerCallback(void *);
    LedControl(){};
  };
}
