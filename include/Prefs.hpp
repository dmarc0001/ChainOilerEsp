#pragma once
#include <Arduino.h>
#include "ProjectDefaults.hpp"

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
    static volatile bool isTachoAction;
    static fClick lastAction;
    static opMode mode;
    Prefs(){};

    public:
    static volatile uint32_t tachoPulseCount;
    static uint32_t tachoPulseActionOn;
    static bool functionSwitchDown;
    static uint32_t lastActionDownTime;
    static uint32_t lastActionUpTime;

    static void initPrefs();
    static void setTachoAction( bool );
    static bool getTachoAction();
    static bool getFunctionSwitchDown();
    static fClick getLastAction();
    static void clearLastAction();
    static void setOpMode( opMode );
    static opMode getOpMode();

    private:
    static void makeDefaults();
  };
}  // namespace Preferences