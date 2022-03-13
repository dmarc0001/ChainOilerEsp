#include "LedControl.hpp"
#include <esp_err.h>

namespace esp32s2
{
  /**
   * @brief instanzieren und initialisieren der statischen variablen
   *
   */
  const char *LedControl::tag{"LedControl"};                //! tag fürs debug logging
  volatile int64_t LedControl::lastChanged{0ULL};           //! letzte Änderung
  volatile int64_t LedControl::pumpLedSwitchedOff{false};   //! ist die Pumpen LED noch an?
  volatile int64_t LedControl::controlLedSwitchedOff{0ULL}; // wann soll die Control LED wieder aus?
  volatile int64_t LedControl::apModeLedSwitchOff{0ULL};    // wan soll ap-mode ausgeschakltet werden?
  esp_timer_handle_t LedControl::timerHandle{nullptr};      //! timer handle

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
#ifdef DEBUG
    LedControl::initWroverLED();
#endif
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
    gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_OFF);
    gpio_set_level(Prefs::LED_RAIN, Prefs::LED_OFF);
    gpio_set_level(Prefs::LED_PUMP, Prefs::LED_OFF);
  }

  /**
   * @brief blinken mit allen LED (z.B. beim Booten)
   *
   */
  void LedControl::showAttention()
  {
    static bool attentionLEDIsOn = false;
    int64_t timeDiff = esp_timer_get_time() - LedControl::lastChanged;
    //
    if (attentionLEDIsOn && timeDiff > Prefs::BLINK_LED_ATTENTION_ON)
    {
      attentionLEDIsOn = false;
      gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_OFF);
      gpio_set_level(Prefs::LED_RAIN, Prefs::LED_ON);
      gpio_set_level(Prefs::LED_PUMP, Prefs::LED_ON);
      LedControl::lastChanged = esp_timer_get_time();
    }
    else if (!attentionLEDIsOn && timeDiff > Prefs::BLINK_LED_ATTENTION_OFF)
    {
      attentionLEDIsOn = true;
      gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_ON);
      gpio_set_level(Prefs::LED_RAIN, Prefs::LED_OFF);
      gpio_set_level(Prefs::LED_PUMP, Prefs::LED_OFF);
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
  // void LedControl::setPumpLED(bool _set)
  // {
  //   if (_set)
  //   {
  //     // ausschaltzeit setzten und einschalten
  //     pumpLedSwitchedOff = esp_timer_get_time() + Prefs::PUMP_LED_DELAY;
  //     gpio_set_level(Prefs::LED_PUMP, 1);
  //   }
  //   else
  //   {
  //     // deaktivieren
  //     pumpLedSwitchedOff = 0ULL;
  //     gpio_set_level(Prefs::LED_PUMP, 0);
  //   }
  // }

  void LedControl::setControlLED(int64_t timeout)
  {
    //
    // Ausschaltzeitpunkt setzen
    //
    controlLedSwitchedOff = esp_timer_get_time() + timeout;
    // LED einschalten
    gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_ON);
  }

  void LedControl::setAPModeLED(int64_t timeout)
  {
    //
    // ausschaltzeitpunkt setzen
    //
    apModeLedSwitchOff = esp_timer_get_time() + timeout;
    gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_ON);
    gpio_set_level(Prefs::LED_RAIN, Prefs::LED_ON);
    gpio_set_level(Prefs::LED_PUMP, Prefs::LED_ON);
  }

  /**
   * @brief Callback für den Timer (100 ms)
   *
   */
  void LedControl::timerCallback(void *)
  {
    using namespace Prefs;
    static bool isPumpLedOn{false};
    int64_t nowTime = esp_timer_get_time();

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
        gpio_set_level(Prefs::LED_CONTROL, Prefs::LED_OFF);
        controlLedSwitchedOff = 0ULL;
      }
    }

    //
    // ist die Pumpe in action?
    //
    if (Preferences::pumpCycles > 0U)
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
        gpio_set_level(Prefs::LED_PUMP, Prefs::LED_ON);
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
          gpio_set_level(Prefs::LED_PUMP, Prefs::LED_OFF);
          // zustand vermerken
          isPumpLedOn = false;
          pumpLedSwitchedOff = 0ULL;
        }
      }
    }
  }

  void LedControl::initWroverLED()
  {
    uint32_t red = 255;
    uint32_t green = 255;
    uint32_t blue = 0;

    ESP_LOGE(tag, "init WS2812 driver...");
    rmt_config_t config =
        {
            .rmt_mode = RMT_MODE_TX,
            .channel = RMT_TX_CHANNEL,
            .gpio_num = GPIO_NUM_18,
            .clk_div = 2, // set counter clock to 40MHz was 40
            .mem_block_num = 1,
            .flags = 0,
            .tx_config =
                {
                    .carrier_freq_hz = 38000,
                    .carrier_level = RMT_CARRIER_LEVEL_HIGH,
                    .idle_level = RMT_IDLE_LEVEL_LOW,
                    .carrier_duty_percent = 33,
                    .loop_count = 2,
                    .carrier_en = false,
                    .loop_en = false,
                    .idle_output_en = true,
                }};

    // set counter clock to 40MHz
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));

    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(NUM_OF_LED_PER_STRIP, (led_strip_dev_t)config.channel);
    led_strip_t *strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip)
    {
      ESP_LOGE(tag, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    for (auto i = 0; i < 25; ++i)
    {
      switch (i & 3)
      {
      case 0:
        red = 255;
        green = 0;
        blue = 0;
        break;
      case 1:
        red = 0;
        green = 255;
        blue = 0;
        break;
      case 2:
        red = 0;
        green = 0;
        blue = 255;
        break;
      default:
        red = 80;
        green = 80;
        blue = 80;
      }
      for (int j = 0; j < NUM_OF_LED_PER_STRIP; ++j)
      {
        // Write RGB values to strip driver
        ESP_ERROR_CHECK(strip->set_pixel(strip, j, red, green, blue));
      }
      // Flush RGB values to LEDs
      ESP_ERROR_CHECK(strip->refresh(strip, 100));
      vTaskDelay(pdMS_TO_TICKS(100));
    }
    strip->clear(strip, 50);
  }

} // namespace
