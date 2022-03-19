#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "soc/rtc_wdt.h"
#include "led_strip.h"
#include "ProjectDefaults.hpp"

namespace esp32s2
{
  class LedStrip
  {
  private:
    static const char *tag;               //! der Logname
    static gpio_num_t stripeRemoteTXPort; //! port für Kontrolle
    static rmt_channel_t rmtChannel;      //! remote channel
    static uint32_t ledCount;             //! Anzahl der LED im Streifen
    static portMUX_TYPE colorMutex;       //! Mutex für zugriff auf hsv-umrechnung
    static led_strip_t *strip;            //! zeiger auf STRIP objekt/struktur

  public:
    static void init();
    static void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
    static void hsv2rgba(uint32_t h, uint32_t s, uint32_t v, uint32_t *rgba);
    static void clear();
    static void setPixel(uint32_t, uint32_t); //! set pixel (index, rgba)
    static void refresh();
    static void flashControlLed(int64_t); // control led soll blitzen

  private:
    LedStrip();
  };

}
