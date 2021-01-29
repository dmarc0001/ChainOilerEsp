#include "ConfigRequestHandler.hpp"
#include "indexPage.hpp"

namespace webservice
{
  ConfigRequestHandler::ConfigRequestHandler()
  {
  }

  ConfigRequestHandler::~ConfigRequestHandler()
  {
  }

  bool ConfigRequestHandler::canHandle( AsyncWebServerRequest *request )
  {
    // request->addInterestingHeader("ANY");
    return true;
  }

  void ConfigRequestHandler::handleRequest( AsyncWebServerRequest *request )
  {
    AsyncWebServerResponse *response = request->beginResponse_P( 200, "text/html", INDEX_PAGE_CONTENT, INDEX_PAGE_SIZE );
    response->addHeader( "Content-Encoding", "gzip" );
    request->send( response );
  }

}  // namespace webservice