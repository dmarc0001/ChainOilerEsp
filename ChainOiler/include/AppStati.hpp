#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "AppTypes.hpp"

namespace ChOiler
{
  class MainWorker; // forward dekl
}
namespace esp32s2
{
  class EspCtrl;    // forward deklaration
  class LedControl; // forward deklaration

  class AppStati
  {
  private:
    static opMode appOpMode; //! In welchem Zustand ist das Programm

  protected:
    static volatile uint32_t tachoPulseCount;          //! zähle die impulse zum Streckenmessen
    static volatile uint32_t tachoPulseForSpeedCount;  //! zähle die impulse zum Geschwindigkeit messen
    static volatile uint32_t lastTachoPulse;           //! wann war der letzte Puls (deep sleep)
    static volatile bool functionSwitchDown;           //! ist der Knopf gedrückt?
    static volatile bool functionSwitchAction;         //! ist ein Ereignis?
    static volatile uint64_t lastFunctionSwitchAction; //! wann war die letzte Änderung?
    static volatile bool rainSwitchDown;               //! ist der Knopf gedrückt?
    static volatile bool rainSwitchAction;             //! ist ein ereignis?
    static volatile uint64_t lastRainSwitchAction;     //! wann war die letzte Änderung?
    static volatile uint8_t pumpCycles;                //! Anzahl der Pumenstösse, setzten aktiviert die Pumpe

  public:
    friend EspCtrl;                 //! befreundet
    friend LedControl;              // befreundet
    friend ChOiler::MainWorker;     // ! befreundet
    static opMode getAppMode();     //! gib Operationsmode zurück
    static void setAppMode(opMode); //! setzte Opertationsmode

  private:
    AppStati(){};
  };
}
