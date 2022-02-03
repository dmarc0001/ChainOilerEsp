#pragma once
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "AppPreferences.hpp"
#include "ProjectDefaults.hpp"

namespace ChOiler
{
  class WiFiAccessPoint
  {
    private:
    static const char *tag;

    public:
    WiFiAccessPoint();

    private:
    bool wifiInitAp( void );  // NVS muss initialisiert sein
    static void wifiEventHandler( void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data );
    bool shutdownWifi( void );
  };
}  // namespace ChOiler
