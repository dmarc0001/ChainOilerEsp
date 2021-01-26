
#include "main.hpp"

//
// wird durch Schaltung von aussen der Webservice angefordert
//
volatile bool isWebService{ false };
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
  // Hardware initialisieren
  //
#ifdef DEBUG
  Serial.println( "initializing hardware..." );
#endif
  initHardware();
#ifdef DEBUG
  Serial.println( "initializing hardware...OK" );
#endif
}

void loop()
{
  using namespace Preferences;
  static bool runWebservice{ false };
  static volatile unsigned long lastTimer = 0;

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

      //
      // zurücksetzten, da ausgewertet
      //
      Prefs::setLastAction( fClick::NONE );
    }
    else if ( Prefs::getLastAction() == fClick::LONG )
    {
      //
      // WLAN umschalten
      //
      isWebService = !isWebService;
      //
      // zurücksetzten, da ausgewertet
      //
      Prefs::setLastAction( fClick::NONE );
    }
  }

  //
  // Kontrolle ob der Webservive und WiFi gestartet werden sollen
  //
  if ( isWebService && !runWebservice )
  {
    //
    // WiFi und Webservice starten
    //
    WiFi.softAP( "chainoiler" );
    Serial.print( "AP IP: " );
    Serial.println( WiFi.softAPIP() );
    Serial.println( " dns service is starting..." );
    dnsServer.start( 53, "*", WiFi.softAPIP() );
    oilServer.begin( &asServer );
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
  if ( runWebservice && !isWebService )
  {
    //
    // der Service soll beendet werden
    //
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
#ifdef DEBUG
    Serial.print( "analog value: " );
    Serial.print( rainSensorValue );
    Serial.println( " from max 1023" );
#endif
    digitalWrite( LED_INTERNAL, LOW );
    delay( 50 );
    digitalWrite( LED_INTERNAL, HIGH );
  }
}