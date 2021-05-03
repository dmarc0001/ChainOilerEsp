#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/pcnt.h>
#include <esp_err.h>
#include "ProjectDefaults.hpp"

namespace esp32s2
{
  class PumpControl
  {
    private:
    static const char *tag;
    static PumpControl inst;
    esp_timer_handle_t timerHandle;
    volatile bool pumpIsOn;

    public:
    static PumpControl *getInstance();
    ~PumpControl();
    void stop();
    void start();
    PumpControl( PumpControl const & ) = delete;
    void operator=( PumpControl const & ) = delete;
    static void timerCallback( void * );

    private:
    PumpControl();
  };
}  // namespace esp32s2
