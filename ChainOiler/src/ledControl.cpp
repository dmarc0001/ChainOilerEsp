#include "ledControl.hpp"

uint32_t LedControl::lastChanged{ 0 };
uint32_t LedControl::pumpSwitchedOn{ 0 };

void LedControl::loop()
{
  using namespace Preferences;
  static bool controlLedIsOn = false;
  uint32_t timeDiff = millis() - LedControl::pumpSwitchedOn;
  //
  // die Pumpen LED aus, wenn die Zeit ran ist
  //
  if ( Prefs::getOpMode() != opMode::AWAKE )
  {
    if ( digitalRead( LED_PUMP ) == HIGH )
    {
      if ( timeDiff > Prefs::getTimeForPumpLedFlash() )
      {
        digitalWrite( LED_PUMP, LOW );
      }
    }
  }
  //
  // die anderen LED
  //
  timeDiff = millis() - LedControl::lastChanged;
  switch ( Prefs::getOpMode() )
  {
    case opMode::AWAKE:
      if ( controlLedIsOn && timeDiff > BLINK_LED_AWAKE_ON )
      {
        controlLedIsOn = false;
        digitalWrite( LED_RAIN, HIGH );
        digitalWrite( LED_CONTROL, LOW );
        digitalWrite( LED_PUMP, HIGH );
        LedControl::lastChanged = millis();
      }
      else if ( !controlLedIsOn && timeDiff > BLINK_LED_AWAKE_OFF )
      {
        controlLedIsOn = true;
        digitalWrite( LED_RAIN, LOW );
        digitalWrite( LED_CONTROL, HIGH );
        digitalWrite( LED_PUMP, LOW );
        LedControl::lastChanged = millis();
      }
      break;
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

void LedControl::setPumpLED( bool en )
{
  digitalWrite( LED_PUMP, en ? HIGH : LOW );
  LedControl::pumpSwitchedOn = millis();
}

void LedControl::showAttention()
{
  static bool attentionLEDIsOn = false;
  uint32_t timeDiff = millis() - LedControl::lastChanged;

  if ( attentionLEDIsOn && timeDiff > BLINK_LED_ATTENTION_ON )
  {
    attentionLEDIsOn = false;
    digitalWrite( LED_CONTROL, LOW );
    digitalWrite( LED_RAIN, HIGH );
    LedControl::lastChanged = millis();
  }
  else if ( !attentionLEDIsOn && timeDiff > BLINK_LED_ATTENTION_OFF )
  {
    attentionLEDIsOn = true;
    digitalWrite( LED_CONTROL, HIGH );
    digitalWrite( LED_RAIN, LOW );
    LedControl::lastChanged = millis();
  }
}