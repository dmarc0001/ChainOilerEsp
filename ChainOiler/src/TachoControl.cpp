#include "TachoControl.hpp"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "LedControl.hpp"
#include <cmath>

namespace esp32s2
{
  xQueueHandle TachoControl::pathLenQueue = nullptr;                              //! handle fuer queue
  xQueueHandle TachoControl::speedQueue = nullptr;                                //! handle fuer queue
  const char *TachoControl::tag{"EspCtrl"};                                       //! tag fürs debug logging
  esp_sleep_wakeup_cause_t TachoControl::wakeupCause{ESP_SLEEP_WAKEUP_UNDEFINED}; //! der Grund des Erwachens

  /**
   * Initialisieere die Hardware
   */
  void TachoControl::init()
  {
    using namespace Prefs;
    //
    ESP_LOGD(tag, "%s: init hardware...", __func__);
    //
    // warum geweckt/resettet
    //
    TachoControl::processStartupCause();

    //
    // Tacho initialisieren
    //
    ESP_LOGD(tag, "%s: init GPIO...", __func__);
    //
    //  Tacho und Knopf (Knopf-GPIO_INTR_ANYEDGE)
    //
    gpio_config_t config_in = {.pin_bit_mask = BIT64(INPUT_TACHO),
                               .mode = GPIO_MODE_INPUT,
                               .pull_up_en = GPIO_PULLUP_ENABLE,
                               .pull_down_en = GPIO_PULLDOWN_DISABLE,
                               .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_in);
    //
    // ISR Servive installieren
    // verschoben in main
    // gpio_install_isr_service(0);
    //
    // Handler für die beiden Ports
    //
    // gpio_isr_handler_add(INPUT_FUNCTION_SWITCH, EspCtrl::buttonIsr, (void *)&EspCtrl::isr_control);
    // gpio_isr_handler_add(INPUT_RAIN_SWITCH_OPTIONAL, EspCtrl::buttonIsr, (void *)&EspCtrl::isr_rain);
    //
    // Interrupt für zwei PINS einschalten
    //
    // gpio_set_intr_type(INPUT_FUNCTION_SWITCH, GPIO_INTR_ANYEDGE);
    // gpio_set_intr_type(INPUT_RAIN_SWITCH_OPTIONAL, GPIO_INTR_ANYEDGE);
    //
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
    pcnt_isr_handler_add(unit0, TachoControl::tachoOilerCountISR, nullptr);
    pcnt_isr_handler_add(unit1, TachoControl::speedCountISR, nullptr);

    //
    // alles ist initialisiert, starte die Counter
    //
    pcnt_counter_resume(unit0);
    pcnt_counter_resume(unit1);
    ESP_LOGD(tag, "%s: init pulse counter unit0/unit1...done", __func__);
  }

  void TachoControl::processStartupCause()
  {
    TachoControl::wakeupCause = esp_sleep_get_wakeup_cause();
#ifdef DEBUG
    switch (TachoControl::wakeupCause)
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
      printf("wakeup is not defined, number is %d\n", TachoControl::wakeupCause);
      break;
    }
#endif
    if (TachoControl::wakeupCause == ESP_SLEEP_WAKEUP_EXT0)
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
  }

  /**
  * @brief Messe die gefahrene Entfernung
  * 
  */
  void IRAM_ATTR TachoControl::tachoOilerCountISR(void *)
  {
    pcnt_evt_t evt;
    evt.unit = PCNT_UNIT_0;
    evt.meters = 100;
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
    if (uxQueueMessagesWaiting(TachoControl::pathLenQueue) < Prefs::QUEUE_LEN_DISTANCE)
    {
      xQueueSendFromISR(TachoControl::pathLenQueue, &evt, &task_awoken);
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
  void IRAM_ATTR TachoControl::speedCountISR(void *)
  {
    uint64_t currentTimeStamp = esp_timer_get_time();
    int task_awoken = pdFALSE;
    //
    // die Daten in die Queue speichern
    //
    if (uxQueueMessagesWaiting(TachoControl::speedQueue) < Prefs::QUEUE_LEN_TACHO)
    {
      xQueueSendFromISR(TachoControl::speedQueue, &currentTimeStamp, &task_awoken);
      if (task_awoken == pdTRUE)
      {
        portYIELD_FROM_ISR();
      }
    }
  }

} // namespace esp32s2