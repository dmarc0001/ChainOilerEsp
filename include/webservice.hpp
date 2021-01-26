#pragma once

#include <DNSServer.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#elif defined( ESP8266 )
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>
#include "ConfigRequestHandler.hpp"

namespace webservice
{
  class OilerWebServer
  {
    private:
    AsyncWebServer *_server;
    bool restartRequired{ false };
    String _id = _getID();

    public:
    void setID( const char *id );
    String getID();
    void loop();

    void begin( AsyncWebServer *_as_server );
    void end();

    private:
    String _getID();
  };
}  // namespace webservice