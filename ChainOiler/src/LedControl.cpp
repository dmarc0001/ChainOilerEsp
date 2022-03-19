#include "LedControl.hpp"
#include <esp_err.h>

namespace esp32s2
{
  /**
   * @brief instanzieren und initialisieren der statischen variablen
   *
   */
  const char *LedControl::tag{"LedControl"};                    //! tag fürs debug logging
  volatile int64_t LedControl::pumpLedSwitchedOff{false};       //! ist die Pumpen LED noch an?
  volatile int64_t LedControl::controlLedSwitchedOffDelta{0LL}; //! wann soll die Control LED wieder aus?
  volatile int64_t LedControl::controlLedSwitchedOff{0LL};      //! wann soll die Control LED wieder aus?
  volatile int64_t LedControl::rainLedSwitchedOff{0LL};         //! wann soll die Rgen-LED wieder aus?
  volatile int64_t LedControl::apModeLedSwitchOff{0LL};         //! wann soll ap-mode ausgeschakltet werden?
  esp_timer_handle_t LedControl::timerHandle{nullptr};          //! timer handle
  dedic_gpio_bundle_handle_t LedControl::ledBundle{nullptr};    //! gebündeltes GPOI Array

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
    //
    // erstelle ein GPIO Bundle, das macht Ausgaben zu OUT in einem Befehl
    //
    const int bundleGPIOs[] = {LED_REED_CONTROL, LED_CONTROL, LED_RAIN, LED_PUMP};
    //
    dedic_gpio_bundle_config_t bundleConfig =
        {
            .gpio_array = bundleGPIOs,
            .array_size = sizeof(bundleGPIOs) / sizeof(bundleGPIOs[0]),
            .flags = {
                .in_en = 0,
                .in_invert = 0,
                .out_en = 1,
                .out_invert = 0},
        };
    ESP_ERROR_CHECK(dedic_gpio_new_bundle(&bundleConfig, &LedControl::ledBundle));
    //
    // Timer starten
    //
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
    ESP_ERROR_CHECK(esp_timer_start_periodic(LedControl::timerHandle, 100000ULL));
    //
  }

  /**
   * @brief Alle LED ausschalten
   *
   */
  void LedControl::allOff()
  {
    uint32_t ctrlLEDMask = G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
    dedic_gpio_bundle_write(LedControl::ledBundle, ctrlLEDMask, 0U);
  }

  /**
   * @brief schalte Regen LED an/aus
   *
   * @param _set
   */
  void LedControl::setRainLED(bool _set)
  {
  }

  void LedControl::setControlLED(int64_t timeout)
  {
    //
    // Ausschaltzeitpunkt setzen
    //
    // while (controlLedSwitchedOff != 0LL)
    //  vTaskDelay(pdMS_TO_TICKS(50));
    controlLedSwitchedOffDelta = timeout;
  }

  void LedControl::setPumpLed(int64_t _delay)
  {
    pumpLedSwitchedOff = esp_timer_get_time() + _delay;
  }

  void LedControl::setAPModeLED(int64_t timeout)
  {
    //
    // ausschaltzeitpunkt setzen
    //
    // apModeLedSwitchOff = esp_timer_get_time() + timeout;
    /*
    gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_ON);
    gpio_set_level(Prefs::LED_RAIN, Prefs::LED_ON);
    gpio_set_level(Prefs::LED_PUMP, Prefs::LED_ON);
    */
  }

  /**
   * @brief Timer alle 100 ms
   *
   */
  void LedControl::timerCallback(void *)
  {
    //
    // alle 100 ms
    //
    using namespace Prefs;
    static int64_t nextChange{0LL};
    static bool isControlLedOn = false;
    static bool isPumpLedOn = false;
    static bool isRainLedOn = false;
    static bool isApModeOn = false;

    int64_t nowTime{esp_timer_get_time()};
    uint32_t ctrlLEDMask{0U};
    uint32_t ctrlLedValue{0U};

    if (Preferences::isAttentionFlag)
    {
      static bool attentionLEDIsOn = false;
      //
      // Achtungzeichen, LEDS flackern
      //
      if (nowTime > nextChange)
      {
        if (attentionLEDIsOn)
        {
          ctrlLedValue = G_LED_CONTROL_MASK | G_LED_PUMP_MASK;
          nextChange = nowTime + Prefs::BLINK_LED_ATTENTION_OFF;
          attentionLEDIsOn = false;
        }
        else
        {
          ctrlLedValue = G_LED_RAIN_MASK;
          nextChange = nowTime + Prefs::BLINK_LED_ATTENTION_ON;
          attentionLEDIsOn = true;
        }
        ctrlLEDMask = G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
        dedic_gpio_bundle_write(LedControl::ledBundle, ctrlLEDMask, ctrlLedValue);
      }
      return;
    }

    if (Preferences::appOpMode == opMode::APMODE)
    {
      if (nowTime > nextChange)
      {
        if (isApModeOn)
        {
          isApModeOn = false;
          ctrlLedValue = 0;
          dedic_gpio_bundle_write(LedControl::ledBundle, ctrlLEDMask, ctrlLedValue);
          nextChange = nowTime + Prefs::BLINK_LED_CONTROL_AP_OFF;
        }
        else
        {
          isApModeOn = true;
          ctrlLedValue = G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
          dedic_gpio_bundle_write(LedControl::ledBundle, ctrlLEDMask, ctrlLedValue);
          nextChange = nowTime + Prefs::BLINK_LED_CONTROL_AP_ON;
        }
        ctrlLEDMask = G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
        dedic_gpio_bundle_write(LedControl::ledBundle, ctrlLEDMask, ctrlLedValue);
      }
      return;
    }

    //
    // Control LED Flackern...
    //
    if (controlLedSwitchedOffDelta != 0ULL)
    {
      controlLedSwitchedOff = nowTime + controlLedSwitchedOffDelta;
      controlLedSwitchedOffDelta = 0LL;
    }

    if (controlLedSwitchedOff)
    {
      if (nowTime > controlLedSwitchedOff)
      {
        if (isControlLedOn)
        {
          // diese LED bearbeiten, die LED muss wieder aus
          ctrlLEDMask |= G_LED_CONTROL_MASK;
          ctrlLedValue = ctrlLedValue & !G_LED_CONTROL_MASK;
          isControlLedOn = false;
          controlLedSwitchedOff = 0ULL;
        }
      }
      else
      {
        if (!isControlLedOn)
        {
          // diese LED bearbeiten, die LED muss ein
          ctrlLEDMask |= G_LED_CONTROL_MASK;
          ctrlLedValue |= G_LED_CONTROL_MASK;
          isControlLedOn = true;
        }
      }
    }
    else
    {
      if (isControlLedOn)
      {
        // diese LED bearbeiten, die LED muss wieder aus
        ctrlLEDMask |= G_LED_CONTROL_MASK;
        ctrlLedValue = ctrlLedValue & !G_LED_CONTROL_MASK;
        isControlLedOn = false;
        controlLedSwitchedOff = 0ULL;
      }
    }

    //
    // ist die Pumpe aktiv?
    //
    if (Preferences::pumpCycles > 0U || Preferences::pumpAction)
    {
      //
      // ist die LED schon an?
      //
      if (!isPumpLedOn)
      {
        // diese LED maskieren zum setzen
        ctrlLEDMask |= G_LED_PUMP_MASK;
        // und den Wert eintragen
        ctrlLedValue |= G_LED_PUMP_MASK;
        // den Wert setzten wann sie wieder aus soll
        isPumpLedOn = true;
      }
      // die Ausschaltzeit setzen, falls pumpCycles 0 wird
      pumpLedSwitchedOff = nowTime + Prefs::PUMP_LED_DELAY;
      Preferences::pumpAction = false;
    }
    else
    {
      // Pumpenzyklen sind 0
      // läuft die Einschaltzeit noch?
      if (pumpLedSwitchedOff != 0ULL)
      {
        if (isPumpLedOn && (nowTime > pumpLedSwitchedOff))
        {
          // diese LED maskieren zum setzen
          ctrlLEDMask |= G_LED_PUMP_MASK;
          // und den Wert eintragen
          ctrlLedValue &= !G_LED_PUMP_MASK;
          // den Wert setzten wann sie wieder aus soll
          isPumpLedOn = false;
          pumpLedSwitchedOff = 0ULL;
        }
      }
    }

    if (rainLedSwitchedOff != 0uLL)
    {
      if (isRainLedOn)
      {
        // TODO: ausschalten
      }
    }

    //
    // das setzten der LED Bits
    //
    if (ctrlLEDMask > 0U)
    {
      dedic_gpio_bundle_write(LedControl::ledBundle, ctrlLEDMask, ctrlLedValue);
    }
  }

} // namespace
