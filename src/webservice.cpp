#include "webservice.hpp"

namespace webservice
{
  void OilerWebServer::begin( AsyncWebServer *_as_server )
  {
    _server = _as_server;
    //
    // implement handlers
    //
    _server->addHandler( new ConfigRequestHandler() ).setFilter( ON_AP_FILTER );  // only when requested from AP
    _server->begin();
  }

  void OilerWebServer::end()
  {
    _server->end();
  }

  /**
   * ID des Servers setzen
   */
  void OilerWebServer::setID( const char *id )
  {
    _id = id;
  }

  /**
   * ID des Webservers erfragen
   */
  String OilerWebServer::getID()
  {
    return _id;
  }

  /**
   * internes erzeugen der éindeutigen ID
   */
  String OilerWebServer::_getID()
  {
    String id = "API-";
#if defined( ESP8266 )
    id += String( ESP.getChipId() );
#elif defined( ESP32 )
    id += String( ( uint32_t ) ESP.getEfuseMac(), HEX );
#endif
    id.toUpperCase();
    return id;
  }

  /**
   * zyklische Frage ob ewas getan werden muss
   */
  void OilerWebServer::loop()
  {
    if ( restartRequired )
    {
      yield();
      delay( 1000 );
      yield();
#if defined( ESP8266 )
      ESP.restart();
#elif defined( ESP32 )
      // ESP32 will commit sucide
      esp_task_wdt_init( 1, true );
      esp_task_wdt_add( NULL );
      while ( true )
        ;
#endif
    }
  }
}  // namespace webservice