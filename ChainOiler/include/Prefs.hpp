#pragma once
#include <Arduino.h>
#include "HardwareInit.hpp"
#include "LittleFS.h"
#include "ProjectDefaults.hpp"
#include "SPI.h"

namespace Preferences
{
  //! Typ des Tastendrucks (Kein, kurz,lang)
  enum fClick : uint8_t
  {
    NONE,
    SHORT,
    LONG
  };

  //! Betriebsart des Ölers (normal, regen, cross, accesspoint, test)
  enum opMode : uint8_t
  {
    AWAKE,
    NORMAL,
    RAIN,
    CROSS,
    APMODE,
    TEST
  };

  //! Das Objekt ist nur genau einmal vorhanden, also sind die Variablen static
  class Prefs
  {
    private:
    static const char *serialStr;
    static String WLANSSID;                //! SSID fuer den Accesspoint
    static String WLANPassword;            //! das WLAN Passwort fuer den Accesspoint
    static double pulsePerWeelRound;       //! Defaultwert für Reed ist 1, Honda Africa Twin 109
    static double weelCircumFerence;       //! Umfang Hinterrad
    static double normalOilInterval;       //! Schmierinterval standart ist 4000 Meter
    static double rainOilIntervalFactor;   //! wieviel mal gegenüber normal schmieren
    static double crossOilIntervalFactor;  //! wieviel mal öfter beim Crossen schmieren
    static double speedProgressionFactor;  //! der Faktor für die Wegstreckenberechnung bei Geschwindigkeit
    static int threshodRainSensor;         //! Schwellenwert für Regen TODO: Hysterese zum Abschalten
    static uint32_t pumpLedLightingTime;   //! Leuchtzeit der Pumpen-LED
    static volatile bool isTachoAction;    //! Marker, Pume anwerfen!
    static fClick lastAction;              //! was hat die Funktionstaste hinterlassen
    static opMode mode;                    //! Betriebsart des Gerätes
#ifdef DEBUG
    static void printPrefs();  //! Preferenzen zeigen
#endif
    Prefs(){};  //! privater Konstruktor, nur statisches Objekt!

    protected:
    static volatile uint32_t tachoPulseCount;          //! aktueller Zähler der impulse (ISR)
    static volatile uint32_t tachoPulseForSpeedCount;  //! aktueller Zähler der impulse (ISR)
    static uint32_t tachoPulseActionOnCount;           //! Fuer ISR der Zeitpunkt für Action
    static uint32_t pulsesPerMeasuredRoute;            //! Wieviele Impulse für Messstrecke
    static uint32_t measuresMsPerRouteMeters;          //! gemessene Mullisekunden für Messstrecke
    static bool functionSwitchDown;                    //! Fuer ISR als Marker ob Knopf unten ist
    static uint32_t lastActionDownTime;                //! seit wann ist der knopf unten
    static uint32_t lastActionUpTime;                  //! sit wann ist der knopf wieder oben

    public:
    friend void ::tachoPulse();      //! Funktion aus HardwareInit.cpp darf zugreifen
    friend void ::functionSwitch();  //! Funktion aus HardwareInit.cpp darf zugreifen
    static bool initPrefs();
    static void setTachoAction( bool );
    static bool getTachoAction();
    static void computeTachoActionCountValue( double );
    static uint32_t getTimeForPumpLedFlash();
    static bool getFunctionSwitchDown();
    static bool getLongClickTimeElapsed();
    static fClick getLastAction();
    static void clearLastAction();
    static void setOpMode( opMode );
    static opMode getOpMode();
    static int getThreshodRainSensor();
    static uint32_t getTachoPulseCount();
    static uint32_t getTachoPulseActionOnCount();
    static uint32_t computeSpeed();             //! berechne Geschwindigkeit in M/S
    static double getSpeedProgressionFactor();  //! gib den progressionsfaktor
    private:
    static void makeDefaults();
  };
}  // namespace Preferences