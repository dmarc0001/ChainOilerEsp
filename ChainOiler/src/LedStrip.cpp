#include "LedStrip.hpp"

namespace esp32s2
{
  const char *LedStrip::tag{"ledstrip"};
  gpio_num_t LedStrip::stripeRemoteTXPort{Prefs::LED_STRIPE_RMT_TX_GPIO};
  rmt_channel_t LedStrip::rmtChannel{Prefs::LED_STRIPE_RMT_CHANNEL};
  uint32_t LedStrip::ledCount{Prefs::LED_STRIPE_COUNT};
  portMUX_TYPE LedStrip::colorMutex{0U, 0U};
  led_strip_t *LedStrip::strip{nullptr};

  /**
   * @brief initialize dribvers for WS2812
   *
   */
  void LedStrip::init()
  {
    ESP_LOGD(tag, "initialize WS2812 led stripe...");
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
    LedStrip::strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!LedStrip::strip)
    {
      ESP_LOGE(tag, "install WS2812 driver failed");
    }
    // Clear LED strip (turn off all LEDs)
    ESP_LOGD(tag, "clear all led...");
    ESP_ERROR_CHECK(LedStrip::strip->clear(LedStrip::strip, 100));
    //
    ESP_LOGD(tag, "install WS2812 led stripe driver...OK");
  }

  /**
   * @brief clear all LED
   *
   */
  void LedStrip::clear()
  {
    if (!LedStrip::strip)
      return;
    ESP_ERROR_CHECK(LedStrip::strip->clear(LedStrip::strip, 100));
  }

  /**
   * @brief set one pixel without flush
   *
   * @param _index
   * @param _rgba
   */
  void LedStrip::setPixel(uint32_t _index, uint32_t _rgba)
  {
    if (!LedStrip::strip)
      return;
    ESP_ERROR_CHECK(LedStrip::strip->set_pixel(LedStrip::strip, _index, _rgba >> 24, _rgba >> 16, _rgba >> 8));
  }

  /**
   * @brief refresh/flush all pixels
   *
   */
  void LedStrip::refresh()
  {
    if (!LedStrip::strip)
      return;
    ESP_ERROR_CHECK(LedStrip::strip->refresh(LedStrip::strip, 100));
  }

  void LedStrip::flashControlLed(int64_t)
  {
    // TODO: blitzen einbauen
  }

  /**
   * @brief vonvert color rooms hsv => rgba
   *
   * @param _hue
   * @param _sat
   * @param _value
   * @param _rgba
   */
  void LedStrip::hsv2rgba(uint32_t _hue, uint32_t _sat, uint32_t _value, uint32_t *_rgba)
  {
    portENTER_CRITICAL(&LedStrip::colorMutex);

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
    portEXIT_CRITICAL(&LedStrip::colorMutex);
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
  void LedStrip::hsv2rgb(uint32_t hue, uint32_t sat, uint32_t value, uint32_t *red, uint32_t *green, uint32_t *blue)
  {
    portENTER_CRITICAL(&LedStrip::colorMutex);

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
    portEXIT_CRITICAL(&LedStrip::colorMutex);
  }

}
