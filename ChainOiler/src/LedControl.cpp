#include "LedControl.hpp"
#include <esp_err.h>

namespace esp32s2
{
  /**
   * @brief instanzieren und initialisieren der statischen variablen
   * 
   */
  const char *LedControl::tag{"LedControl"};           //! tag fürs debug logging
  uint64_t LedControl::lastChanged{0ULL};              //! letzte Änderung
  uint64_t LedControl::pumpLedSwitchOffTime{0ULL};     //! ist die Pumpen LED noch an?
  uint64_t LedControl::nextControlLedFlash{0ULL};      //! das nächste mal Blitzen
  esp_timer_handle_t LedControl::timerHandle{nullptr}; //! timer handle
  uint8_t LedControl::ledState{0};                     //! Status der LED

  /**
   * @brief initialisierung der Hardware für die LED
   * 
   */
  void LedControl::init()
  {
    using namespace Prefs;
    //
    // GPIO Konfigurieren
    //
    ESP_LOGI(tag, "init hardware for LED...");
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
    ESP_LOGD(tag, "init hardware for LED...done");
  }

  /**
   * @brief eigene Timer routine für die Steuerung der LED
   * 
   */
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

  /**
   * @brief Alle LED ausschalten
   * 
   */
  void LedControl::allOff()
  {
    gpio_set_level(Prefs::LED_CONTROL, 0);
    gpio_set_level(Prefs::LED_RAIN, 0);
    gpio_set_level(Prefs::LED_PUMP, 0);
  }

  /**
   * @brief blinken mit allen LED (z.B. beim Booten)
   * 
   */
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

  /**
   * @brief schalte Regen LED an/aus
   * 
   * @param _set 
   */
  void LedControl::setRainLED(bool _set)
  {
    if (_set)
    {
      ledState |= Prefs::whichLed::WICH_LED_RAIN;
      gpio_set_level(Prefs::LED_RAIN, 1);
    }
    else
    {
      ledState &= ~Prefs::whichLed::WICH_LED_RAIN;
      gpio_set_level(Prefs::LED_RAIN, 0);
    }
  }

  /**
   * @brief schalte Pumpen LED
   * 
   * @param _set 
   */
  void LedControl::setPumpLED(bool _set)
  {
    if (_set)
    {
      // wann soll das Leuchten aufhören?
      LedControl::pumpLedSwitchOffTime = esp_timer_get_time() + Prefs::Preferences::getPumpLedTimeout();
      ledState |= Prefs::whichLed::WICH_LED_PUMP;
      gpio_set_level(Prefs::LED_PUMP, 1);
    }
    else
    {
      LedControl::pumpLedSwitchOffTime = 0ULL;
      ledState &= ~Prefs::whichLed::WICH_LED_PUMP;
      gpio_set_level(Prefs::LED_PUMP, 0);
    }
  }

  /**
   * @brief schalte control LED
   * 
   * @param _set 
   */
  void LedControl::setContolLed(bool _set)
  {
    if (_set)
    {
      ledState |= Prefs::whichLed::WICH_LED_CONTROL;
      gpio_set_level(Prefs::LED_CONTROL, 1);
    }
    else
    {
      ledState &= ~Prefs::whichLed::WICH_LED_CONTROL;
      gpio_set_level(Prefs::LED_CONTROL, 0);
    }
  }

  /**
   * @brief Callback für den Timer (100 ms)
   * 
   */
  void LedControl::timerCallback(void *)
  {

    using namespace Prefs;

    //
    // pumpen LED Nachlauf beendet?
    //
    if (LedControl::pumpLedSwitchOffTime > 0ULL)
    {
      if (esp_timer_get_time() > LedControl::pumpLedSwitchOffTime)
      {
        LedControl::setPumpLED(false);
      }
    }
    //
    // je nach Zustand arbeiten
    //
    switch (Preferences::getAppMode())
    {
    case opMode::NORMAL:
      LedControl::normalMode();
      break;
    default:
      break;
    }
  }

  /**
   * @brief Timer routine im mormalen Mode
   * 
   */
  void LedControl::normalMode()
  {
    //
    // das blitzen der control LED als Zeichen das es läuft
    if (esp_timer_get_time() > LedControl::nextControlLedFlash)
    {
      // da muss was passieren
      if (ledState & Prefs::whichLed::WICH_LED_CONTROL)
      {
        // ist on soll off
        ESP_LOGD(tag, "Control LED off");
        LedControl::setContolLed(0);
        LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_OFF;
      }
      else
      {
        // ist off soll on
        ESP_LOGD(tag, "Control LED on");
        LedControl::setContolLed(1);
        LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_ON;
      }
    }
  }

}
