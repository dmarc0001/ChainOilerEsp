#pragma once

#include <ESPAsyncWebServer.h>
//#include "indexPage.hpp"

namespace webservice
{
  class ConfigRequestHandler : public AsyncWebHandler
  {
    public:
    ConfigRequestHandler();
    virtual ~ConfigRequestHandler();

    bool canHandle( AsyncWebServerRequest *request );

    void handleRequest( AsyncWebServerRequest *request );
  };
}  // namespace webservice