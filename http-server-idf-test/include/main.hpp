#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

extern uint8_t apSSID[ 32 ];
extern size_t lenSSID;
extern uint8_t apPw[ 64 ];

constexpr uint8_t wifiChannel = 8;
constexpr uint8_t maxWifiConn = 3;

extern "C" void app_main( void );
