#pragma once
#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"

namespace Prefs
{
  class Preferences
  {
  private:
    static const char *tag;                //! Kennzeichnung fürs debug
    static const char *serialStr;          //! Seriennummer
    static const std::string serialString; //! Seriennummer als string
    static nvs_handle_t nvs_handle;        //! Handle für den Speicher
    static bool isInit;                    //! ist der Speicher initialisiert?
    static int32_t version;                //! version der einstellungen
    static std::string ssid;               //! SSID des Accesspoints
    static std::string ap_passwd;          //! Passwort des Accespoints
    static double pulsePerRound;           //! Impulse per Radumdrehung
    static double circumFerence;           //! Antriebsrad Umfang
    static double oilInterval;             //! Strecke zwischen den Ölgaben
    static double rainFactor;              //! Faktor für Öl intervall verlängerung
    static double crossFactor;             //! Faktor für öl beim crossen
    static double speedProgression;        //! Mehr öl bei höherer Geschwindigkeit
    static int rainSensorThreshold;        //! Schwellwert des Regensensors

  protected:
    static volatile uint32_t tachoPulseCount;         //! zähle die impulse zum Streckenmessen
    static volatile uint32_t tachoPulseForSpeedCount; //! zähle die impulse zum Geschwindigkeit messen
    static volatile uint32_t lastTachoPulse;          //! wann war der letzte Puls (deep sleep)

  public:
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
    static void setPulsePerRound(double);          //! Setzte Impulse per Radumdrehung
    static double getPulsePerRound();              //! lese Impulse per Radumdrehung
    static void setCircumFerence(double);          //! setzte Radumfang
    static double getCircumFerence();              //! lese Radumfang
    static void setOilInterval(double);            //! Setzte die Strecke zwischen den Ölungen
    static double getOilInterval();                //! lese Strecke zwischen den Ölungen
    static void setRainFactor(double);             //! Setzte den Faktor bei Regen
    static double getRainFactor();                 //! lese den Faktor bei Regen
    static void setCrossFactor(double);            //! setze den Faktor beim Crossbetrieb
    static double getCrossFactor();                //! lese den Faktor beim Crossbetrieb
    static void setSpeedProgression(double);       //! setzte Faktor bei Geschwindigkeit
    static double getSpeedProgression();           //! lese den Faktor bei Geschwindigkeit
    static void setSensorThreshold(int);           //! setzte sen Schwelenwert des Regensensors
    static int getSensorThreshold();               //! lese Schwellenwert des Regensensors
    static int16_t getPulsesFor100Meters();        //! gib impulse per 100 Meter, errechnet aus den Parametern
    static int16_t getPulsesFor10Meters();         //! impulse per 5 Metrer, für Tacho
    static uint16_t getMinimalPulseLength();       //! die kleinste Pulslänge in meiner Konfiguration
    static uint32_t getCounterPulsesForInterval(); //! wie viele impuse zum erreichen der strecke

  private:
    static int32_t getIntValue(const char *, int32_t);      //! lese einen 32 Bit INT wert aus dem Speicher
    static bool setIntValue(const char *, int32_t);         //! schreibe einen 32 Bit INT wert in den Speicher
    static std::string getStringValue(const char *);        //! lese eine Zeichenkette aus dem Speicher
    static bool setStringValue(const char *, const char *); //! schreibe eine Zeichenkette in den Speicher
    static double getDoubleValue(const char *, double);     //! lies einen double Wert aus dem Speicher
    static bool setDoubleValue(const char *, double);       //! schreibe einen double Wert in den Speicher
  };
} // namespace Prefs
