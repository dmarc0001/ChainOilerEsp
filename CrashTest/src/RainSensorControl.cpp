#include "RainSensorControl.hpp"

namespace esp32s2
{
  /**
   * @brief instanziere und initialisiere statische variable
   * 
   */
  const char *RainSensorControl::tag{"RainSensorControl"};

  /**
   * @brief initialisiere die Hardware für Regensensor
   * 
   */
  void RainSensorControl::init()
  {
    using namespace Prefs;

    //
    // GPIO Konfigurieren
    //
    ESP_LOGI(tag, "init rain sensor...");
    //
    // Ausgabesignale Digital
    //
    gpio_config_t config_out = {.pin_bit_mask = BIT64(OUTPUT_RAIN_SW_01) | BIT64(OUTPUT_RAIN_SW_02),
                                .mode = GPIO_MODE_OUTPUT,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_ENABLE,
                                .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&config_out);
    //
    // Analog-Digital-Wandler starten
    // TODO: Messbereich optimieren
    // Setzte Auflösung auf 13 Bit
    // TODO: 0.1 uF an ADC
    //
    ESP_LOGD(tag, "init ADC...");
    adc1_config_width(ADC_WIDTH_BIT_13);
    //
    // Dämpfung für Meßbereich einstellen
    //
    adc1_config_channel_atten(INPUT_ADC_RAIN_00, ADC_ATTEN_DB_11 /*ADC_ATTEN_DB_0*/);
    adc1_config_channel_atten(INPUT_ADC_RAIN_01, ADC_ATTEN_DB_11 /*ADC_ATTEN_DB_0*/);
    ESP_LOGD(tag, "init ...done");
  }

  /**
   * Regensensor Wert zurück geben
   */
  rain_value_t RainSensorControl::getRainValues()
  {
    using namespace Prefs;

    rain_value_t val;
    val.first = adc1_get_raw(INPUT_ADC_RAIN_00);
    val.second = adc1_get_raw(INPUT_ADC_RAIN_01);
    return (val);
  }

}
