#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_task_wdt.h>
#include <driver/rmt.h>
#include <soc/rtc_wdt.h>
#include <led_strip.h>
#include "ProjectDefaults.hpp"
#include "SignalControlAbstract.hpp"

namespace esp32s2
{
  constexpr uint32_t fadingStep = 3U;

  class LedStripeControl : public SignalControlAbstract
  {
    //
    // beim instanzieren darf channel und port nicht doppelt vergeben sein
    // TODO: eine Sperre einbauen
    //
  protected:
    const char *tag = "ledstripe";

  private:
    gpio_num_t stripeRemoteTXPort;   //! port für Kontrolle
    rmt_channel_t rmtChannel;        //! remote channel
    uint32_t ledCount;               //! Anzahl der LED im Streifen
    static portMUX_TYPE colorMutex;  //! Mutex für zugriff auf hsv-umrechnung
    led_strip_t *strip;              //! zeiger auf STRIP objekt/struktur
    bool changed;                    //! gab es änderungen nach dem refresh?
    static bool colorsComputed;      //! Farben schon berechnet?
    static uint32_t blackRGBA;       //! dunkel
    static uint32_t controlRGBA;     //! farbe für CONTROL
    static uint32_t crossRGBA;       //! farbe für CROSS Betrieb
    static uint32_t crossDarkRGBA;   //! farbe für CROSS dunkelphase Betrieb
    static uint32_t pumpRGBA;        //! Farbe für Pumpe
    static uint32_t rainRGBA;        //! Farbe für Regen
    static uint32_t attentionRGBA;   //! Farbe für Attention
    static uint32_t accessPointRGBA; //! farbe für AP
    uint32_t pumpFadingValue;        //! Fading für Pumpe

  public:
    LedStripeControl();                    //! Der Konstruktor
    virtual void init();                   //! hardware/values initialisieren
    virtual void allOff();                 //! alles ausschalten
    virtual void setRainLED(bool);         //! LED für Regen einschalten
    virtual void setControlLED(bool);      //! LED für Control ein
    virtual void setControlCrossLED(bool); //! LED für Cross ein
    virtual void setPumpLED(bool);         //! LED für Pumpe ein
    virtual bool fadeOutPumpLED();         //! Pumpen LED ausblenden
    virtual void setAttentionLED(bool);    //! Achtung LED ein oder AUS
    virtual void setAPModeLED(bool);       //! LEDs für AP Mide ein
    virtual void makeChange();             //! Veränderungen setzen
    void setPixel(uint32_t, uint32_t);     //! set pixel (index, rgba)
    //
    static void hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);
    static void hsv2rgba(uint32_t h, uint32_t s, uint32_t v, uint32_t *rgba);

  private:
  };

}
