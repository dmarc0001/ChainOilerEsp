#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <utility>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "driver/pcnt.h"
#include "driver/ledc.h"
#include <driver/adc.h>
#include "esp_sleep.h"
#include "esp_attr.h"
#include "esp_log.h"

namespace ChOiler
{
  class MainWorker;
}

namespace esp32s2
{
  void IRAM_ATTR tachoOilerCountISR(void *);

  using rain_value_t = std::pair<int, int>;
  using pcnt_evt_t = struct
  {
    // pulse counter queue structur
    pcnt_unit_t unit;         //! the PCNT unit that originated an interrupt
    pcnt_evt_type_t evt_type; //! INT type
    int16_t value;            //! value
  };

  class EspCtrl
  {
  private:
    static const char *tag;
    static esp_sleep_wakeup_cause_t wakeupCause;

  protected:
    static void init();
    static xQueueHandle pcnt_evt_queue;

  public:
    friend class ChOiler::MainWorker;
    friend void IRAM_ATTR tachoOilerCountISR(void *);
    static rain_value_t getRainValues();
    static esp_sleep_wakeup_cause_t getWakeupCause() { return wakeupCause; };
    static void goDeepSleep();

  private:
    static bool initTachoPulseCounter();
  };

} // namespace esp32s2
