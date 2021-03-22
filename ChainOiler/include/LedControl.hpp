#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/ledc.h>
#include <esp_log.h>
#include "AppTypes.hpp"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{

  class LedControl
  {
  private:
    static const char *tag;
    static uint64_t lastChanged;
    static uint64_t pumpLedSwitchOffTime;
    static uint64_t nextControlLedFlash;
    static esp_timer_handle_t timerHandle;
    static uint8_t ledStateField;

  public:
    static void init();             //! init hardware
    static void allOff();           //! alles aus
    static void showAttention();    //! alles blinken
    static void setContolLED(bool); //! control led setzen/löschen
    static void setRainLED(bool);   //! regen led setzen/löschen
    static void setPumpLED(bool);   //! pumpen led setzen/löschen

  private:
    static void startTimer();                          //! timer für led steuerung starten
    static void timerCallback(void *);                 //! timer callback für led
    static void processLEDNormalMode();                //! timer schleife in Normalmode
    static void processLEDCrossMode();                 //! timer schleife in Crossmode
    static void processLEDRainMode();                  //! timer schleife in Regenmode
    static void processLEDApMode();                    //! in AP Mode
    static void processControlLEDFlash(Prefs::opMode); //! blitzen in den verschiedenen Modi

    LedControl(){};
  };
}
