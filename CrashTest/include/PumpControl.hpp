#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/pcnt.h>
#include <esp_err.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{
  class PumpControl
  {
  private:
    static const char *tag;
    static esp_timer_handle_t timerHandle; //! timer handle

  public:
    static void init();

  private:
    PumpControl(){};
    static void startTimer();
    static void timerCallback(void *);
  };
}
