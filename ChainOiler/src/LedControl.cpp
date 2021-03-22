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
  uint8_t LedControl::ledStateField{0};                //! Status der LED

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
    LedControl::nextControlLedFlash = 0ULL;
    ledStateField &= ~(Prefs::whichLed::WICH_LED_CONTROL | Prefs::whichLed::WICH_LED_RAIN | Prefs::whichLed::WICH_LED_PUMP);
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
      ledStateField |= Prefs::whichLed::WICH_LED_RAIN;
      gpio_set_level(Prefs::LED_RAIN, 1);
    }
    else
    {
      ledStateField &= ~Prefs::whichLed::WICH_LED_RAIN;
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
      ledStateField |= Prefs::whichLed::WICH_LED_PUMP;
      gpio_set_level(Prefs::LED_PUMP, 1);
    }
    else
    {
      LedControl::pumpLedSwitchOffTime = 0ULL;
      ledStateField &= ~Prefs::whichLed::WICH_LED_PUMP;
      gpio_set_level(Prefs::LED_PUMP, 0);
    }
  }

  /**
   * @brief schalte control LED
   * 
   * @param _set 
   */
  void LedControl::setContolLED(bool _set)
  {
    if (_set)
    {
      ledStateField |= Prefs::whichLed::WICH_LED_CONTROL;
      gpio_set_level(Prefs::LED_CONTROL, 1);
    }
    else
    {
      ledStateField &= ~Prefs::whichLed::WICH_LED_CONTROL;
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
      LedControl::processControlLEDFlash(opMode::NORMAL);
      LedControl::processLEDNormalMode();
      break;
    case opMode::CROSS:
      LedControl::processControlLEDFlash(opMode::CROSS);
      LedControl::processLEDCrossMode();
      break;
    case opMode::RAIN:
      LedControl::processControlLEDFlash(opMode::CROSS);
      LedControl::processLEDRainMode();
      break;
    case opMode::APMODE:
      LedControl::processLEDApMode();
      break;
    default:
      break;
    }
  }

  /**
   * @brief Timer routine im mormalen Mode
   * 
   */
  void LedControl::processLEDNormalMode()
  {
    //
    // control led durch blinken gesetzt
    // regen hier AUS
    //
    if (ledStateField & Prefs::whichLed::WICH_LED_RAIN)
    {
      LedControl::setRainLED(0);
    }
  }

  /**
   * @brief zyklisch alle 100 ms in CROSS Mode
   * 
   */
  void LedControl::processLEDCrossMode()
  {
    //
    // control led durch blinken gesetzt
    // regen hier aus
    //
    if (ledStateField & Prefs::whichLed::WICH_LED_RAIN)
    {
      LedControl::setRainLED(0);
    }
  }

  /**
   * @brief zyklisch alle 100 ms in RAIN Mode
   * 
   */
  void LedControl::processLEDRainMode()
  {
    //
    // control led durch blinken gesetzt
    // regen LED hier AN
    //
    if (!(ledStateField & Prefs::whichLed::WICH_LED_RAIN))
    {
      LedControl::setRainLED(1);
    }
  }

  /**
   * @brief zyklisch alle 100 ms in AccesPoint Mode
   * 
   */
  void LedControl::processLEDApMode()
  {
    using namespace Prefs;
    //
    // das blitzen der control und Regen LED im wechsel als Zeichen das es läuft
    //
    if (esp_timer_get_time() > LedControl::nextControlLedFlash)
    {
      // da muss was passieren
      if (ledStateField & Prefs::whichLed::WICH_LED_CONTROL)
      {
        // ist on soll off
        // ESP_LOGD(tag, "Control LED off, Rain LED on");
        LedControl::setContolLED(0);
        LedControl::setRainLED(1);
        LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_AP_OFF;
      }
      else
      {
        // ist off soll on
        // ESP_LOGD(tag, "Control LED on, Rain LED of");
        LedControl::setContolLED(1);
        LedControl::setRainLED(0);
        LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_AP_ON;
      }
    }
  }

  /**
   * @brief zyklisch alle 100 ms zum blinken der Controll LED in normal und crossmode
   * 
   * @param _mode 
   */
  void LedControl::processControlLEDFlash(Prefs::opMode _mode)
  {
    using namespace Prefs;
    //
    // das blitzen der control LED als Zeichen das es läuft
    //
    if (esp_timer_get_time() > LedControl::nextControlLedFlash)
    {
      // da muss was passieren
      if (ledStateField & Prefs::whichLed::WICH_LED_CONTROL)
      {
        // ist on soll off
        // ESP_LOGD(tag, "Control LED off");
        LedControl::setContolLED(0);
        if (Prefs::Preferences::getAttentionFlag())
        {
          LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_ATTENTION_OFF;
        }
        else if (_mode == opMode::NORMAL || _mode == opMode::RAIN)
          LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_OFF;
        else
          LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_ON;
      }
      else
      {
        // ist off soll on
        // ESP_LOGD(tag, "Control LED on");
        LedControl::setContolLED(1);
        if (Prefs::Preferences::getAttentionFlag())
        {
          LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_ATTENTION_ON;
        }
        else if (_mode == opMode::NORMAL || _mode == opMode::RAIN)
          LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_ON;
        else
          LedControl::nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_OFF;
      }
    }
  }
}
