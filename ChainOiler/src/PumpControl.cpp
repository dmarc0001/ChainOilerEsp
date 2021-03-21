#include "PumpControl.hpp"
#include "esp_log.h"

namespace esp32s2
{
  const char *PumpControl::tag{"PumpControl"};

  void PumpControl::init()
  {
    using namespace Prefs;
    //
    // GPIO Konfigurieren
    //
    ESP_LOGD(tag, "%s: init pump hardware...", __func__);
    //
    // Ausgabesignale Digital
    //
    gpio_config_t config_out = {.pin_bit_mask = BIT64(Prefs::OUTPUT_PUMP_CONTROL),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_out);
    ESP_LOGD(tag, "%s: init pump hardware...OK", __func__);
  }
}
