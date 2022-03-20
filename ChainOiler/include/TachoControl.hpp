#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <utility>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <driver/rtc_io.h>
#include <driver/uart.h>
#include <driver/pcnt.h>
#include <driver/adc.h>
#include <esp_sleep.h>
#include <esp_attr.h>
#include <esp_timer.h>

namespace ChOiler
{
  class MainWorker;
}

namespace esp32s2
{
  struct tachoQueueEntry_t
  {
    int64_t timestamp_us;
    uint32_t meters;
  };

  class TachoControl
  {
  private:
    static const char *tag;

  protected:
    static void init();
    static xQueueHandle speedQueue;

  public:
    friend class ChOiler::MainWorker;
    static void pause();
    static void resume();

  private:
    static void IRAM_ATTR tachoCountISR(void *);
  };

} // namespace esp32s2
