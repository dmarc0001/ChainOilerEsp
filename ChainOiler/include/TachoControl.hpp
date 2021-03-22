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
#include <esp_log.h>

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

  class TachoControl
  {
  private:
    static const char *tag;                      //! TExtmarkierung fürs debugging
    static esp_sleep_wakeup_cause_t wakeupCause; //! der Grund für den bootvorgang
    static bool isInit;                          //! hardware initialisiert?

  protected:
    static xQueueHandle pathLenQueue; //! queue für Wegstreckenmeldungen
    static xQueueHandle speedQueue;   //! queue für Geschwindigkeitsmessung

  public:
    static void init();                                                       //! initialisiere die Hardware/software
    static void pause();                                                      //! halte das zählen an
    static void resume();                                                     //! weiter machen
    friend class ChOiler::MainWorker;                                         //! Freund darf hier rummachen
    static esp_sleep_wakeup_cause_t getWakeupCause() { return wakeupCause; }; //! Grund fürs booten nennen

  private:
    static void processStartupCause();                //! bearbeite Sachen beim startup
    static void IRAM_ATTR tachoOilerCountISR(void *); //! ISR zum messen der Wegstrecke
    static void IRAM_ATTR speedCountISR(void *);      //! ISR zum messen der Geschwindigkeit
  };

} // namespace esp32s2
