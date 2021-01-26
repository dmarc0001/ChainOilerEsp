#include "ledControl.hpp"

uint32_t LedControl::lastChanged{ 0 };

void LedControl::loop()
{
  static bool controlLedIsOn = false;
  uint32_t timeDiff = millis() - LedControl::lastChanged;

  using namespace Preferences;
  switch ( Prefs::getOpMode() )
  {
    case opMode::NORMAL:
      if ( digitalRead( LED_RAIN ) != LOW )
      {
        digitalWrite( LED_RAIN, LOW );
      }
    case opMode::RAIN:
      if ( controlLedIsOn && timeDiff > BLINK_LED_CONTROL_NORMAL_ON )
      {
        controlLedIsOn = false;
        digitalWrite( LED_CONTROL, LOW );
        LedControl::lastChanged = millis();
      }
      else if ( !controlLedIsOn && timeDiff > BLINK_LED_CONTROL_NORMAL_OFF )
      {
        controlLedIsOn = true;
        digitalWrite( LED_CONTROL, HIGH );
        LedControl::lastChanged = millis();
      }
      break;

    case opMode::CROSS:
      if ( digitalRead( LED_RAIN ) != LOW )
      {
        digitalWrite( LED_RAIN, LOW );
      }
      if ( controlLedIsOn && timeDiff > BLINK_LED_CONTROL_CROSS_ON )
      {
        controlLedIsOn = false;
        digitalWrite( LED_CONTROL, LOW );
        LedControl::lastChanged = millis();
      }
      else if ( !controlLedIsOn && timeDiff > BLINK_LED_CONTROL_CROSS_OFF )
      {
        controlLedIsOn = true;
        digitalWrite( LED_CONTROL, HIGH );
        LedControl::lastChanged = millis();
      }
      break;

    case opMode::APMODE:
      if ( digitalRead( LED_RAIN ) != LOW )
      {
        digitalWrite( LED_RAIN, LOW );
      }
      if ( controlLedIsOn && timeDiff > BLINK_LED_CONTROL_AP_ON )
      {
        controlLedIsOn = false;
        digitalWrite( LED_CONTROL, LOW );
        digitalWrite( LED_RAIN, HIGH );
        LedControl::lastChanged = millis();
      }
      else if ( !controlLedIsOn && timeDiff > BLINK_LED_CONTROL_AP_OFF )
      {
        controlLedIsOn = true;
        digitalWrite( LED_CONTROL, HIGH );
        digitalWrite( LED_RAIN, LOW );
        LedControl::lastChanged = millis();
      }
      break;

    case opMode::TEST:
      if ( digitalRead( LED_RAIN ) != LOW )
      {
        digitalWrite( LED_RAIN, LOW );
      }
      if ( controlLedIsOn && timeDiff > BLINK_LED_CONTROL_TEST_ON )
      {
        controlLedIsOn = false;
        digitalWrite( LED_CONTROL, LOW );
        LedControl::lastChanged = millis();
      }
      else if ( !controlLedIsOn && timeDiff > BLINK_LED_CONTROL_TEST_OFF )
      {
        controlLedIsOn = true;
        digitalWrite( LED_CONTROL, HIGH );
        LedControl::lastChanged = millis();
      }
      break;
  }
}

void LedControl::setRainLED( bool en )
{
  digitalWrite( LED_RAIN, en ? HIGH : LOW );
}
