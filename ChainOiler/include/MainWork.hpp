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

namespace esp32s2
{
  class ButtonControl;
  class LedControl;
  class PumpControl;
  class TachoControl;
  class RainSensorControl;
}  // namespace esp32s2

namespace ChOiler
{
  /**
   * Auch ein statisches Objekt, ist nur enmalig vorhanden und spart damit RAM
   */
  class MainWorker
  {
    private:
    static const char *tag;                   //! Kennzeichnung fürs debug
    esp_sleep_wakeup_cause_t wakeupCause;     //! der Grund für den bootvorgang
    esp32s2::ButtonControl *buttonControl;    //! Zeiger auf esp32s2::ButtonControl
    esp32s2::LedControl *ledControl;          //! Zeiger auf esp32s2::LedControl
    esp32s2::PumpControl *pumpControl;        //! Zeiger auf esp32s2::PumpControl
    esp32s2::TachoControl *tachoControl;      //! Zeiger auf esp32s2::TachoControl
    esp32s2::RainSensorControl *rainControl;  //! Zeiger auf esp32s2::RainSensorControl

    protected:
    public:
    MainWorker();  //! initialisiert Prferenzen und Hardware
    void run();    //! da geht es los
    esp_sleep_wakeup_cause_t getWakeupCause()
    {
      return wakeupCause;
    };                                 //! Grund fürs booten nennen
    void switchToAccessPointMode();    //! AP Mode einschalten
    void switchFromAccessPointMode();  //! AP Mode aus zum Normal

    private:
    void goDeepSleep();          //! schlaf schön
    void processStartupCause();  //! bearbeite Sachen beim startup
  };

}  // namespace ChOiler
