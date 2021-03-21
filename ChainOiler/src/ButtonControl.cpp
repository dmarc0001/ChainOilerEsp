#include "ButtonControl.hpp"
#include <esp_log.h>

namespace esp32s2
{
  /**
   * @brief statische Variablen instanzieren und initialisieren
   * 
   */
  const gpio_num_t ButtonControl::isr_control{Prefs::INPUT_CONTROL_SWITCH};    //! Marker für die ISR
  const gpio_num_t ButtonControl::isr_rain{Prefs::INPUT_RAIN_SWITCH_OPTIONAL}; //! Marker für die ISR
  volatile int ButtonControl::controlSwitchDown{false};                        //! ist der Controll Taster gedrückt
  volatile uint64_t ButtonControl::lastControlSwitchAction{0ULL};              //! wann war das letzte Control Tasten Ereignis
  volatile int ButtonControl::rainSwitchDown{false};                           //! der Regentaster ist gedrückt
  volatile uint64_t ButtonControl::lastRainSwitchAction{0ULL};                 //! wann war die letzte Regentaster Aktion
  const char *ButtonControl::tag{"ButtonControl"};                             //! tag fürs debug logging

  /**
   * @brief initialisiere Hardware für die Schalter 
   * 
   */
  void ButtonControl::init()
  {
    using namespace Prefs;

    ESP_LOGI(tag, "init button control hardware...");
    //
    //  Tacho und Knopf (Knopf-GPIO_INTR_ANYEDGE)
    //
    gpio_config_t config_in = {.pin_bit_mask = BIT64(INPUT_CONTROL_SWITCH) | BIT64(INPUT_RAIN_SWITCH_OPTIONAL),
                               .mode = GPIO_MODE_INPUT,
                               .pull_up_en = GPIO_PULLUP_ENABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type = GPIO_INTR_ANYEDGE};
    gpio_config(&config_in);
    //
    // Handler für die beiden Ports
    //
    gpio_isr_handler_add(INPUT_CONTROL_SWITCH, ButtonControl::buttonIsr, (void *)&ButtonControl::isr_control);
    gpio_isr_handler_add(INPUT_RAIN_SWITCH_OPTIONAL, ButtonControl::buttonIsr, (void *)&ButtonControl::isr_rain);
    ESP_LOGD(tag, "init button control hardware...done");
  }

  /**
   * @brief wie lange ist der Control schalter seit SHORT schon gedrückt
   * 
   * @return uint64_t 
   */
  uint64_t ButtonControl::controlDownSince()
  {
    //
    // wenn taste unten, bin ich im Bereich über SHORT?
    // beim langen drücken für's blinken
    //
    if (controlSwitchDown == 0)
    {
      if (esp_timer_get_time() > (ButtonControl::lastControlSwitchAction + Prefs::DEBOUNCE_TIME_US))
      {
        return ButtonControl::lastControlSwitchAction;
      }
    }
    return (0ULL);
  }

  /**
   * @brief ISR für Schalter Betätigungen
   * 
   * @param arg 
   */
  void IRAM_ATTR ButtonControl::buttonIsr(void *arg)
  {
    using namespace Prefs;

    gpio_num_t *_num = static_cast<gpio_num_t *>(arg);
    uint64_t now = esp_timer_get_time();
    int level;

    switch (*_num)
    {
    case Prefs::INPUT_CONTROL_SWITCH:
      level = gpio_get_level(Prefs::INPUT_CONTROL_SWITCH);
      //
      // Was ist passiert? Level 1 bedeutet Knopf gelöst
      //
      if ((level == 1) /*&& (ButtonControl::controlSwitchDown == 0)*/)
      {
        //
        // button gelöst, war gedrückt
        // mindestens für kurzen klick gedrückt?
        //
        if (now > (ButtonControl::lastControlSwitchAction + DEBOUNCE_TIME_US))
        {
          // schon mal gültiger Tastendruck
          if (now > (ButtonControl::lastControlSwitchAction + LONG_CLICK_TIME_US))
          {
            Preferences::setControlSwitchAction(fClick::LONG);
          }
          else
          {
            Preferences::setControlSwitchAction(fClick::SHORT);
          }
        }
      }
      ButtonControl::controlSwitchDown = level;
      ButtonControl::lastControlSwitchAction = now;
      break;

    case Prefs::INPUT_RAIN_SWITCH_OPTIONAL:
      level = gpio_get_level(Prefs::INPUT_RAIN_SWITCH_OPTIONAL);
      //
      // Was ist passiert? Level 0 bedeutet Knopf gedrückt
      //
      if (level == 1 && (ButtonControl::rainSwitchDown == 0))
      {
        //
        // button gelöst, war gedrückt
        //
        if (now > (ButtonControl::lastRainSwitchAction + DEBOUNCE_TIME_US))
        {
          // short click
          Preferences::setRainSwitchAction(fClick::SHORT);
        }
      }
      ButtonControl::rainSwitchDown = level;
      ButtonControl::lastRainSwitchAction = now;
      break;
    default:
      break;
    }
  }

}
