#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"

#include "main.hpp"

class WiFiStuff
{
  private:
  static const char *tag;

  public:
  static void initWiFi();

  private:
  explicit WiFiStuff(){};
  static void evtHandler( void *, esp_event_base_t, int32_t, void * );
};
