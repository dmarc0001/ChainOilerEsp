#include "LedStripeControl.hpp"
#include <esp_log.h>
#include <esp_err.h>

namespace esp32s2
{
  /*
  const char *LedStripeControl::tag{"ledstrip"};
  gpio_num_t LedStripeControl::stripeRemoteTXPort{Prefs::LED_STRIPE_RMT_TX_GPIO};
  rmt_channel_t LedStripeControl::rmtChannel{Prefs::LED_STRIPE_RMT_CHANNEL};
  uint32_t LedStripeControl::ledCount{Prefs::LED_STRIPE_COUNT};
  led_strip_t *LedStripeControl::strip{nullptr};
  bool LedStripeControl::changed{true};
  */
  portMUX_TYPE LedStripeControl::colorMutex{0U, 0U};
  bool LedStripeControl::colorsComputed{false};
  uint32_t LedStripeControl::blackRGBA{0U};
  uint32_t LedStripeControl::controlRGBA{0U};
  uint32_t LedStripeControl::crossRGBA{0U};
  uint32_t LedStripeControl::crossDarkRGBA{0U};
  uint32_t LedStripeControl::pumpRGBA{0U};
  uint32_t LedStripeControl::rainRGBA{0U};
  uint32_t LedStripeControl::attentionRGBA{0U};
  uint32_t LedStripeControl::accessPointRGBA{0U};

  LedStripeControl::LedStripeControl() : stripeRemoteTXPort{Prefs::LED_STRIPE_RMT_TX_GPIO}, rmtChannel{Prefs::LED_STRIPE_RMT_CHANNEL}, ledCount{Prefs::LED_STRIPE_COUNT}, strip{nullptr}, changed{true}
  {
    //
    // Farben für LED vorher zurechtlegen (speed optimized)
    //
    if (!LedStripeControl::colorsComputed)
    {
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_CONTROL_HSVCOLOR, 100, 100, &LedStripeControl::controlRGBA);
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_CROSS_HSVCOLOR, 100, 100, &LedStripeControl::crossRGBA);
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_CROSS_HSVCOLOR, 100, 5, &LedStripeControl::crossDarkRGBA);
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_PUMP_HSVCOLOR, 100, 100, &LedStripeControl::pumpRGBA);
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_RAIN_HSVCOLOR, 100, 100, &LedStripeControl::rainRGBA);
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_ATT_HSVCOLOR, 100, 100, &LedStripeControl::attentionRGBA);
      LedStripeControl::hsv2rgba(Prefs::LED_STRIPE_AP_HSVCOLOR, 100, 100, &LedStripeControl::accessPointRGBA);
      LedStripeControl::colorsComputed = true;
    }
  }

  /**
   * @brief initialize dribvers for WS2812
   *
   */
  void LedStripeControl::init()
  {
    ESP_LOGD(tag, "initialize WS2812 led stripe...");
    //
    rmt_config_t config =
        {
            .rmt_mode = RMT_MODE_TX,
            .channel = rmtChannel,
            .gpio_num = stripeRemoteTXPort,
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
    // Konfiguriere...
    ESP_ERROR_CHECK(rmt_config(&config));
    // remote driver install
    ESP_LOGD(tag, "install WS2812 led stripe driver...");
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    // install ws2812 driver
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(ledCount, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip)
    {
      ESP_LOGE(tag, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_LOGD(tag, "clear all led...");
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    changed = false;
    //
    ESP_LOGD(tag, "install WS2812 led stripe driver...OK");
  }

  /**
   * @brief clear all LED
   *
   */
  void LedStripeControl::allOff()
  {
    if (!strip)
      return;
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    changed = false;
  }

  void LedStripeControl::setRainLED(bool _set)
  {
    if (!strip)
      return;
    if (_set)
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, LedStripeControl::rainRGBA >> 24, LedStripeControl::rainRGBA >> 16, LedStripeControl::rainRGBA >> 8));
    else
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, 0U, 0U, 0U));
    changed = true;
  }

  void LedStripeControl::setControlLED(bool _set)
  {
    if (!strip)
      return;
    if (_set)
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_CONTROL, LedStripeControl::controlRGBA >> 24, LedStripeControl::controlRGBA >> 16, LedStripeControl::controlRGBA >> 8));
    else
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_CONTROL, 0U, 0U, 0U));
    changed = true;
  }

  void LedStripeControl::setControlCrossLED(bool _set)
  {
    if (!strip)
      return;
    //
    // Regen kan ncht gleichzeitig mit CROSS aktiv sein, daher geht das
    //
    if (_set)
    {
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_CONTROL, LedStripeControl::crossRGBA >> 24, LedStripeControl::crossRGBA >> 16, LedStripeControl::crossRGBA >> 8));
      // ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, LedStripeControl::crossDarkRGBA >> 24, LedStripeControl::crossDarkRGBA >> 16, LedStripeControl::crossDarkRGBA >> 8));
    }
    else
    {
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_CONTROL, LedStripeControl::crossDarkRGBA >> 24, LedStripeControl::crossDarkRGBA >> 16, LedStripeControl::crossDarkRGBA >> 8));
      // ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, LedStripeControl::crossRGBA >> 24, LedStripeControl::crossRGBA >> 16, LedStripeControl::crossRGBA >> 8));
    }
    changed = true;
  }

  void LedStripeControl::setPumpLED(bool _set)
  {
    if (!strip)
      return;
    if (_set)
    {
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_PUMP, LedStripeControl::pumpRGBA >> 24, LedStripeControl::pumpRGBA >> 16, LedStripeControl::pumpRGBA >> 8));
      pumpFadingValue = 100U;
    }
    else
    {
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_PUMP, 0U, 0U, 0U));
      pumpFadingValue = 0U;
    }
    changed = true;
  }

  bool LedStripeControl::fadeOutPumpLED()
  {
    if (!strip)
      return true;
    if (pumpFadingValue == 0)
      return true;
    // berechnen welche Farbe dran ist
    uint32_t r, g, b;
    if (pumpFadingValue > fadingStep)
      pumpFadingValue -= fadingStep;
    else
      pumpFadingValue = 0U;
    LedStripeControl::hsv2rgb(Prefs::LED_STRIPE_PUMP_HSVCOLOR, 100U, pumpFadingValue, &r, &g, &b);
    ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_PUMP, r, g, b));
    changed = true;
    // noch nicht fertig
    return false;
  }

  void LedStripeControl::setAttentionLED(bool _set)
  {
    if (!strip)
      return;
    if (_set)
    {
      ESP_ERROR_CHECK(strip->set_pixel(LedStripeControl::strip, Prefs::LED_STRIPE_CONTROL, LedStripeControl::attentionRGBA >> 24, LedStripeControl::attentionRGBA >> 16, LedStripeControl::attentionRGBA >> 8));
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, 0U, 0U, 0U));
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_PUMP, LedStripeControl::attentionRGBA >> 24, LedStripeControl::attentionRGBA >> 16, LedStripeControl::attentionRGBA >> 8));
    }
    else
    {
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_CONTROL, 0U, 0U, 0U));
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, LedStripeControl::attentionRGBA >> 24, LedStripeControl::attentionRGBA >> 16, LedStripeControl::attentionRGBA >> 8));
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_PUMP, 0U, 0U, 0U));
    }
    changed = true;
  }

  void LedStripeControl::setAPModeLED(bool _set)
  {
    if (!strip)
      return;
    if (_set)
    {
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_CONTROL, LedStripeControl::accessPointRGBA >> 24, LedStripeControl::accessPointRGBA >> 16, LedStripeControl::accessPointRGBA >> 8));
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_RAIN, LedStripeControl::accessPointRGBA >> 24, LedStripeControl::accessPointRGBA >> 16, LedStripeControl::accessPointRGBA >> 8));
      ESP_ERROR_CHECK(strip->set_pixel(strip, Prefs::LED_STRIPE_PUMP, LedStripeControl::accessPointRGBA >> 24, LedStripeControl::accessPointRGBA >> 16, LedStripeControl::accessPointRGBA >> 8));
      changed = true;
    }
    else
    {
      ESP_ERROR_CHECK(strip->clear(strip, 100));
      changed = false;
    }
  }

  /**
   * @brief refresh/flush all pixels
   *
   */
  void LedStripeControl::makeChange()
  {
    if (!changed)
      return;
    if (!strip)
      return;
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
    changed = false;
  }

  /**
   * @brief set one pixel without flush
   *
   * @param _index
   * @param _rgba
   */
  void LedStripeControl::setPixel(uint32_t _index, uint32_t _rgba)
  {
    if (!strip)
      return;
    ESP_ERROR_CHECK(strip->set_pixel(strip, _index, _rgba >> 24, _rgba >> 16, _rgba >> 8));
  }

  /**
   * @brief vonvert color rooms hsv => rgba
   *
   * @param _hue
   * @param _sat
   * @param _value
   * @param _rgba
   */
  void LedStripeControl::hsv2rgba(uint32_t _hue, uint32_t _sat, uint32_t _value, uint32_t *_rgba)
  {
    portENTER_CRITICAL(&LedStripeControl::colorMutex);

    _hue %= 360; // h -> [0,360]
    uint32_t rgb_max = _value * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - _sat) / 100.0f;

    uint32_t hue_i = _hue / 60;
    uint32_t diff = _hue % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (hue_i)
    {
    case 0:
      *_rgba = rgb_max << 24 | (rgb_min + rgb_adj) << 16 | rgb_min << 8;
      break;
    case 1:
      *_rgba = (rgb_max - rgb_adj) << 24 | rgb_max << 16 | rgb_min << 8;
      break;
    case 2:
      *_rgba = rgb_min << 24 | rgb_max << 16 | (rgb_min + rgb_adj) << 8;
      break;
    case 3:
      *_rgba = rgb_min << 24 | (rgb_max - rgb_adj) << 16 | rgb_max << 8;
      break;
    case 4:
      *_rgba = (rgb_min + rgb_adj) << 24 | rgb_min << 16 | rgb_max << 8;
      break;
    default:
      *_rgba = rgb_max << 24 | rgb_min << 16 | (rgb_max - rgb_adj) << 8;
      break;
    }
    portEXIT_CRITICAL(&LedStripeControl::colorMutex);
  }

  /**
   * @brief convert HSV => R G B
   *
   * @param hue
   * @param sat
   * @param value
   * @param red
   * @param green
   * @param blue
   */
  void LedStripeControl::hsv2rgb(uint32_t hue, uint32_t sat, uint32_t value, uint32_t *red, uint32_t *green, uint32_t *blue)
  {
    portENTER_CRITICAL(&LedStripeControl::colorMutex);

    hue %= 360; // h -> [0,360]
    uint32_t rgb_max = value * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - sat) / 100.0f;

    uint32_t hue_i = hue / 60;
    uint32_t diff = hue % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (hue_i)
    {
    case 0:
      *red = rgb_max;
      *green = rgb_min + rgb_adj;
      *blue = rgb_min;
      break;
    case 1:
      *red = rgb_max - rgb_adj;
      *green = rgb_max;
      *blue = rgb_min;
      break;
    case 2:
      *red = rgb_min;
      *green = rgb_max;
      *blue = rgb_min + rgb_adj;
      break;
    case 3:
      *red = rgb_min;
      *green = rgb_max - rgb_adj;
      *blue = rgb_max;
      break;
    case 4:
      *red = rgb_min + rgb_adj;
      *green = rgb_min;
      *blue = rgb_max;
      break;
    default:
      *red = rgb_max;
      *green = rgb_min;
      *blue = rgb_max - rgb_adj;
      break;
    }
    portEXIT_CRITICAL(&LedStripeControl::colorMutex);
  }

}
