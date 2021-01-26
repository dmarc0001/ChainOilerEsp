
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
}

void loop()
{
  using namespace Preferences;
  static bool runWebservice{ false };

  int rainSensorValue;
  //
  // ÖLPUMPE erforderlich?
  //
  if ( Prefs::getTachoAction() )
  {
    //
    // TODO: Pumpe auslösen
    //
    Prefs::setTachoAction( false );
  }
  //
  // Funktionstaste
  //
  if ( Prefs::getLastAction() != fClick::NONE )
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

  //
  // Kontrolle ob der Webservive und WiFi gestartet werden sollen
  //
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

  //
  // REGENSENSOR
  // alle paar Sekunden erfragen
  // warte immer etwa 2 Sekunden
  //
  if ( ( 0x07ffUL & millis() ) == 0 )
  {
    rainSensorValue = analogRead( INPUT_ANALOG );
    if ( rainSensorValue > threshodRainSensor )
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
    Serial.print( "analog value: " );
    Serial.print( rainSensorValue );
    Serial.println( " from max 1023" );
#endif
    /*
        digitalWrite( LED_INTERNAL, LOW );
        delay( 50 );
        digitalWrite( LED_INTERNAL, HIGH );
        */
  }
  //
  // noch das verchiedentliche LED gedingse
  //
  LedControl::loop();
}