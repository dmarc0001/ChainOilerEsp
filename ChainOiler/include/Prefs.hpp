#pragma once
#include <Arduino.h>
#include "FS.h"
#include "HardwareInit.hpp"
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
    static double pulsePerWeelRound;       //! Defaultwert für Reed, Honda Africa Twin 109
    static double weelCircumFerence;       //! Default Umfang Hinterrad
    static double normalOilInterval;       //! Schmierinterval 4000 Meter
    static double rainOilIntervalFactor;   //! wieviel mal gegenüber normal schmieren
    static double crossOilIntervalFactor;  //! wieviel mal öfter beim Crossen schmieren
    static int threshodRainSensor;         //! Schwellenwert für Regen TODO: Hysterese zum Abschalten
    static uint32_t pumpLedLightingTime;   //! Leuchtzeit der Pumpen-LED
    static volatile bool isTachoAction;    //! Marker, Pume anwerfen!
    static fClick lastAction;              //! was hat die Funktionstaste hinterlassen
    static opMode mode;                    //! Betriebsart des Gerätes
    Prefs(){};                             //! privater Konstruktor, nur statisches Objekt!

    protected:
    static volatile uint32_t tachoPulseCount;  //! aktueller Zähler der impulse (ISR)
    static uint32_t tachoPulseActionOnCount;   //! Fuer ISR der Zeitpunkt für Action
    static bool functionSwitchDown;            //! Fuer ISR als Marker ob Knopf unten ist
    static uint32_t lastActionDownTime;        //! seit wann ist der knopf unten
    static uint32_t lastActionUpTime;          //! sit wann ist der knopf wieder oben

    public:
    friend void ::tachoPulse();      //! Funktion aus HardwareInit.cpp darf zugreifen
    friend void ::functionSwitch();  //! Funktion aus HardwareInit.cpp darf zugreifen
    static bool initPrefs();
    static void setTachoAction( bool );
    static bool getTachoAction();
    static void computeTachoActionCountValue();
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

    private:
    static void makeDefaults();
  };
}  // namespace Preferences