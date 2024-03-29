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
  using pcnt_evt_t = struct
  {
    // pulse counter queue structur
    pcnt_unit_t unit;         //! the PCNT unit that originated an interrupt
    pcnt_evt_type_t evt_type; //! INT type
    int16_t value;            //! value
    uint16_t meters;          //! anzahl Meter
  };
  using deltaTimeTenMeters_us = uint64_t; //! zeitstempel für 10 Meter
  using pathLenMeters = uint32_t;         // Wegstrecke in Metern

  class TachoControl
  {
  private:
    static const char *tag;

  protected:
    static void init();
    static xQueueHandle pathLenQueue;
    static xQueueHandle speedQueue;

  public:
    friend class ChOiler::MainWorker;
    static void pause();
    static void resume();

  private:
    static void IRAM_ATTR tachoOilerCountISR(void *);
    static void IRAM_ATTR speedCountISR(void *);
  };

} // namespace esp32s2
