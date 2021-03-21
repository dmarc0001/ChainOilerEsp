#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/pcnt.h>
#include <driver/adc.h>
#include <esp_log.h>
#include <esp_err.h>
#include "ProjectDefaults.hpp"

namespace esp32s2
{
  using rain_value_t = std::pair<int, int>;

  class RainSensorControl
  {
  private:
    static const char *tag;

  public:
    static void init();
    static rain_value_t getRainValues();

  private:
    RainSensorControl(){};
  };
}
