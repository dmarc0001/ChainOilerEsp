#include "HardwareUtil.hpp"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "LedControl.hpp"
#include <cmath>

namespace esp32s2
{
  xQueueHandle EspCtrl::pathLenQueue = nullptr; //! handle fuer queue
  xQueueHandle EspCtrl::speedQueue = nullptr;   //! handle fuer queue
  const uint8_t EspCtrl::isr_control{1};        //! Marker für die ISR
  const uint8_t EspCtrl::isr_rain{2};           //! Marker für die ISR

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
      printf("TODO: TACHO WAKEUP restore counters from sram...");
    }
    else
    {
      //
      // Kompletter Neustart
      //
      printf("TODO: POWER_ON_WAKEUP restore counters from NVS...");
    }
    EspCtrl::initGPIOPorts();
    EspCtrl::initTachoPulseCounters();
    EspCtrl::initADC();
  }

  /**
 * @brief initiiere GPIO Ports
 * 
 * @return true 
 * @return false 
 */
  bool EspCtrl::initGPIOPorts()
  {
    using namespace Prefs;
    //
    // GPIO Konfigurieren
    //
    ESP_LOGD(tag, "%s: init GPIO...", __func__);
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
    //  Tacho und Knopf (Knopf-GPIO_INTR_ANYEDGE)
    //
    /*gpio_config_t config_in = {.pin_bit_mask = BIT64(INPUT_TACHO) | BIT64(INPUT_FUNCTION_SWITCH),*/
    gpio_config_t config_in = {.pin_bit_mask = BIT64(INPUT_FUNCTION_SWITCH) | BIT64(INPUT_TACHO) | BIT64(INPUT_RAIN_SWITCH_OPTIONAL),
                               .mode = GPIO_MODE_INPUT,
                               .pull_up_en = GPIO_PULLUP_ENABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_in);
    //
    // ISR Servive installieren
    //
    gpio_install_isr_service(0);
    //
    // Handler für die beiden Ports
    //
    gpio_isr_handler_add(INPUT_FUNCTION_SWITCH, EspCtrl::buttonIsr, (void *)&EspCtrl::isr_control);
    gpio_isr_handler_add(INPUT_RAIN_SWITCH_OPTIONAL, EspCtrl::buttonIsr, (void *)&EspCtrl::isr_rain);
    //
    // Interrupt für zwei PINS einschalten
    //
    gpio_set_intr_type(INPUT_FUNCTION_SWITCH, GPIO_INTR_ANYEDGE);
    gpio_set_intr_type(INPUT_RAIN_SWITCH_OPTIONAL, GPIO_INTR_ANYEDGE);
    return true;
  }

  bool EspCtrl::initADC()
  {
    using namespace Prefs;
    //
    // Analog-Digital-Wandler starten
    // TODO: Messbereich optimieren
    // Setzte Auflösung auf 13 Bit
    // TODO: 0.1 uF an ADC
    //
    ESP_LOGD(tag, "%s: init ADC...", __func__);
    adc1_config_width(ADC_WIDTH_BIT_13);
    //
    // Dämpfung für Meßbereich einstellen
    //
    adc1_config_channel_atten(INPUT_ADC_RAIN_00, ADC_ATTEN_DB_11 /*ADC_ATTEN_DB_0*/);
    adc1_config_channel_atten(INPUT_ADC_RAIN_01, ADC_ATTEN_DB_11 /*ADC_ATTEN_DB_0*/);
    ESP_LOGD(tag, "%s: init ADC...done", __func__);
    return true;
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
  bool EspCtrl::initTachoPulseCounters()
  {
    using namespace Prefs;

    pcnt_unit_t unit0 = PCNT_UNIT_0;
    pcnt_unit_t unit1 = PCNT_UNIT_1;
    int16_t pulsesFor100Meters = Preferences::getPulsesFor100Meters();
    int16_t pulsesFor10Meters = Preferences::getPulsesFor10Meters();
    //
    // Queue initialisieren
    //
    pathLenQueue = xQueueCreate(QUEUE_LEN_DISTANCE, sizeof(pcnt_evt_t));
    ESP_LOGD(tag, "%s: init pulse counter unit0, channel 0...", __func__);
    speedQueue = xQueueCreate(QUEUE_LEN_TACHO, sizeof(deltaTimeTenMeters_us));
    ESP_LOGD(tag, "%s: init pulse counter unit1, channel 0...", __func__);
    //
    // Pulszähler Wegstrecke initialisieren
    //
    pcnt_config_t pcnt_config_w = {
        .pulse_gpio_num = INPUT_TACHO,       /* der eingangspin */
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,  /* kein Kontrolleingang */
        .lctrl_mode = PCNT_MODE_KEEP,        /* Zählrichtung bei CTRL 0 (ignoriert, da ctrl -1) */
        .hctrl_mode = PCNT_MODE_KEEP,        /* Zählrichting bei CTRL HIGH (ignoriert da ctrl -1) */
        .pos_mode = PCNT_COUNT_INC,          /* bei positiver Flanke zählen */
        .neg_mode = PCNT_COUNT_DIS,          /* Zähler bei negativer flanke lassen */
        .counter_h_lim = pulsesFor100Meters, /* 100 Meter Wert, dann rücksetzen */
        .counter_l_lim = 0,                  /* beim rückwärtszählen (aber hier nur positiver zähler) */
        .unit = unit0,                       /* unti 0 zum messen der wegstrecke */
        .channel = PCNT_CHANNEL_0,           /* Kanal 0 zum zählen */
    };
    //
    // initialisieren der unit
    //
    ESP_ERROR_CHECK(pcnt_unit_config(&pcnt_config_w));
    ESP_LOGD(tag, "%s: init pulse counter unit0, channel 0...done", __func__);
    //
    // Pulszähler Tacho initialisieren
    //
    pcnt_config_t pcnt_config_s = {
        .pulse_gpio_num = INPUT_TACHO,      /* der eingangspin, deselbe wir unit 0 */
        .ctrl_gpio_num = PCNT_PIN_NOT_USED, /* kein Kontrolleingang */
        .lctrl_mode = PCNT_MODE_KEEP,       /* Zählrichtung bei CTRL 0 (ignoriert, da ctrl -1) */
        .hctrl_mode = PCNT_MODE_KEEP,       /* Zählrichting bei CTRL HIGH (ignoriert da ctrl -1) */
        .pos_mode = PCNT_COUNT_INC,         /* bei positiver Flanke zählen */
        .neg_mode = PCNT_COUNT_DIS,         /* Zähler bei negativer flanke lassen */
        .counter_h_lim = pulsesFor10Meters, /* 10 Meter Wert, dann rücksetzen */
        .counter_l_lim = 0,                 /* beim rückwärtszählen (aber hier nur positiver zähler) */
        .unit = unit1,                      /* unti 1 zum messen der wegstrecke */
        .channel = PCNT_CHANNEL_0,          /* Kanal 0 zum zählen */
    };
    //
    // initialisieren der unit
    //
    ESP_ERROR_CHECK(pcnt_unit_config(&pcnt_config_s));
    ESP_LOGD(tag, "%s: init pulse counter unit1, channel 0...done", __func__);
    //
    // Configure and enable the input filter
    // 1800 zyklen sind bei 80 mhz 22 us
    // also wird alles ignoriert was kürzer ist
    //
    ESP_LOGD(tag, "%s: init unit0 pulse filter: filter value: %d, PPR: %.2f...", __func__, Preferences::getMinimalPulseLength(), Preferences::getPulsePerRound());
    pcnt_set_filter_value(unit0, Preferences::getMinimalPulseLength());
    //pcnt_set_filter_value(unit0, 1023);
    pcnt_filter_enable(unit0);
    //
    ESP_LOGD(tag, "%s: init unbi1 pulse filter: filter value: %d, PPR: %.2f...", __func__, Preferences::getMinimalPulseLength(), Preferences::getPulsePerRound());
    pcnt_set_filter_value(unit1, Preferences::getMinimalPulseLength());
    pcnt_filter_enable(unit1);
    //
    // Wert für event (ISR) setzten, wert erreicht, ISR rufen
    //
    pcnt_set_event_value(unit0, PCNT_EVT_THRES_0, pulsesFor100Meters);
    pcnt_event_enable(unit0, PCNT_EVT_THRES_0);
    pcnt_set_event_value(unit1, PCNT_EVT_THRES_0, pulsesFor10Meters);
    pcnt_event_enable(unit1, PCNT_EVT_THRES_0);
    //
    // event freigeben
    //
    pcnt_event_enable(unit0, PCNT_EVT_H_LIM);
    pcnt_event_enable(unit1, PCNT_EVT_H_LIM);
    //
    // Counter initialisieren
    //
    pcnt_counter_pause(unit0);
    pcnt_counter_clear(unit0);
    pcnt_counter_pause(unit1);
    pcnt_counter_clear(unit1);
    //
    // ISR installieren und Callback aktivieren
    //
    pcnt_isr_service_install(0);
    pcnt_isr_handler_add(unit0, EspCtrl::tachoOilerCountISR, nullptr);
    pcnt_isr_handler_add(unit1, EspCtrl::speedCountISR, nullptr);

    //
    // alles ist initialisiert, starte die Counter
    //
    pcnt_counter_resume(unit0);
    pcnt_counter_resume(unit1);
    ESP_LOGD(tag, "%s: init pulse counter unit0/unit1...done", __func__);
    return true;
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
  * @brief Messe die gefahrene Entfernung
  * 
  */
  void IRAM_ATTR EspCtrl::tachoOilerCountISR(void *)
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
    if (uxQueueMessagesWaiting(EspCtrl::pathLenQueue) < Prefs::QUEUE_LEN_DISTANCE)
    {
      xQueueSendFromISR(EspCtrl::pathLenQueue, &evt, &task_awoken);
      if (task_awoken == pdTRUE)
      {
        portYIELD_FROM_ISR();
      }
    }
  }

  /**
  * @brief Messe Zeit für 10 Meter
  * 
  */
  void IRAM_ATTR EspCtrl::speedCountISR(void *)
  {
    uint64_t currentTimeStamp = esp_timer_get_time();
    int task_awoken = pdFALSE;
    //
    // die Daten in die Queue speichern
    //
    if (uxQueueMessagesWaiting(EspCtrl::speedQueue) < Prefs::QUEUE_LEN_TACHO)
    {
      xQueueSendFromISR(EspCtrl::speedQueue, &currentTimeStamp, &task_awoken);
      if (task_awoken == pdTRUE)
      {
        portYIELD_FROM_ISR();
      }
    }
  }

  void IRAM_ATTR EspCtrl::buttonIsr(void *arg)
  {
    using namespace Prefs;

    uint8_t *_num = static_cast<uint8_t *>(arg);
    int level;

    switch (*_num)
    {
    case 1:
      level = gpio_get_level(Prefs::INPUT_FUNCTION_SWITCH);
      //
      // Was ist passiert? Level 0 bedeutet Knopf gedrückt
      //
      AppStati::functionSwitchDown = (level == 1) ? false : true;
      AppStati::lastFunctionSwitchAction = esp_timer_get_time();
      AppStati::functionSwitchAction = true;
      break;

    case 2:
      level = gpio_get_level(Prefs::INPUT_RAIN_SWITCH_OPTIONAL);
      //
      // Was ist passiert? Level 0 bedeutet Knopf gedrückt
      //
      AppStati::rainSwitchDown = (level == 1) ? false : true;
      AppStati::lastRainSwitchAction = esp_timer_get_time();
      AppStati::rainSwitchAction = true;
      break;
    default:
      break;
    }
  }

} // namespace esp32s2
