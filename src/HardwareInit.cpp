#include "HardwareInit.hpp"
#include "ProjectDefaults.hpp"

void initHardware()
{
  using namespace Preferences;
  //
  // Ausgänge initialisieren
  //
  pinMode( LED_INTERNAL, OUTPUT );
  pinMode( LED_CONTROL, OUTPUT );
  pinMode( LED_RAIN, OUTPUT );
  pinMode( LED_PUMP, OUTPUT );
  digitalWrite( LED_INTERNAL, HIGH );  // ==> AUS
  digitalWrite( LED_CONTROL, LOW );
  digitalWrite( LED_RAIN, LOW );
  digitalWrite( LED_PUMP, LOW );
  //
  // Eingänge nitialisieren
  //
  pinMode( INPUT_TACHO, INPUT_PULLUP );
  pinMode( INPUT_FUNCTION_SWITCH, INPUT_PULLUP );
  pinMode( INPUT_RAIN_SWITCH, INPUT );
  //
  // Interrupt für Eingänge
  //
  // Tachoimpuls
  attachInterrupt( digitalPinToInterrupt( INPUT_TACHO ), tachoPulse, FALLING );
  // Funktionstaste
  attachInterrupt( digitalPinToInterrupt( INPUT_FUNCTION_SWITCH ), functionSwitch, CHANGE );
}

/**
 * ISR für den Tachoimpuls
 */
ICACHE_RAM_ATTR void tachoPulse()
{
  using namespace Preferences;

  ++Prefs::tachoPulseCount;
  {
    if ( Prefs::tachoPulseCount == Prefs::tachoPulseActionOn )
    {
      Prefs::tachoPulseCount = 0L;
      Prefs::setTachoAction( true );
    }
  }
}

ICACHE_RAM_ATTR void functionSwitch()
{
  using namespace Preferences;
  //
  // digital entprellen macht Prefs mit den Zeitstempeln
  //
  if ( digitalRead( INPUT_FUNCTION_SWITCH ) == LOW )
  {
    //
    // LOW festhalten
    //
    Prefs::functionSwitchDown = true;
    Prefs::lastActionDownTime = millis();
  }
  else
  {
    //
    // HIGH festhalten
    //
    Prefs::lastActionUpTime = millis();
    Prefs::functionSwitchDown = false;
  }
}
