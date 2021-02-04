
#include "main.hpp"

//
// lokale Server instsanzieren
//
DNSServer dnsServer;
AsyncWebServer asServer( 80 );
webservice::OilerWebServer oilServer;

void setup()
{
  Serial.begin( 115200 );
  Serial.println( "" );
  Serial.println( "controller is starting..." );
  //
  // Einstellungen lesen, Preferenzen initialisieren
  //
#ifdef DEBUG
  Serial.println( "initializing preferences..." );
#endif
  Preferences::Prefs::initPrefs();
#ifdef DEBUG
  Serial.println( "initializing prferences...OK" );
#endif
  //
  // Hardware initialisieren
  //
#ifdef DEBUG
  Serial.println( "initializing hardware..." );
#endif
  initHardware();
#ifdef DEBUG
  Serial.println( "initializing hardware...OK" );
#endif
  Prefs::setOpMode( opMode::NORMAL );
  Prefs::computeTachoActionCountValue( 1.0 );
}

void loop()
{
  using namespace Preferences;
  //
  // ÖLPUMPE erforderlich?
  //
  checkTachoActions();
  //
  // REGENSENSOR
  // alle paar Sekunden erfragen
  // warte immer etwa 4 Sekunden
  //
  checkRainSensor();
  //
  // Funktionstaste
  //
  if ( Prefs::getLastAction() != fClick::NONE )
  {
    checkControlKey();
  }
  else
  {
    //
    // bei fClick::NONE
    //
    if ( Prefs::getLongClickTimeElapsed() )
    {
      LedControl::showAttention();
    }
    else
    {
      //
      // noch das verchiedentliche LED gedingse
      //
      LedControl::loop();
    }
  }
  if ( opMode::NORMAL == Prefs::getOpMode() )
  {
    //
    // im normalen Mode mache ich Anpassung an die Geschwindigkeit
    //
    checkSpeedActions();
  }
  //
  // Kontrolle ob der Webservive und WiFi gestartet werden sollen
  //
  checkStartStopWLANService();
}

/**
 * Tachoaktionen machen
 */
void checkTachoActions()
{
  static Preferences::opMode lastMode = opMode::NORMAL;

  if ( Prefs::getTachoAction() )
  {
    //
    // TODO: Pumpe auslösen
    //
    Serial.println( "oil pump action... " );
    LedControl::setPumpLED( true );
    //
    // Pumpeminmuls starten
    //
    digitalWrite( Preferences::PUMP_CONTROL, HIGH );
    timer1_write( 625 );  // 20 ms
    //
    Prefs::setTachoAction( false );
  }
  //
  // gab es eine Änderung?
  //
  if ( Prefs::getOpMode() == lastMode )
  {
    //
    // nein, habe fertig
    //
    return;
  }
  //
  // Lasse daten für neuen Zustand berechnen
  //
  Prefs::computeTachoActionCountValue( 1.0 );
  // merken
  lastMode = Prefs::getOpMode();
}

/**
 * Regnet es?
 */
void checkRainSensor()
{
  static int oldSensorValue;
  int rainSensorValue;
  //
  if ( ( 0x0fffUL & millis() ) == 0 )
  {
    Serial.print( "TACHO " );
    Serial.print( Prefs::getTachoPulseCount() );
    Serial.print( " to: " );
    Serial.println( Prefs::getTachoPulseActionOnCount() );
    rainSensorValue = analogRead( INPUT_ANALOG );
    if ( rainSensorValue == oldSensorValue )
    {
      delay( 5 );
      return;
    }
    oldSensorValue = rainSensorValue;
    //
    if ( rainSensorValue > Prefs::getThreshodRainSensor() )
    {
      //
      // nur bei NORMAL, die anderen Modi haben Vorrang
      //
      if ( Prefs::getOpMode() == opMode::NORMAL )
      {
        Prefs::setOpMode( opMode::RAIN );
        LedControl::setRainLED( true );
      }
    }
    else if ( Prefs::getOpMode() == opMode::RAIN )
    {
      Prefs::setOpMode( opMode::NORMAL );
    }
#ifdef DEBUG
    Serial.print( "analog value (rain sensor): " );
    Serial.print( rainSensorValue );
    Serial.println( " from max 1023" );
#endif
    delay( 5 );
  }
}

/**
 * Was passierte an der Taste?
 */
void checkControlKey()
{
  // auswerten
  if ( Prefs::getLastAction() == fClick::SHORT )
  {
    // Kurzer Taster -> Cross Aktiv / deaktiv
#ifdef DEBUG
    Serial.println( "short func key click..." );
#endif
    if ( Prefs::getOpMode() == opMode::CROSS || Prefs::getOpMode() == opMode::APMODE )
    {
      Prefs::setOpMode( opMode::NORMAL );
    }
    else
    {
      Prefs::setOpMode( opMode::CROSS );
    }
    //
    // zurücksetzten, da ausgewertet
    //
    Prefs::clearLastAction();
  }
  else if ( Prefs::getLastAction() == fClick::LONG )
  {
#ifdef DEBUG
    Serial.println( "long func key click..." );
#endif
    //
    // WLAN umschalten
    //
    if ( Prefs::getOpMode() == opMode::APMODE )
    {
      Prefs::setOpMode( opMode::NORMAL );
    }
    else
    {
      Prefs::setOpMode( opMode::APMODE );
    }
    //
    // zurücksetzten, da ausgewertet
    //
    Prefs::clearLastAction();
  }
}

/**
 * Prüfe ob was mit der Geschwindigkeit zu machen ist
 */
void checkSpeedActions()
{
  static double oldSpeed = 0.0;
  // so alle Sekunde
  if ( ( 0x03ffUL & millis() ) == 0 )
  {
    // wie schnell sind wir
    double currSpeed = Prefs::computeSpeed();
    // wie gross ist die Differenz zu eben
    int diff = abs( static_cast< int >( currSpeed - oldSpeed ) );
    if ( diff > 5 )
    {
      // Berechne die Progression
      // y = x * Factor + 120
      //
      double fact = ( ( currSpeed * Prefs::getSpeedProgressionFactor() ) + 120.0 ) / 100.0;
#ifdef DEBUG
      Serial.print( "progression speed factor: " );
      Serial.println( fact );
#endif
      Prefs::computeTachoActionCountValue( fact );
    }
  }
}

/**
 * starten oder stoppen der WLAN Dienste?
 */
void checkStartStopWLANService()
{
  static bool runWebservice{ false };

  if ( Prefs::getOpMode() == opMode::APMODE && !runWebservice )
  {
    //
    // WiFi und Webservice starten
    //
#ifdef DEBUG
    Serial.println( "start accesspoint..." );
#endif
    WiFi.softAP( "chainoiler" );
    Serial.print( "AP IP: " );
#ifdef DEBUG
    Serial.println( WiFi.softAPIP() );
    Serial.println( " dns service is starting..." );
#endif
    dnsServer.start( 53, "*", WiFi.softAPIP() );
    oilServer.begin( &asServer );
    runWebservice = true;
    delay( 50 );
  }
  //
  // läuft der Service
  //
  if ( runWebservice )
  {
    dnsServer.processNextRequest();
    oilServer.loop();
  }
  //
  // Service beenden
  //
  if ( runWebservice && Prefs::getOpMode() != opMode::APMODE )
  {
    //
    // der Service soll beendet werden
    //
#ifdef DEBUG
    Serial.println( "stop accesspoint..." );
#endif
    dnsServer.stop();
    oilServer.end();
    WiFi.softAPdisconnect();
    runWebservice = false;
  }
}