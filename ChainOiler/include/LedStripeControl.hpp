#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <driver/rmt.h>
#include <soc/rtc_wdt.h>
#include <led_strip.h>
#include "ProjectDefaults.hpp"

namespace esp32s2
{
  class LedStripeControl
  {
  private:
    static const char *tag;               //! der Logname
    static gpio_num_t stripeRemoteTXPort; //! port für Kontrolle
    static rmt_channel_t rmtChannel;      //! remote channel
    static uint32_t ledCount;             //! Anzahl der LED im Streifen
    static portMUX_TYPE colorMutex;       //! Mutex für zugriff auf hsv-umrechnung
    static led_strip_t *strip;            //! zeiger auf STRIP objekt/struktur
    static bool changed;                  //! gab es änderungen nach dem refresh?
    static uint32_t blackRGBA;            //! dunkel
    static uint32_t controlRGBA;          //! farbe für CONTROL
    static uint32_t crossRGBA;            //! farbe für CROSS Betrieb
    static uint32_t crossDarkRGBA;        //! farbe für CROSS dunkelphase Betrieb
    static uint32_t pumpRGBA;             //! Farbe für Pumpe
    static uint32_t rainRGBA;             //! Farbe für Regen
    static uint32_t attentionRGBA;        //! Farbe für Attention
    static uint32_t accessPointRGBA;      //! farbe für AP

  public:
    static void init();
    static void allOff();                 //! alles ausschalten
    static void setRainLED(bool);         //! LED für Regen einschalten
    static void setControlLED(bool);      //! LED für ..ms ein
    static void setControlCrossLED(bool); //! LED für ..ms ein
    static void setPumpLED(bool);         //! pump led ein ....ms
    static void setAttentionLED(bool);    //! Achtung LED ein oder AUS
    static void setAPModeLED(bool);       //! LEDs für AP Mide ein
    static void makeChange();             //! Veränderungen setzen
    //
    static void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
    static void hsv2rgba(uint32_t h, uint32_t s, uint32_t v, uint32_t *rgba);
    static void setPixel(uint32_t, uint32_t); //! set pixel (index, rgba)

  private:
    LedStripeControl();
  };

}
