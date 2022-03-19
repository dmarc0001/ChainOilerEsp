#include "TachoControl.hpp"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include <cmath>
#include <esp_log.h>

namespace esp32s2
{
  /**
   * @brief instanziere und initialisiere die statischen Variablen
   *
   */
  xQueueHandle TachoControl::pathLenQueue = nullptr; //! handle fuer queue
  xQueueHandle TachoControl::speedQueue = nullptr;   //! handle fuer queue
  const char *TachoControl::tag{"TachoCtrl"};        //! tag fürs debug logging

  /**
   * Initialisieere die Hardware
   */
  void TachoControl::init()
  {
    using namespace Prefs;
    //
    ESP_LOGI(tag, "init hardware...");
    //
    // Queue initialisieren
    //
    pathLenQueue = xQueueCreate(QUEUE_LEN_DISTANCE, sizeof(pathLenMeters));
    speedQueue = xQueueCreate(QUEUE_LEN_TACHO, sizeof(deltaTimeTenMeters_us));
    //
    // Tacho initialisieren
    //
    ESP_LOGD(tag, "init tacho input...");
    //
    //  Tacho  Eingang
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
    pcnt_unit_t unit0 = PCNT_UNIT_0;
    ESP_LOGD(tag, "init pulse counter unit0, channel 0...");
    int16_t pulsesFor25Meters = Preferences::getPulsesFor25Meters();
    //
    // Pulszähler Wegstrecke initialisieren
    //
    pcnt_config_t pcnt_config_w = {
        .pulse_gpio_num = INPUT_TACHO,      /* der eingangspin */
        .ctrl_gpio_num = PCNT_PIN_NOT_USED, /* kein Kontrolleingang */
        .lctrl_mode = PCNT_MODE_KEEP,       /* Zählrichtung bei CTRL 0 (ignoriert, da ctrl -1) */
        .hctrl_mode = PCNT_MODE_KEEP,       /* Zählrichting bei CTRL HIGH (ignoriert da ctrl -1) */
        .pos_mode = PCNT_COUNT_INC,         /* bei positiver Flanke zählen */
        .neg_mode = PCNT_COUNT_DIS,         /* Zähler bei negativer flanke lassen */
        .counter_h_lim = pulsesFor25Meters, /* 25 Meter Wert, dann rücksetzen */
        .counter_l_lim = 0,                 /* beim rückwärtszählen (aber hier nur positiver zähler) */
        .unit = unit0,                      /* unti 0 zum messen der wegstrecke */
        .channel = PCNT_CHANNEL_0,          /* Kanal 0 zum zählen */
    };
    //
    // initialisieren der unit
    //
    ESP_ERROR_CHECK(pcnt_unit_config(&pcnt_config_w));
    ESP_LOGD(tag, "init pulse counter unit0, channel 0...done");
    // Configure and enable the input filter
    // 1800 zyklen sind bei 80 mhz 22 us
    // also wird alles ignoriert was kürzer ist
    //
    ESP_LOGD(tag, "init unit0 pulse filter: filter value: %d, PPR: %.2f...", Preferences::getMinimalPulseLength(), Preferences::getPulsePerRound());
    pcnt_set_filter_value(unit0, Preferences::getMinimalPulseLength());
    pcnt_filter_enable(unit0);
    //
    // Wert für event (ISR) setzten, wert erreicht, ISR rufen
    //
    pcnt_set_event_value(unit0, PCNT_EVT_THRES_0, pulsesFor25Meters);
    pcnt_event_enable(unit0, PCNT_EVT_THRES_0);
    //
    // event freigeben
    //
    pcnt_event_enable(unit0, PCNT_EVT_H_LIM);
    //
    // Counter initialisieren
    //
    pcnt_counter_pause(unit0);
    pcnt_counter_clear(unit0);
    //
    // ISR installieren und Callback aktivieren
    //
    pcnt_isr_service_install(ESP_INTR_FLAG_IRAM);
    pcnt_isr_handler_add(unit0, TachoControl::tachoOilerCountISR, nullptr);
    //
    // alles ist initialisiert, starte die Counter
    //
    pcnt_counter_resume(unit0);
    ESP_LOGD(tag, "%s: init pulse counter unit0...done", __func__);
  }

  /**
   * @brief Benachrichtige bei gewünschter Entfernung (hier 100 Meter)
   *
   */
  void IRAM_ATTR TachoControl::tachoOilerCountISR(void *)
  {
    static uint32_t counter{0};
    static uint32_t path_len = Prefs::PATH_LEN_METERS_PER_ISR * 4U;
    uint64_t currentTimeStamp{0};
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
    // die speed Daten in die Queue speichern
    //
    if (uxQueueMessagesWaiting(TachoControl::speedQueue) < Prefs::QUEUE_LEN_TACHO)
    {
      currentTimeStamp = esp_timer_get_time();
      xQueueSendToBackFromISR(TachoControl::speedQueue, &currentTimeStamp, &task_awoken);
      if (task_awoken == pdTRUE)
      {
        portYIELD_FROM_ISR();
      }
    }
    if ((counter & 0x03U) == 0x03U)
    {
      //
      // alle 100 Meter, weil interrupt alle 25 Meter
      // die Entfernungdsdaten in die Queue speichern
      //
      // pcnt_get_event_value(PCNT_UNIT_0, PCNT_EVT_THRES_0, &evt.value);
      if (uxQueueMessagesWaiting(TachoControl::pathLenQueue) < Prefs::QUEUE_LEN_DISTANCE)
      {
        // xQueueSendToBackFromISR(TachoControl::pathLenQueue, &path_len, &task_awoken);
        xQueueSendToBackFromISR(TachoControl::pathLenQueue, &path_len, &task_awoken);
        if (task_awoken == pdTRUE)
        {
          portYIELD_FROM_ISR();
        }
      }
    }
    ++counter;
  }

  /**
   * @brief Tacho timer INT stoppen
   *
   */
  void TachoControl::pause()
  {
    pcnt_unit_t unit0 = PCNT_UNIT_0;
    pcnt_counter_pause(unit0);
    pcnt_counter_clear(unit0);
  }

  /**
   * @brief
   *
   */
  void TachoControl::resume()
  {
    pcnt_unit_t unit0 = PCNT_UNIT_0;
    pcnt_counter_resume(unit0);
  }

} // namespace esp32s2
