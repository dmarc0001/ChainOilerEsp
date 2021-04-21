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
#include "AppTypes.hpp"
#include "MainWork.hpp"

namespace esp32s2
{
  class TachoControl
  {
    private:
    static const char *tag;  //! TExtmarkierung fürs debugging
    static bool isInit;      //! hardware initialisiert?

    protected:
    static xQueueHandle pathLenQueue;  //! queue für Wegstreckenmeldungen
    static xQueueHandle speedQueue;    //! queue für Geschwindigkeitsmessung

    public:
    static void init();                               //! initialisiere die Hardware/software
    static void pause();                              //! halte das zählen an
    static void resume();                             //! weiter machen
    static void tachoCompute();                       //! Durchschnittsgeschwindighkeit errechnen
    friend void ChOiler::MainWorker::tachoCompute();  //! Freund darf hier rummachen

    private:
    static void IRAM_ATTR tachoOilerCountISR( void * );  //! ISR zum messen der Wegstrecke
    static void IRAM_ATTR speedCountISR( void * );       //! ISR zum messen der Geschwindigkeit
  };

}  // namespace esp32s2
