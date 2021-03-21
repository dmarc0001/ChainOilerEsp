#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <list>
#include <limits>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "AppTypes.hpp"
#include "HardwareUtil.hpp"
#include "LedControl.hpp"
#include "ButtonControl.hpp"

namespace ChOiler
{
  /**
   * Auch ein statisches Objekt, ist nur enmalig vorhanden und spart damit RAM
   */
  class MainWorker
  {
  private:
    static const char *tag;                                     //! Kennzeichnung fürs debug
    static std::list<esp32s2::deltaTimeTenMeters_us> speedList; // Vector für Tachoauswertung

  protected:
  public:
    static void init();            //! initialisiert Prferenzen und Hardware
    static void run();             //! da geht es los
    static void tachoCompute();    //! berechne Tacho Geschichten
    static void buttonStati();     //! guck was die Buttons machen
    static void computeAvgSpeed(); //! berechne Durchschnitt für max 4 Sekunden
  };

} // namespace ChOiler
