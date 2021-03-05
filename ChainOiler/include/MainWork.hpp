#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "esp_log.h"

#include "ProjectDefaults.hpp"
#include "HardwareUtil.hpp"
#include "AppPreferences.hpp"

namespace ChOiler
{
  /**
   * Auch ein statisches Objekt, ist nur enmalig vorhanden und spart damit RAM
   */
  class MainWorker
  {
  private:
    static const char *tag; //! Kennzeichnung fürs debug

  protected:
  public:
    static void init();        //! initialisiert Prferenzen und Hardware
    static void run();         //! da geht es los
    static void defaultLoop(); //! schleife in der der Controller läuft, normale Betriebsart
  };

} // namespace ChOiler
