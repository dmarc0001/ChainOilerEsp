#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include <driver/dedic_gpio.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "SignalControlAbstract.hpp"

namespace esp32s2
{
  //
  // gpio group funktion, definitionen dafür
  //
  // {LED_REED_CONTROL, LED_CONTROL, LED_RAIN, LED_PUMP};
  constexpr uint32_t G_LED_REED_CONTROL_MASK = 0b00001;
  constexpr uint32_t G_LED_CONTROL_MASK = 0b00010;
  constexpr uint32_t G_LED_RAIN_MASK = 0b00100;
  constexpr uint32_t G_LED_PUMP_MASK = 0b01000;

  class LedControl : public SignalControlAbstract
  {
    //
    // bein instsanzieren dürfen keine Ports doppelt genutzt werden
    // TODO: Sperre einbuen
    //
  protected:
    const char *tag = "led_single";

  private:
    dedic_gpio_bundle_handle_t ledBundle; //! gebündeltes GPIO Array
    uint32_t ctrlLEDMask;                 //! Maske für zu bearbeitende Werte
    uint32_t ctrlLedValue;                //! Werte

  public:
    LedControl();
    virtual void init();                   //! hardware/values initialisieren
    virtual void allOff();                 //! alles ausschalten
    virtual void setRainLED(bool);         //! LED für Regen einschalten
    virtual void setControlLED(bool);      //! LED für Control ein
    virtual void setControlCrossLED(bool); //! LED für Cross ein
    virtual void setPumpLED(bool);         //! LED für Pumpe ein
    virtual bool fadeOutPumpLED();         //! Pumpen LED ausblenden
    virtual void setAttentionLED(bool);    //! Achtung LED ein oder AUS
    virtual void setAPModeLED(bool);       //! LEDs für AP Mide ein
    virtual void makeChange();             //! Veränderungen setzen

  private:
  };
}
