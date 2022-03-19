#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <driver/dedic_gpio.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{
  /**
   * @brief Abstraktes Objekt als Vorlage für Polymorphe Anwendung
   *
   * DAs Objekt ist "schablone" für signalisierungs-Objekte, hier
   * für "native" led/sdtripes/pwm led
   *
   */
  class SignalControlAbstract
  {
  protected:
    const char *tag = 0; //! Hinweistext für logger

  public:
    SignalControlAbstract(){};
    virtual void init() = 0;                   //! hardware/values initialisieren
    virtual void allOff() = 0;                 //! alles ausschalten
    virtual void setRainLED(bool) = 0;         //! LED für Regen einschalten
    virtual void setControlLED(bool) = 0;      //! LED Contol
    virtual void setControlCrossLED(bool) = 0; //! LED Cross
    virtual void setPumpLED(bool) = 0;         //! pump LED
    virtual bool fadeOutPumpLED() = 0;         //! pumpen LED ausblenden
    virtual void setAttentionLED(bool) = 0;    //! Achtung LED ein oder AUS
    virtual void setAPModeLED(bool) = 0;       //! LEDs für AP Mide ein
    virtual void makeChange() = 0;             //! Veränderungen setzen
  };
}
