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

  public:
    static void init();

  private:
    PumpControl(){};
  };
}
