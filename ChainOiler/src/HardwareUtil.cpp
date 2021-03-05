#include "HardwareUtil.hpp"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include <cmath>

namespace esp32s2
{
  xQueueHandle EspCtrl::pcnt_evt_queue = nullptr;                            //! handle fuer queue
  const char *EspCtrl::tag{"EspCtrl"};                                       //! tag fürs debug logging
  esp_sleep_wakeup_cause_t EspCtrl::wakeupCause{ESP_SLEEP_WAKEUP_UNDEFINED}; //! der Grund des Erwachens

  /**
   * Initialisieere die Hardware
   */
  void EspCtrl::init()
  {
    using namespace Prefs;
    //
    ESP_LOGD(tag, "%s: init hardware...", __func__);
    //
    // warum geweckt/resettet
    //
    EspCtrl::wakeupCause = esp_sleep_get_wakeup_cause();
#ifdef DEBUG
    switch (EspCtrl::wakeupCause)
    {
    case ESP_SLEEP_WAKEUP_EXT0:
      printf("wakeup source is external RTC_IO signal\n");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      printf("wakeup source is external RTC_CNTL signal\n");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      printf("wakeup ist timer\n");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      printf("wakeup ist touch sensor\n");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      printf("wakeup is ULP processor\n");
      break;
    default:
      printf("wakeup is not defined, number is %d\n", EspCtrl::wakeupCause);
      break;
    }
#endif
    if (EspCtrl::wakeupCause == ESP_SLEEP_WAKEUP_EXT0)
    {
      //
      // Der Tachoimpuls hat geweckt
      //
      printf("TODO: restore counters from sram...");
    }
    else
    {
      //
      // Kompletter Neustart
      //
      printf("TODO: restore counters from NVS...");
    }
    //
    // GPIO Konfigurieren
    //
    ESP_LOGD(tag, "%s: init GPIO...", __func__);
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
    // Ausgabesignale Digital
    //
    gpio_config_t config_out = {.pin_bit_mask = BIT64(OUTPUT_PUMP_CONTROL) | BIT64(OUTPUT_RAIN_SW_01) | BIT64(OUTPUT_RAIN_SW_02),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_out);
    //
    //  Tacho und Knopf
    //
    /*gpio_config_t config_in = {.pin_bit_mask = BIT64(INPUT_TACHO) | BIT64(INPUT_FUNCTION_SWITCH),*/
    gpio_config_t config_in = {.pin_bit_mask = BIT64(INPUT_FUNCTION_SWITCH),
                               .mode = GPIO_MODE_INPUT,
                               .pull_up_en = GPIO_PULLUP_DISABLE,
                               .pull_down_en = GPIO_PULLDOWN_ENABLE,
                               .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_in);
    //
    // Analog-Digital-Wandler starten
    // TODO: Messbereich optimieren
    // Setzte Auflösung auf 13 Bit
    // TODO: 0.1 uF an ADC
    //
    ESP_LOGD(tag, "%s: init ADC...", __func__);
    adc1_config_width(ADC_WIDTH_BIT_13);
    adc1_config_channel_atten(INPUT_ADC_RAIN_00, ADC_ATTEN_DB_0);
    adc1_config_channel_atten(INPUT_ADC_RAIN_01, ADC_ATTEN_DB_0);
    ESP_LOGD(tag, "%s: init ADC...done", __func__);
    EspCtrl::initTachoPulseCounter();
  }

  /**
   * Regensensor wert zurück geben
   */
  rain_value_t EspCtrl::getRainValues()
  {
    using namespace Prefs;

    rain_value_t val;
    val.first = adc1_get_raw(INPUT_ADC_RAIN_00);
    val.second = adc1_get_raw(INPUT_ADC_RAIN_01);
    return (val);
  }

  /**
   * gehe in den Tiefen Schlaf
   */
  void EspCtrl::goDeepSleep()
  {
    esp_sleep_enable_ext0_wakeup(Prefs::INPUT_TACHO, 1); // 1 == HIGH
    ESP_LOGD(tag, "%s: controller is going to deep sleep...", __func__);
    printf("deep sleep...");
    Prefs::Preferences::close();
    for (int i = 5; i > 0; --i)
    {
      ESP_LOGD(tag, "%s: sleep in %02d secounds...", __func__, i);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    //
    // TODO: alle LED aus?
    //
    printf("..Good night.\n");
    //
    // Wiederbelebung erst durch Tachoimpuls
    //
    esp_deep_sleep_start();
  }

  /**
   * initialisiert den Counter für Eingangssignale für den Tachoimpuls
   */
  bool EspCtrl::initTachoPulseCounter()
  {
    using namespace Prefs;

    pcnt_unit_t unit0 = PCNT_UNIT_0;
    int16_t pulsesFor100Meters = Preferences::getPulsesFor100Meters();

    ESP_LOGD(tag, "%s: init pulse counter unit0...", __func__);
    //
    // Queue initialisieren
    //
    pcnt_evt_queue = xQueueCreate(100, sizeof(pcnt_evt_t));
    //
    // Pulszähler initialisieren
    //
    pcnt_config_t pcnt_config = {
        .pulse_gpio_num = INPUT_TACHO,       /* der eingangspin */
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,  /* kein Kontrolleingang */
        .lctrl_mode = PCNT_MODE_KEEP,        /* Zählrichtung bei CTRL 0 (ignoriert, da ctrl -1) */
        .hctrl_mode = PCNT_MODE_KEEP,        /* Zählrichting bei CTRL HIGH (ignoriert da ctrl -1) */
        .pos_mode = PCNT_COUNT_INC,          /* bei positiver Flanke zählen */
        .neg_mode = PCNT_COUNT_DIS,          /* Zähler bei negativer Betriebsart lassen */
        .counter_h_lim = pulsesFor100Meters, /* 100 Meter Wert, dann rücksetzen */
        .counter_l_lim = 0,                  /* beim rückwärtszählen (aber hier nur positiver zähler) */
        .unit = unit0,                       /* unti 0 zum messen der wegstrecke */
        .channel = PCNT_CHANNEL_0,           /* Kanal 0 zum zählen */
    };
    //
    // initialisieren der unit
    //
    pcnt_unit_config(&pcnt_config);
    //
    // Configure and enable the input filter
    // 1800 zyklen sind bei 80 mhz 22 us
    // also wird alles ignoriert was kürzer ist
    //
    ESP_LOGD(tag, "%s: init pulse filter: filter value: %d, PPR: %.2f...", __func__, Preferences::getMinimalPulseLength(), Preferences::getPulsePerRound());
    pcnt_set_filter_value(unit0, Preferences::getMinimalPulseLength());
    //pcnt_set_filter_value(unit0, 1023);
    pcnt_filter_enable(unit0);
    //
    // Wert für event (ISR) setzten, wert erreicht, ISR rufen
    //
    pcnt_set_event_value(unit0, PCNT_EVT_THRES_0, pulsesFor100Meters);
    pcnt_event_enable(unit0, PCNT_EVT_THRES_0);
    //pcnt_set_event_value(unit0, PCNT_EVT_THRES_1, pulsesFor100Meters);
    //pcnt_event_enable(unit0, PCNT_EVT_THRES_1);
    //
    // event freigeben
    //
    //pcnt_event_enable(unit0, PCNT_EVT_ZERO);
    pcnt_event_enable(unit0, PCNT_EVT_H_LIM);
    //pcnt_event_enable(unit0, PCNT_EVT_L_LIM);
    //
    // Counter initialisieren
    //
    pcnt_counter_pause(unit0);
    pcnt_counter_clear(unit0);
    //
    // ISR installieren und Callback aktivieren
    //
    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(unit0, tachoOilerCountISR, nullptr);

    /* Everything is set up, now go to counting */
    pcnt_counter_resume(unit0);
    ESP_LOGD(tag, "%s: init pulse counter unit0...done", __func__);
    return true;
  }

  /* Decode what PCNT's unit originated an interrupt
 * and pass this information together with the event type
 * the main program using a queue.
 */
  void IRAM_ATTR tachoOilerCountISR(void *)
  {
    pcnt_evt_t evt;
    evt.unit = PCNT_UNIT_0;
    int task_awoken = pdFALSE;
    /*
    PCNT_EVT_THRES_1 = BIT(2),           //!< PCNT watch point event: threshold1 value event 
    PCNT_EVT_THRES_0 = BIT(3),           //!< PCNT watch point event: threshold0 value event 
    PCNT_EVT_L_LIM = BIT(4),             //!< PCNT watch point event: Minimum counter value
    PCNT_EVT_H_LIM = BIT(5),             //!< PCNT watch point event: Maximum counter value 
    PCNT_EVT_ZERO = BIT(6),              //!< PCNT watch point event: counter value zero event
    PCNT_EVT_MAX
    */
    //
    // die Daten in die Queue speichern
    //
    pcnt_get_event_value(PCNT_UNIT_0, PCNT_EVT_THRES_0, &evt.value);
    //if (uxQueueMessagesWaiting(EspCtrl::pcnt_evt_queue) < 90)
    //{
    xQueueSendFromISR(EspCtrl::pcnt_evt_queue, &evt, &task_awoken);
    if (task_awoken == pdTRUE)
    {
      portYIELD_FROM_ISR();
    }
    //}
  }
} // namespace esp32s2
