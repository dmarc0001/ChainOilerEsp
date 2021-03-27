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
  bool PumpControl::pumpIsOn{false};

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
    PumpControl::start();
    ESP_LOGD(tag, "init pump hardware...OK");
  }

  /**
   * @brief stoppe den Timer, lösche das handle
   * 
   */
  void PumpControl::stop()
  {
    esp_timer_stop(&PumpControl::timerHandle);
    PumpControl::timerHandle = nullptr;
    gpio_set_level(Prefs::OUTPUT_PUMP_CONTROL, 0);
    PumpControl::pumpIsOn = false;
  }

  /**
   * @brief starte den Timer für die Pumpe
   * 
   */
  void PumpControl::start()
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

  /**
   * @brief timer ISR alle 20 ms
   * 
   */
  void PumpControl::timerCallback(void *)
  {
    if (PumpControl::pumpIsOn)
    {
      // war aktiv => deaktivieren
      gpio_set_level(Prefs::OUTPUT_PUMP_CONTROL, 0);
      PumpControl::pumpIsOn = false;
    }
    else
    {
      //
      // pumpe war inaktiv, soll was passieren?
      //
      if (Prefs::Preferences::pumpCycles > 0)
      {
        //
        // Punke anschalten, status markieren, Zyklen runterzählen, PUMP_LED einschalten
        //
        gpio_set_level(Prefs::OUTPUT_PUMP_CONTROL, 1);
        PumpControl::pumpIsOn = true;
        --Prefs::Preferences::pumpCycles;
        LedControl::setPumpLED(true);
      }
    }
  }
}
