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
  pinMode( INPUT_TACHO, INPUT );
  pinMode( INPUT_FUNCTION_SWITCH, INPUT );
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

  static bool switchDown = false;
  static uint32_t last = 0;
  //
  // digital entprellen...
  //
  int newval = digitalRead( INPUT_FUNCTION_SWITCH );
  if ( newval == LOW )
  {
    // gedrückt, oder prellt noch
    if ( last == 0 )
    {
      // zum ersten mal
      last = millis();
      return;
    }
    if ( !switchDown )
    {
      if ( ( millis() - last ) > deBounceTimeMs )
      {
        //
        // okay sollte das prellen erledigt haben
        // markiere das der schalter gedrückt ist
        //
        switchDown = true;
        Prefs::functionSwitchDown = true;
      }
      else
      {
        //
        // neu anfangen und warten dass das stabil bleibt
        //
        last = millis();
      }
    }
    return;
  }
  //
  // Okay dann ist es HIGH
  // Losgelassen (oder prellt noch)
  //
  if ( switchDown )
  {
    //
    // war schalter gedrückt akzeptiert?
    //
    if ( ( millis() - last ) > longClickTimeMs )
    {
      //
      // war länger als longClickTimeMs -> Funktion WiFi aktiviert
      //
      Prefs::setLastAction( fClick::LONG );
    }
    else
    {
      //
      //
    }
    Prefs::setLastAction( fClick::SHORT );
    switchDown = false;
    Prefs::functionSwitchDown = false;
  }
}
