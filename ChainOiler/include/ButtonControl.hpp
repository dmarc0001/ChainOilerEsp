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
    const gpio_num_t isr_control{ Prefs::INPUT_CONTROL_SWITCH };     //! Kennung für Control taster
    const gpio_num_t isr_rain{ Prefs::INPUT_RAIN_SWITCH_OPTIONAL };  //! Kennung für Regentaster
    volatile int controlSwitchDown;                                  //! ist der Knopf gedrückt?
    volatile uint64_t lastControlSwitchAction;                       //! wann war die letzte Änderung?
    volatile int rainSwitchDown;                                     //! ist der Knopf gedrückt?
    volatile uint64_t lastRainSwitchAction;                          //! wann war die letzte Änderung?

    protected:
    public:
    static ButtonControl *getInstance();
    ~ButtonControl();
    void buttonStati();           //! Tastenstati und Reaktion darauf
    uint64_t controlDownSince();  //! seit wann gedrückt?
    ButtonControl( ButtonControl const & ) = delete;
    void operator=( ButtonControl const & ) = delete;

    private:
    ButtonControl();
    static void IRAM_ATTR buttonIsr( void * );
  };
}  // namespace esp32s2
