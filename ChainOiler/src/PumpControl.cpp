#include "PumpControl.hpp"
#include "esp_log.h"
#include <esp_err.h>

namespace esp32s2
{
  /**
   * @brief instanziere und initialisiere statische variable
   * 
   */
  const char *PumpControl::tag{"PumpControl"};
  esp_timer_handle_t PumpControl::timerHandle{nullptr}; //! timer handle

  /**
   * @brief initialisiere die Hardware für die Pumpe
   * 
   */
  void PumpControl::init()
  {
    using namespace Prefs;
    //
    // GPIO Konfigurieren
    //
    ESP_LOGI(tag, "init pump hardware...");
    //
    // Ausgabesignale Digital
    //
    gpio_config_t config_out = {.pin_bit_mask = BIT64(Prefs::OUTPUT_PUMP_CONTROL),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_out);
    //
    PumpControl::startTimer();
    ESP_LOGD(tag, "init pump hardware...OK");
  }

  void PumpControl::startTimer()
  {
    //
    // timer für Punpe starten
    //
    const esp_timer_create_args_t appTimerArgs =
        {
            .callback = &PumpControl::timerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "led_timer"};
    //
    // timer erzeugen
    //
    ESP_ERROR_CHECK(esp_timer_create(&appTimerArgs, &PumpControl::timerHandle));
    //
    // timer starten, microsekunden ( 20 ms soll es)
    //
    ESP_ERROR_CHECK(esp_timer_start_periodic(PumpControl::timerHandle, 20000));
    //
  }

  void PumpControl::timerCallback(void *)
  {
    // TODO: pumpe max 20 ms on
    }
}
