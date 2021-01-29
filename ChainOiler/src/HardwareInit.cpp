#include "HardwareInit.hpp"
#include "Prefs.hpp"
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
  pinMode( PUMP_CONTROL, OUTPUT );
  digitalWrite( LED_INTERNAL, HIGH );  // ==> AUS
  digitalWrite( LED_CONTROL, LOW );
  digitalWrite( LED_RAIN, LOW );
  digitalWrite( LED_PUMP, LOW );
  digitalWrite( PUMP_CONTROL, LOW );
  //
  // Eingänge nitialisieren
  //
  pinMode( INPUT_TACHO, INPUT );
  pinMode( INPUT_FUNCTION_SWITCH, INPUT_PULLUP );
  pinMode( INPUT_RAIN_SWITCH, INPUT );
  //
  // Interrupt für Eingänge
  //
  // Tachoimpuls
  attachInterrupt( digitalPinToInterrupt( INPUT_TACHO ), tachoPulse, FALLING );
  // Funktionstaste
  attachInterrupt( digitalPinToInterrupt( INPUT_FUNCTION_SWITCH ), functionSwitch, CHANGE );
  // Timer für die Pumle
  timer1_attachInterrupt( timerIsr );
  //
  // 80 MHZ geteilt durch 256 == 31.25 kHz (Uhrquarz?)
  // TIM_EDGE == interrupt muss nicht zurück gesetzt werden
  // TIM_LOOP == kontinuierliches auslösen
  //
  timer1_enable( TIM_DIV256, TIM_EDGE, TIM_SINGLE );
  // 20 ms
  timer1_write( 625 );
}

/**
 * ISR für den Tachoimpuls
 */
ICACHE_RAM_ATTR void tachoPulse()
{
  using namespace Preferences;
  ++Prefs::tachoPulseCount;
  {
    if ( Prefs::tachoPulseCount > Prefs::tachoPulseActionOnCount )
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

/**
 * Time Interrupt Service routine
 * schaltet die Pumpe LOW
 */
ICACHE_RAM_ATTR void timerIsr()
{
  digitalWrite( Preferences::PUMP_CONTROL, LOW );
}