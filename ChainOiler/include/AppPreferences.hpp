#pragma once
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "AppTypes.hpp"

namespace esp32s2
{
#ifdef RAWLED
  class LedControl; //! forward deklaration für friend
#endif
#ifdef LEDSTRIPE
  class LedStrip;
#endif
  class PumpControl;
}

namespace Prefs
{
  class Preferences
  {
  private:
    // statisch
    static const char *tag;                //! Kennzeichnung fürs debug
    static const char *serialStr;          //! Seriennummer
    static const std::string serialString; //! Seriennummer als string
    static nvs_handle_t nvs_handle;        //! Handle für den Speicher
    static bool isInit;                    //! ist der Speicher initialisiert?
    static uint32_t version;               //! version der einstellungen
    static std::string ssid;               //! SSID des Accesspoints
    static std::string ap_passwd;          //! Passwort des Accespoints
    static uint8_t ap_cannel;              //! Kanal des WiFi AP
    static uint8_t ap_max_connections;     //! maximale Verbindungen wlan AP
    static float pulsePerRound;            //! Impulse per Radumdrehung
    static float circumFerence;            //! Antriebsrad Umfang
    static float oilInterval;              //! Strecke zwischen den Ölgaben
    static float rainFactor;               //! Faktor für Öl intervall verlängerung
    static float crossFactor;              //! Faktor für öl beim crossen
    static float speedProgression;         //! Mehr öl bei höherer Geschwindigkeit
    static uint32_t rainSensorThreshold;   //! Schwellwert des Regensensors
    static uint64_t pumpLedTimeout;        //! wie lange leuchtet die LED nach
    static bool isAttentionFlag;           //! Flag für Ankündigung/Achtung (soll blinken auslösen)
    static portMUX_TYPE oilCycleMutex;     //! Mutex für zugriff auf Olpumpen zyklen

  protected:
    static opMode appOpMode;                    //! In welchem Zustand ist das Programm
    static float currentSpeedMeterPerSec;       //! aktuele Geschwindigkeit
    static float currentRouteLenPastOil;        //! Wegstrecke nach dem Ölen
    static double ackumulatedRouteLen;          //! Akkunukierte Wegstrecke
    static volatile uint64_t lastTachoPulse;    //! wann war der letzte Puls (deep sleep)
    static volatile fClick controlSwitchAction; //! ist ein Ereignis?
    static volatile fClick rainSwitchAction;    //! ist ein ereignis?
    static volatile uint8_t pumpCycles;         //! Anzahl der Pumenstösse, setzten aktiviert die Pumpe
    static volatile bool pumpAction;            //! Marker wenn pumpenstöse gesetzt werden, pumpCycles ist zu schnell auf 0

  public:
#ifdef RAWLED
    friend esp32s2::LedControl; //! ein Freund
#endif
#ifdef LEDSTRIPE
    friend esp32s2::LedStrip; //! ein Freund
#endif
    friend esp32s2::PumpControl;                   //! ein Freund
    static const std::string &getVersion();        //! Versionsstring zurückgeben
    static void init();                            //! das (statische) Objekt initialisieren
    static void close();                           //! Objekt schließen
    static void format();                          //! den Speicher schließen
    static void setApSSID(std::string);            //! Accesspoint SSID setzten
    static std::string getApSSID();                //! Accesspoint SSID lesen
    static void makeDefaults();                    //! Einstellungen initialisieren
    static void readPreferences();                 //! Einstellungen aus dem Speicher lesen
    static void setApPasswd(std::string);          //! Accesspoint Passwort setzen
    static std::string getApPasswd();              //! Accesspoint Passwort lesen
    static uint8_t getWiFiChannel();               //! Wifi Channel aus der Einstellung
    static void setWiFiChannel(uint8_t);           // setzte WifiChannel
    static uint8_t getMaxConnections();            //! wie viele WLAN Verbindungen max?
    static void setMaxConnections(uint8_t);        //! wie viele WLAN Verbindungen
    static void setPulsePerRound(float);           //! Setzte Impulse per Radumdrehung
    static float getPulsePerRound();               //! lese Impulse per Radumdrehung
    static void setCircumFerence(float);           //! setzte Radumfang
    static float getCircumFerence();               //! lese Radumfang
    static void setOilInterval(float);             //! Setzte die Strecke zwischen den Ölungen
    static float getOilInterval();                 //! lese Strecke zwischen den Ölungen
    static void setRainFactor(float);              //! Setzte den Faktor bei Regen
    static float getRainFactor();                  //! lese den Faktor bei Regen
    static void setCrossFactor(float);             //! setze den Faktor beim Crossbetrieb
    static float getCrossFactor();                 //! lese den Faktor beim Crossbetrieb
    static void setSpeedProgression(float);        //! setzte Faktor bei Geschwindigkeit
    static float getSpeedProgression();            //! lese den Faktor bei Geschwindigkeit
    static void setSensorThreshold(int);           //! setzte sen Schwelenwert des Regensensors
    static int getSensorThreshold();               //! lese Schwellenwert des Regensensors
    static uint16_t getPulsesFor100Meters();       //! gib impulse per 100 Meter, errechnet aus den Parametern
    static uint16_t getPulsesFor25Meters();        //! impulse per 25 Meter, für Tacho
    static uint16_t getMinimalPulseLength();       //! die kleinste Pulslänge in meiner Konfiguration
    static uint32_t getCounterPulsesForInterval(); //! wie viele impuse zum erreichen der strecke
    static opMode getAppMode();                    //! gib Operationsmode zurück
    static void setAppMode(opMode);                //! setzte Opertationsmode
    static fClick getRainSwitchAction();           //! war eine Aktion
    static void setRainSwitchAction(fClick);       //! setzte Aktion
    static fClick getControlSwitchAction();        //! war eine Control switch aktion
    static void setControlSwitchAction(fClick);    //! setzte Control Switch Aktion
    static void setCurrentSpeed(float);            //! setze aktuelle Geschwindigkeit
    static float getCurrentSpeed();                //! erfrage aktuelle Geschwindigkeit
    static void setRouteLenPastOil(float);         //! setze die Strecke nach dem Ölen
    static void addRouteLenPastOil(float);         //! füge Strecke nach dem Ölen hinzu
    static float getRouteLenPastOil();             //! gib die Strecke seit dem letzen Ölen zurück
    static double getAckumulatedRouteLen();        //! wie weit war es bisher
    static uint64_t getPumpLedTimeout();           //! LED Timeout bei Meldungen
    static void setPumpLedTimeout(uint64_t);       //! LED tiemout bei Meldungen
    static bool getAttentionFlag();                //! gib das ACHTUNG Flag zurück
    static void setAttentionFlag(bool);            //! die Achtung Flagge
    static bool writePreferences();                //! schreibe Einstellungen in den NVS
    static uint8_t getPumpCycles();                //! wie viele Pumpenzyklen sind aktuell?
    static void addPumpCycles(uint8_t);            //! zufügen von Zyklen
    static void setPumpCycles(uint8_t);            //! zyklen einstellen

  private:
    static uint8_t getIntValue(const char *, uint8_t);             //! lese einen 8 Bit UINT wert aus dem Speicher
    static bool setIntValue(const char *, uint8_t);                //! schreibe einen 8 Bit UINT wert in den Speicher
    static uint32_t getIntValue(const char *, uint32_t);           //! lese einen 32 Bit UINT wert aus dem Speicher
    static bool setIntValue(const char *, uint32_t);               //! schreibe einen 32 Bit UINT wert in den Speicher
    static uint64_t getIntValue(const char *, uint64_t);           //! lese einen 64 Bit UINT wert aus dem Speicher
    static bool setIntValue(const char *, uint64_t);               //! schreibe einen 64 Bit UINT wert in den Speicher
    static std::string getStringValue(const char *);               //! lese eine Zeichenkette aus dem Speicher
    static bool setStringValue(const char *, const char *);        //! schreibe eine Zeichenkette in den Speicher
    static bool setStringValue(const char *, const std::string &); //! schreibe eine Zeichenkette in den Speicher
    static float getFloatValue(const char *, float);               //! lies einen double Wert aus dem Speicher
    static bool setFloatValue(const char *, float);                //! schreibe einen double Wert in den Speicher
    static float getDoubleValue(const char *, double);             //! lies einen double Wert aus dem Speicher
    static bool setDoubleValue(const char *, double);              //! schreibe einen double Wert in den Speicher
  };
} // namespace Prefs
