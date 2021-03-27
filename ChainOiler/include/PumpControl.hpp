#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/pcnt.h>
#include <esp_err.h>
#include "ProjectDefaults.hpp"

namespace esp32s2
{
  class PumpControl
  {
  private:
    static const char *tag;
    static esp_timer_handle_t timerHandle;
    static volatile bool pumpIsOn;

  public:
    static void init();
    static void stop();
    static void start();

  private:
    void timerCallback(void *);
    PumpControl(){};
  };
}
