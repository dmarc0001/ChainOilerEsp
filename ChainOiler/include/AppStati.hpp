#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace esp32s2
{
  class EspCtrl; // forward deklaration

  class AppStati
  {
  protected:
    static volatile uint32_t tachoPulseCount;          //! zähle die impulse zum Streckenmessen
    static volatile uint32_t tachoPulseForSpeedCount;  //! zähle die impulse zum Geschwindigkeit messen
    static volatile uint32_t lastTachoPulse;           //! wann war der letzte Puls (deep sleep)
    static volatile bool functionSwitchDown;           //! ist der Knopf gedrückt?
    static volatile uint64_t lastFunctionSwitchAction; //! wann war die letzte Änderung?
    static volatile bool rainSwitchDown;               //! ist der Knopf gedrückt?
    static volatile uint64_t lastRainSwitchAction;     //! wann war die letzte Änderung?
    static volatile uint8_t pumpCycles;                //! Anzahl der Pumenstösse, setzten aktiviert die Pumpe

  public:
    friend esp32s2::EspCtrl; //! befreundet

  private:
    AppStati(){};
  };
}
