#include "main.hpp"

void setup()
{
  breakAction = false;
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
  double ppm = pulsePerRound / weelScope;
  // Speed in meter per sekunde
  double speedMeterPerSec = speed / 3.6;
  // frequenz bei der gegebenen Geschwindigkeit
  double frequence = ppm * speedMeterPerSec * 2.0;
  Serial.print( "speed: " );
  Serial.print( speed );
  Serial.print( " km/h, frequence (*2): " );
  Serial.print( frequence );
  Serial.print( " Hz, count:" );
  Serial.println( g_counter );
  // faktor = 1/5MHz / 1/Freq ==> 5 mhz / freq
  double factor = timerFreq / frequence;
  uint32_t result = static_cast< uint32_t >( floor( factor ) );
  return result;
}

void loop()
{
  static double speed = 0.0;
  static double current_max = static_cast< double >( random( MIN_SPEED + 10, MAX_SPEED ) );
  static double current_min = static_cast< double >( MIN_SPEED );
  static bool isUpwarts = true;
  static bool makeABreak = false;
  static bool isBreakTime = false;

  if ( ( 0x01ffUL & millis() ) == 0 )
  {
    //
    // zufällig die Geschwindigkeit in meinen Grenzen ändern
    //
    if ( breakAction )
    {
      makeABreak = !makeABreak;
      breakAction = false;
      if ( makeABreak )
      {
        Serial.println( "changed: make a break!" );
        // werde langsamer
        isUpwarts = false;
      }
      else
      {
        Serial.print( "changed: end the break, drive on\n" );
        speed = MIN_SPEED;
        if ( isBreakTime )
        {
          timer1_enable( TIM_DIV16, TIM_EDGE, TIM_LOOP );
        }
      }
    }
    double deltaSpeed = static_cast< double >( random( 1, 8 ) ) / 2.51;
    if ( isUpwarts )
    {
      // schneller?
      if ( speed > current_max )
      {
        // zu schnell
        isUpwarts = false;
        current_min = static_cast< double >( random( MIN_SPEED, MAX_SPEED ) );
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
          speed -= deltaSpeed;
        }
        else
        {
          isUpwarts = true;
          current_max = static_cast< double >( random( MIN_SPEED, MAX_SPEED ) );
        }
      }
      else
      {
        speed -= deltaSpeed;
        if ( speed < 0.0 )
        {
          speed = 0.0;
        }
      }
    }
    //
    // Timer programmieren
    //
    if ( speed > 0.01 )
    {
      uint32_t pulses = pulsesForKmh( speed );
      timer1_write( pulses );
    }
    else
    {
      //
      // wen zu langsam, keine Impulse
      //
      if ( !isBreakTime )
      {
        Serial.println( "make the break, no tacho pulses..." );
        timer1_disable();
        digitalWrite( TACHO_OUT, LOW );
        isBreakTime = true;
      }
    }
    //
    // Simulator läuft signalisieren
    //
    if ( !isBreakTime )
    {
      if ( digitalRead( LED_INTERNAL ) == HIGH )
      {
        digitalWrite( LED_INTERNAL, LOW );
      }
      else
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
ICACHE_RAM_ATTR void timerIsr()
{
  volatile static int tacho_state = LOW;
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

ICACHE_RAM_ATTR void functionBreakSwitchIsr()
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
