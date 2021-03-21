#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_log.h>
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"

namespace esp32s2
{

  class ButtonControl
  {
  private:
    static const char *tag;
    static const uint8_t isr_control;                 //! Kennung für Control taster
    static const uint8_t isr_rain;                    //! Kennung für Regentaster
    static volatile int controlSwitchDown;            //! ist der Knopf gedrückt?
    static volatile uint64_t lastControlSwitchAction; //! wann war die letzte Änderung?
    static volatile int rainSwitchDown;               //! ist der Knopf gedrückt?
    static volatile uint64_t lastRainSwitchAction;    //! wann war die letzte Änderung?

  protected:
  public:
    static void init();
    static uint64_t controlDownSince(); //! seit wann gedrückt?

  private:
    ButtonControl(){};
    static void IRAM_ATTR buttonIsr(void *);
  };
}
