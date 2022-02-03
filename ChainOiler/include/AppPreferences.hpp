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
  class LedControl;  //! forward deklaration für friend
  class PumpControl;
}  // namespace esp32s2

namespace Prefs
{
  class Preferences
  {
    private:
    // statisch
    static const char *tag;                 //! Kennzeichnung fürs debug
    static const char *serialStr;           //! Seriennummer
    static const std::string serialString;  //! Seriennummer als string
    static nvs_handle_t nvs_handle;         //! Handle für den Speicher
    static bool isInit;                     //! ist der Speicher initialisiert?
    static int32_t version;                 //! version der einstellungen
    static std::string ssid;                //! SSID des Accesspoints
    static std::string ap_passwd;           //! Passwort des Accespoints
    static uint8_t ap_cannel;               //! Kanal des WiFi AP
    static uint8_t ap_max_connections;      //! Maximale Verbindungen gleichzeitig
    static float pulsePerRound;             //! Impulse per Radumdrehung
    static float circumFerence;             //! Antriebsrad Umfang
    static float oilInterval;               //! Strecke zwischen den Ölgaben
    static float rainFactor;                //! Faktor für Öl intervall verlängerung
    static float crossFactor;               //! Faktor für öl beim crossen
    static float speedProgression;          //! Mehr öl bei höherer Geschwindigkeit
    static int32_t rainSensorThreshold;     //! Schwellwert des Regensensors
    static uint64_t pumpLedTimeout;         //! wiel lange leuchtet die LED nach?
    static bool isAttentionFlag;            //! ankündigung/Achtung Flag
    protected:
    static opMode appOpMode;                     //! In welchem Zustand ist das Programm
    static float currentSpeedMeterPerSec;        //! aktuele Geschwindigkeit
    static float currentRouteLenPastOil;         //! Wegstrecke nach dem Ölen
    static volatile uint64_t lastTachoPulse;     //! wann war der letzte Puls (deep sleep)
    static volatile fClick controlSwitchAction;  //! ist ein Ereignis?
    static volatile fClick rainSwitchAction;     //! ist ein ereignis?
    static portMUX_TYPE oilCycleMutex;           //! Mutex für Zugriff auf Ölpumpen Zyklenanzahl
    static volatile uint8_t pumpCycles;          //! Anzahl der Pumenstösse, setzten aktiviert die Pumpe

    public:
    friend esp32s2::LedControl;                    //! ein Freund
    friend esp32s2::PumpControl;                   //! ein Freund
    static const std::string &getVersion();        //! Versionsstring zurückgeben
    static void init();                            //! das (statische) Objekt initialisieren
    static void close();                           //! Objekt schließen
    static void format();                          //! den Speicher schließen
    static void setApSSID( std::string );          //! Accesspoint SSID setzten
    static std::string getApSSID();                //! Accesspoint SSID lesen
    static uint8_t getWiFiChannel();               //! Kanal des Accesspoint herausgeben
    static void setWiFiChannel( uint8_t ch );      //! Kanal WiFi setzen
    static uint8_t getMaxConnections();            //! Wie viele Verbindungen gleichzeitig
    static void setMaxConnections( uint8_t max );  //! setzte maximala Anzahl der verbindungen
    static void makeDefaults();                    //! Einstellungen initialisieren
    static void readPreferences();                 //! Einstellungen aus dem Speicher lesen
    static bool writePreferences();                //! Einstellungen in den NVS Speicher sichern
    static void setApPasswd( std::string );        //! Accesspoint Passwort setzen
    static std::string getApPasswd();              //! Accesspoint Passwort lesen
    static void setPulsePerRound( float );         //! Setzte Impulse per Radumdrehung
    static float getPulsePerRound();               //! lese Impulse per Radumdrehung
    static void setCircumFerence( float );         //! setzte Radumfang
    static float getCircumFerence();               //! lese Radumfang
    static void setOilInterval( float );           //! Setzte die Strecke zwischen den Ölungen
    static float getOilInterval();                 //! lese Strecke zwischen den Ölungen
    static void setRainFactor( float );            //! Setzte den Faktor bei Regen
    static float getRainFactor();                  //! lese den Faktor bei Regen
    static void setCrossFactor( float );           //! setze den Faktor beim Crossbetrieb
    static float getCrossFactor();                 //! lese den Faktor beim Crossbetrieb
    static void setSpeedProgression( float );      //! setzte Faktor bei Geschwindigkeit
    static float getSpeedProgression();            //! lese den Faktor bei Geschwindigkeit
    static void setSensorThreshold( int );         //! setzte sen Schwelenwert des Regensensors
    static int getSensorThreshold();               //! lese Schwellenwert des Regensensors
    static void setPumpLedTimeout( uint64_t );     //! setzte nachleuchten der pumpen LED
    static uint64_t getPumpLedTimeout();           //! wie lange leuchtet die Pumpen LED nach
    static int16_t getPulsesFor100Meters();        //! gib impulse per 100 Meter, errechnet aus den Parametern
    static int16_t getPulsesFor10Meters();         //! impulse per 10 Meter, für Tacho
    static uint16_t getMinimalPulseLength();       //! die kleinste Pulslänge in meiner Konfiguration
    static opMode getAppMode();                    //! gib Operationsmode zurück
    static void setAppMode( opMode );              //! setzte Opertationsmode
    static fClick getRainSwitchAction();           //! war eine Aktion
    static void setRainSwitchAction( fClick );     //! setzte Aktion
    static fClick getControlSwitchAction();        //! war eine Control switch aktion
    static void setControlSwitchAction( fClick );  //! setzte Control Switch Aktion
    static void setCurrentSpeed( float );          //! setze aktuelle Geschwindigkeit
    static float getCurrentSpeed();                //! erfrage aktuelle Geschwindigkeit
    static void setRouteLenPastOil( float );       //! setze die Strecke nach dem Ölen
    static void addRouteLenPastOil( float );       //! füge Strecke nach dem Ölen hinzu
    static float getRouteLenPastOil();             //! gib die Strecke seit dem letzen Ölen zurück
    static void setAttentionFlag( bool );          //! setzte das Achtung Flag
    static bool getAttentionFlag();                //! lese das Achtung Flag
    static void setPumpCycles( uint8_t );          //! Setze Pumpenzyklen, zurücknehmen macht Pumpen ISR
    static void addPumpCycles( uint8_t );          //! pumpenzyklen zufügen
    static uint8_t getPumpCycles();                //! wieviel ist noch offen?

    private:
    static int32_t getIntValue( const char *, int32_t );              //! lese einen 32 Bit INT wert aus dem Speicher
    static bool setIntValue( const char *, int32_t );                 //! schreibe einen 32 Bit INT wert in den Speicher
    static std::string getStringValue( const char * );                //! lese eine Zeichenkette aus dem Speicher
    static bool setStringValue( const char *, const char * );         //! schreibe eine Zeichenkette in den Speicher
    static bool setStringValue( const char *, const std::string & );  //! schreibe eine Zeichenkette in den Speicher
    static float getFloatValue( const char *, float );                //! lies einen double Wert aus dem Speicher
    static bool setFloatValue( const char *, float );                 //! schreibe einen double Wert in den Speicher
  };
}  // namespace Prefs
