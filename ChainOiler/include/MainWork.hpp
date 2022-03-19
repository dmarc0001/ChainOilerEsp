#pragma once

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <list>
#include <limits>
#include <time.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <driver/uart.h>
#include <esp_task_wdt.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "AppTypes.hpp"
#include "TachoControl.hpp"
#include "SignalControl.hpp"
#include "ButtonControl.hpp"
#include "PumpControl.hpp"
#include "RainSensorControl.hpp"
#include "ConfigWiFoAP.hpp"

namespace ChOiler
{
  /**
   * Auch ein statisches Objekt, ist nur enmalig vorhanden und spart damit RAM
   */
  class MainWorker
  {
  private:
    static const char *tag;                                     //! Kennzeichnung fürs debug
    static esp_sleep_wakeup_cause_t wakeupCause;                //! der Grund fürs neu starten
    static std::list<esp32s2::deltaTimeTenMeters_us> speedList; // Vector für Tachoauswertung
    static WiFiAccessPoint AccessPoint;                         //! das Objekt für WiFi

  protected:
  public:
    static void init();            //! initialisiert Prferenzen und Hardware
    static void run(void *);       //! da geht es los
    static void tachoCompute();    //! berechne Tacho Geschichten
    static void buttonStati();     //! guck was die Buttons machen
    static void computeAvgSpeed(); //! berechne Durchschnitt für max 4 Sekunden
    static void checkOilState();   //! Teste ob geölt werden muss
    static esp_sleep_wakeup_cause_t getWakeupCause() { return wakeupCause; };

  private:
    static void processStartupCause();       //! finde den Grund
    static void goDeepSleep();               //! schlaf schön
    static void switchToAccessPointMode();   //! AP Mode einschalten
    static void switchFromAccessPointMode(); //! AP Mode aus zum Normal
  };

} // namespace ChOiler
