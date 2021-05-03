#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <esp_log.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "AppTypes.hpp"

namespace esp32s2
{
  class LedControl
  {
    private:
    static const char *tag;
    static LedControl inst;
    uint64_t lastChanged;
    uint64_t pumpLedSwitchOffTime;
    uint64_t nextControlLedFlash;
    esp_timer_handle_t timerHandle;
    uint8_t ledStateField;

    public:
    static LedControl *getInstance();  //! singleton
    ~LedControl();
    LedControl( LedControl const & ) = delete;
    void operator=( LedControl const & ) = delete;
    void allOff();              //! alles aus
    void showAttention();       //! alles blinken
    void setContolLED( bool );  //! control led setzen/löschen
    void setRainLED( bool );    //! regen led setzen/löschen
    void setPumpLED( bool );    //! pumpen led setzen/löschen

    private:
    void startTimer();                             //! timer für led steuerung starten
    static void timerCallback( void * );           //! timer callback für led
    void processLEDNormalMode();                   //! timer schleife in Normalmode
    void processLEDCrossMode();                    //! timer schleife in Crossmode
    void processLEDRainMode();                     //! timer schleife in Regenmode
    void processLEDApMode();                       //! in AP Mode
    void processControlLEDFlash( Prefs::opMode );  //! blitzen in den verschiedenen Modi
    LedControl();
  };
}  // namespace esp32s2
