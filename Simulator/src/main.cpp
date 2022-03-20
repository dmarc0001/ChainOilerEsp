#include "main.hpp"

void setup()
{
  breakAction = false;
  isBreak = false;
  Serial.begin( 115200 );
  Serial.println( "" );
  Serial.println( "controller is starting..." );

  // LED als Ausgang (die interne blaue)
  pinMode( LED_INTERNAL, OUTPUT );
  // den simulierten Tachoumpuls
  pinMode( TACHO_OUT, OUTPUT );
  // break Taste
  pinMode( BREAK_SW_IN, INPUT_PULLUP );
  // LED AUS
  digitalWrite( LED_INTERNAL, HIGH );

  timer1_attachInterrupt( timerIsr );
  attachInterrupt( digitalPinToInterrupt( BREAK_SW_IN ), functionBreakSwitchIsr, CHANGE );

  //
  // 80 MHZ geteilt durch 16 == 5 MHz
  // TIM_EDGE == interrupt muss nicht zurück gesetzt werden
  // TIM_LOOP == kontinuierliches auslösen
  //
  timer1_enable( TIM_DIV16, TIM_EDGE, TIM_LOOP );
  //
  // Voraussetzung:
  //   Radumfang: 1.8 Meter
  //   Geschwindigkeit 50 km/h => 13.88 m/s
  //   => 7,11 Umdrehungen per Sekunde bei 109 Impulsen per Umdrehung
  //   => 775 Impulse per Sekunde bei 50 km/h
  //   => 250 km/h als Max => 3875 Pulse per Sekunde bei 250 km/h
  //   Simulation mit ein/aus erfordert doppelte Frequenz
  //   ====================================
  //   timerinterval = 200 ns
  //   775 impulse * 2 == 1550 Hz
  //   gesuchtes Interval 1/Freq == 645 ys
  //   ==> Faktor 3225
  timer1_write( 1250000 /*pulsesForKmh( 50.0 )*/ );
}

uint32_t pulsesForKmh( double speed )
{
  //
  // Wie viele Impulse per Meter gibt es
  //
  float ppm = pulsePerRound / weelScope;
  // Speed in meter per sekunde
  float speedMeterPerSec = speed / 3.6;
  // frequenz bei der gegebenen Geschwindigkeit
  float frequence = ppm * speedMeterPerSec * 2.0;
  Serial.print( "speed: " );
  Serial.print( speed );
  Serial.print( " km/h, frequence (*2): " );
  Serial.print( frequence );
  Serial.print( " Hz, count:" );
  Serial.println( g_counter );
  // faktor = 1/5MHz / 1/Freq ==> 5 mhz / freq
  float factor = timerFreq / frequence;
  uint32_t result = static_cast< uint32_t >( floor( factor ) );
  return result;
}

void flashLed( uint32_t _len )
{
  digitalWrite( LED_INTERNAL, LOW );  // led an
  delay( _len );
  digitalWrite( LED_INTERNAL, HIGH );  // led an
  delay( _len );
}

void loop()
{
  static float speed = 0.0;
  static float current_max = static_cast< float >( random( MIN_SPEED + 10, MAX_SPEED ) );
  static float current_min = static_cast< float >( MIN_SPEED );
  static bool isUpwarts = true;
  static bool makeABreak = false;

  if ( ( 0x01ffUL & millis() ) == 0 )
  {
    //
    // zufällig die Geschwindigkeit in meinen Grenzen ändern
    //
    if ( breakAction )
    {
      //
      // es gab ein Ereignis
      //
      makeABreak = !makeABreak;
      breakAction = false;
      if ( makeABreak )
      {
        Serial.println( "changed: make a break!" );
        flashLed( 150UL );
        flashLed( 180UL );
        // werde langsamer
        isUpwarts = false;
      }
      else
      {
        Serial.print( "changed: end the break, drive on\n" );
        // Pause beendet
        flashLed( 150UL );
        flashLed( 150UL );
        flashLed( 150UL );
        flashLed( 150UL );
        isBreak = false;
        speed = MIN_SPEED;
        timer1_enable( TIM_DIV16, TIM_EDGE, TIM_LOOP );
      }
    }

    float deltaSpeed = static_cast< float >( random( 1, 8 ) ) / 2.51;
    if ( isUpwarts )
    {
      // schneller?
      if ( speed > current_max )
      {
        // zu schnell
        isUpwarts = false;
        current_min = static_cast< float >( random( MIN_SPEED, MAX_SPEED ) );
      }
      else
      {
        speed += deltaSpeed;
      }
    }
    else
    {
      // langsamer?
      if ( speed < current_min )
      {
        // zu langsam
        if ( makeABreak )
        {
          if ( speed < 0.6 )
          {
            speed = 0.0;
          }
          else
          {
            speed -= deltaSpeed;
          }
        }
        else
        {
          isUpwarts = true;
          current_max = static_cast< float >( random( MIN_SPEED, MAX_SPEED ) );
        }
      }
      else
      {
        // weniger werden
        speed -= deltaSpeed;
      }
    }

    //
    // Timer programmieren
    //
    if ( speed > 0.4 )
    {
      uint32_t pulses = pulsesForKmh( speed );
      timer1_write( pulses );
    }
    else
    {
      //
      // wen zu langsam , keine Impulse
      //
      if ( !isBreak )
      {
        isBreak = true;
        digitalWrite( LED_INTERNAL, LOW );  // led an
        Serial.println( "make the break, no tacho pulses..." );
        timer1_disable();
        digitalWrite( TACHO_OUT, LOW );
      }
    }
    //
    // Simulator läuft signalisieren
    //
    if ( isBreak )
    {
      digitalWrite( LED_INTERNAL, HIGH );  // led aus
      delay( 80 );
      digitalWrite( LED_INTERNAL, LOW );  // led aus
    }
    else
    {
      if ( digitalRead( LED_INTERNAL ) == LOW )
      {
        digitalWrite( LED_INTERNAL, HIGH );
      }
    }
    delay( 2 );
  }
}

/**
 * Time Interrupt Service routine
 */
IRAM_ATTR void timerIsr()
{
  volatile static int tacho_state = LOW;
  if ( isBreak )
    return;
  if ( tacho_state == LOW )
  {
    digitalWrite( TACHO_OUT, HIGH );
    tacho_state = HIGH;
    ++g_counter;
  }
  else
  {
    digitalWrite( TACHO_OUT, LOW );
    tacho_state = LOW;
  }
}

IRAM_ATTR void functionBreakSwitchIsr()
{
  volatile static uint32_t downTime = 0L;
  volatile static bool functionSwitchDown = false;

  if ( digitalRead( BREAK_SW_IN ) == LOW )
  {
    //
    // LOW festhalten
    //
    functionSwitchDown = true;
    downTime = millis();
  }
  else
  {
    //
    // wechsel low->high?
    //
    if ( functionSwitchDown )
    {
      if ( millis() - downTime > 100 )
      {
        // ACTION
        breakAction = true;
      }
      else
      {
        // prellt noch
        downTime = 0L;
      }
      functionSwitchDown = false;
    }
  }
}
