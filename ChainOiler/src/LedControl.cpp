#include "LedControl.hpp"
#include <esp_err.h>

namespace esp32s2
{
  /**
   * @brief instanzieren und initialisieren der statischen variablen
   *
   */
  const char *LedControl::tag{"LedControl"};                 //! tag fürs debug logging
  volatile uint64_t LedControl::lastChanged{0ULL};           //! letzte Änderung
  volatile uint64_t LedControl::pumpLedSwitchedOff{false};   //! ist die Pumpen LED noch an?
  volatile uint64_t LedControl::controlLedSwitchedOff{0ULL}; // wann soll die Control LED wieder aus?
  volatile uint64_t LedControl::apModeLedSwitchOff{0ULL};    // wan soll ap-mode ausgeschakltet werden?
  esp_timer_handle_t LedControl::timerHandle{nullptr};       //! timer handle

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
            .name = "led_timer",
            .skip_unhandled_events = false};
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
      // ausschaltzeit setzten und einschalten
      pumpLedSwitchedOff = esp_timer_get_time() + Prefs::PUMP_LED_DELAY;
      gpio_set_level(Prefs::LED_PUMP, 1);
    }
    else
    {
      // deaktivieren
      pumpLedSwitchedOff = 0ULL;
      gpio_set_level(Prefs::LED_PUMP, 0);
    }
  }

  void LedControl::setControlLED(uint64_t timeout)
  {
    //
    // Ausschaltzeitpunkt setzen
    //
    controlLedSwitchedOff = esp_timer_get_time() + timeout;
    // LED einschalten
    gpio_set_level(Prefs::LED_CONTROL, 1);
  }

  void LedControl::setAPModeLED(uint64_t timeout)
  {
    //
    // ausschaltzeitpunkt setzen
    //
    apModeLedSwitchOff = esp_timer_get_time() + timeout;
    gpio_set_level(Prefs::LED_CONTROL, 1);
    gpio_set_level(Prefs::LED_RAIN, 1);
    gpio_set_level(Prefs::LED_PUMP, 1);
  }

  /**
   * @brief Callback für den Timer (100 ms)
   *
   */
  void LedControl::timerCallback(void *)
  {
    using namespace Prefs;
    static bool isPumpLedOn{false};
    uint64_t nowTime = esp_timer_get_time();

    //
    // Wenn der Accesspount wieder aus ist
    //
    if (apModeLedSwitchOff != 0ULL)
    {
      if (nowTime > controlLedSwitchedOff)
      {
        // die LED alle ausschalten
        LedControl::allOff();
        apModeLedSwitchOff = 0ULL;
      }
    }
    //
    // wenn die Zeit für die Control LED läuft
    //
    else if (controlLedSwitchedOff != 0ULL)
    {
      //
      // Control LED sollte noch an sein...
      // ist die zeit abgelaufen?
      //
      if (nowTime > controlLedSwitchedOff)
      {
        // Zeit abngelaufen,
        // ausschalten der LED
        gpio_set_level(Prefs::LED_CONTROL, 0);
        controlLedSwitchedOff = 0ULL;
      }
    }

    //
    // ist die Pumpe in action?
    //
    if (Preferences::pumpCycles > 0)
    {
      //
      // die Pumpe hat Arbeit
      // ist die LED für die Pumpe eingeschaltet?
      //
      if (!isPumpLedOn)
      {
        //
        // nein, muss noch eingeschaltet werden
        // LED einschalten
        //
        gpio_set_level(Prefs::LED_PUMP, 1);
        // ausschalten nach delay....
        pumpLedSwitchedOff = esp_timer_get_time() + Prefs::PUMP_LED_DELAY;
        // zustand vermerken
        isPumpLedOn = true;
      }
    }
    else
    {
      //
      // Pumpe ist fertig, hat keine Arbeit
      // ist eine delay-Zeit für die LED am laufen?
      //
      if (pumpLedSwitchedOff > 0ULL)
      {
        //
        // ist die Zeit fürs ausschalten gekommen?
        //
        if (nowTime > pumpLedSwitchedOff)
        {
          // ja
          // LED ausschalten
          gpio_set_level(Prefs::LED_PUMP, 0);
          // zustand vermerken
          isPumpLedOn = false;
          pumpLedSwitchedOff = 0ULL;
        }
      }
    }
  }

} // namespace
