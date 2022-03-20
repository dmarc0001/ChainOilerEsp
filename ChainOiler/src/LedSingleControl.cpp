#include "LedSingleControl.hpp"
#include <esp_log.h>
#include <esp_err.h>

namespace esp32s2
{

  LedControl::LedControl() : ledBundle(nullptr), ctrlLEDMask(0U), ctrlLedValue(0U)
  {
  }

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
    ESP_ERROR_CHECK(dedic_gpio_new_bundle(&bundleConfig, &ledBundle));
    ESP_LOGD(tag, "init hardware for LED...done");
  }

  /**
   * @brief Alle LED ausschalten
   *
   */
  void LedControl::allOff()
  {
    //
    // alles ausschalten
    //
    ctrlLEDMask = G_LED_REED_CONTROL_MASK | G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
    ctrlLedValue = 0U;
    makeChange();
  }

  void LedControl::setAttentionLED(bool _set)
  {
    //
    // LED vorbereiten
    //
    if (_set)
    {
      ctrlLedValue = (ctrlLedValue | G_LED_CONTROL_MASK | G_LED_PUMP_MASK) & !G_LED_RAIN_MASK;
    }
    else
    {
      ctrlLedValue = (ctrlLedValue | G_LED_RAIN_MASK) & !(G_LED_CONTROL_MASK | G_LED_PUMP_MASK);
    }
    ctrlLEDMask |= G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
  }

  /**
   * @brief schalte Regen LED an/aus
   *
   * @param _set
   */
  void LedControl::setRainLED(bool _set)
  {
    //
    // LED vorbereiten
    //
    if (_set)
    {
      ctrlLedValue |= G_LED_RAIN_MASK;
    }
    else
    {
      ctrlLedValue &= !G_LED_RAIN_MASK;
    }
    ctrlLEDMask |= G_LED_RAIN_MASK;
  }

  void LedControl::setControlLED(bool _set)
  {
    //
    // LED vorbereiten
    //
    if (_set)
    {
      ctrlLedValue |= G_LED_CONTROL_MASK;
    }
    else
    {
      ctrlLedValue &= !G_LED_CONTROL_MASK;
    }
    ctrlLEDMask |= G_LED_CONTROL_MASK;
  }

  void LedControl::setControlCrossLED(bool _set)
  {
    //
    // LED vorbereiten
    //
    if (_set)
    {
      ctrlLedValue |= G_LED_CONTROL_MASK | G_LED_RAIN_MASK;
    }
    else
    {
      ctrlLedValue &= !(G_LED_CONTROL_MASK | G_LED_RAIN_MASK);
    }
    ctrlLEDMask |= G_LED_CONTROL_MASK | G_LED_RAIN_MASK;
  }

  void LedControl::setPumpLED(bool _set)
  {
    //
    // LED vorbereiten
    //
    if (_set)
    {
      ctrlLedValue |= G_LED_PUMP_MASK;
    }
    else
    {
      ctrlLedValue &= !G_LED_PUMP_MASK;
    }
    ctrlLEDMask |= G_LED_PUMP_MASK;
  }

  bool LedControl::fadeOutPumpLED()
  {
    // hier geht nur ausschalten
    ctrlLedValue &= !G_LED_PUMP_MASK;
    ctrlLEDMask |= G_LED_PUMP_MASK;
    return true;
  }

  void LedControl::setAPModeLED(bool _set)
  {
    //
    // LED vorbereiten
    //
    if (_set)
    {
      ctrlLedValue = G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
    }
    else
    {
      ctrlLedValue = !(G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK);
    }
    ctrlLEDMask = G_LED_CONTROL_MASK | G_LED_PUMP_MASK | G_LED_RAIN_MASK;
  }

  /**
   * @brief setzte alle gemachten Änderungen
   *
   */
  void LedControl::makeChange()
  {
    //
    // wenn es was zu tun gibt
    //
    if (ctrlLEDMask == 0U)
      return;
    // ausführen der Änderungen
    dedic_gpio_bundle_write(ledBundle, ctrlLEDMask, ctrlLedValue);
    ctrlLEDMask = 0U;  // Maske für zu beackernde LED
    ctrlLedValue = 0U; // Wert zum setzten
  }
} // namespace
