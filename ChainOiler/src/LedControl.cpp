#include "LedControl.hpp"
#include <esp_err.h>

namespace esp32s2
{
  const char *LedControl::tag{"LedControl"}; //! tag fürs debug logging
  uint64_t LedControl::lastChanged{0ULL};
  uint64_t LedControl::pumpLedSwitchedOn{false};
  esp_timer_handle_t LedControl::timerHandle{nullptr}; //! timer handle

  void LedControl::init()
  {
    using namespace Prefs;
    //
    // GPIO Konfigurieren
    //
    ESP_LOGD(tag, "%s: init GPIO for LED...", __func__);
    //
    // LED
    //
    gpio_config_t config_led = {.pin_bit_mask = BIT64(LED_REED_CONTROL) | BIT64(LED_CONTROL) | BIT64(LED_RAIN) | BIT64(LED_PUMP),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_led);
    LedControl::startTimer();
    ESP_LOGD(tag, "%s: init GPIO for LED...done", __func__);
  }

  void LedControl::startTimer()
  {
    //
    // timer für Punpe starten
    //
    const esp_timer_create_args_t appTimerArgs =
        {
            .callback = &LedControl::timerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "led_timer"};
    //
    // timer erzeugen
    //
    ESP_ERROR_CHECK(esp_timer_create(&appTimerArgs, &LedControl::timerHandle));
    //
    // timer starten, microsekunden ( 100 ms soll es)
    //
    ESP_ERROR_CHECK(esp_timer_start_periodic(LedControl::timerHandle, 100000));
    //
  }

  void LedControl::allOff()
  {
    gpio_set_level(Prefs::LED_CONTROL, 0);
    gpio_set_level(Prefs::LED_RAIN, 0);
    gpio_set_level(Prefs::LED_PUMP, 0);
  }

  void LedControl::showAttention()
  {
    static bool attentionLEDIsOn = false;
    uint64_t timeDiff = esp_timer_get_time() - LedControl::lastChanged;
    //
    if (attentionLEDIsOn && timeDiff > Prefs::BLINK_LED_ATTENTION_ON)
    {
      attentionLEDIsOn = false;
      gpio_set_level(Prefs::LED_CONTROL, 0);
      gpio_set_level(Prefs::LED_RAIN, 1);
      gpio_set_level(Prefs::LED_PUMP, 1);
      LedControl::lastChanged = esp_timer_get_time();
    }
    else if (!attentionLEDIsOn && timeDiff > Prefs::BLINK_LED_ATTENTION_OFF)
    {
      attentionLEDIsOn = true;
      gpio_set_level(Prefs::LED_CONTROL, 1);
      gpio_set_level(Prefs::LED_RAIN, 0);
      gpio_set_level(Prefs::LED_PUMP, 0);
      LedControl::lastChanged = esp_timer_get_time();
    }
  }

  void LedControl::setRainLED(bool _set)
  {
  }

  void LedControl::setPumpLED(bool _set)
  {
  }

  void LedControl::timerCallback(void *)
  {
    using namespace Prefs;
    static volatile bool haveSwitchedOn = false;

    if (haveSwitchedOn)
    {
      haveSwitchedOn = false;
      // Aus
      gpio_set_level(Prefs::OUTPUT_PUMP_CONTROL, 0);
      // set LED ON
    }
    else if (Preferences::pumpCycles > 0)
    {
      haveSwitchedOn = true;
      --Preferences::pumpCycles;
      // an
      gpio_set_level(Prefs::OUTPUT_PUMP_CONTROL, 1);
    }
  }

}
