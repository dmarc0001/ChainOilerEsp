#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/ledc.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "SignalControlAbstract.hpp"

namespace esp32s2
{
  constexpr ledc_timer_t LEDC_LS_TIMER = LEDC_TIMER_0;
  constexpr ledc_channel_t PWM_TIMER_CHANNEL_CONTROL = LEDC_CHANNEL_0;
  constexpr ledc_channel_t PWM_TIMER_CHANNEL_RAIN = LEDC_CHANNEL_1;
  constexpr ledc_channel_t PWM_TIMER_CHANNEL_PUMP = LEDC_CHANNEL_2;
  constexpr ledc_channel_t PWM_TIMER_CHANNEL_REED = LEDC_CHANNEL_3;
  constexpr int LED_CONTROL_CHANNEL = 0;
  constexpr int LED_RAIN_CHANNEL = 1;
  constexpr int LED_PUMP_CHANNEL = 2;
  constexpr int LED_REED_CHANNEL = 3;
  constexpr int LED_COUNT = 4;
  // constexpr uint32_t LED_MAX = 0b0111111111111U;
  constexpr uint32_t LED_MAX = 511U;
  constexpr uint32_t LED_MIN = 0U;
  constexpr uint32_t LED_DARK = static_cast<uint32_t>(LED_MAX / 10);
  constexpr int FADE_TIME = 1200;

  /**
   * @brief Objetk für LED mit PWM
   *
   *
   */
  class LedPwmControl : public SignalControlAbstract
  {
  protected:
    const char *tag = "led_pwm"; //! Hinweistext für logger

  private:
    ledc_channel_config_t ledc_channel[LED_COUNT];
    bool changed[LED_COUNT];

  public:
    LedPwmControl();
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
  };
}
