#include "MainWork.hpp"

namespace ChOiler
{
  const char *MainWorker::tag{"MainWorker"}; //! tag fürs debug logging

  void MainWorker::init()
  {
    using namespace Prefs;

    printf("controller ist starting, version %s...\n\n", Preferences::getVersion().c_str());
    //
    // lese die Einstellungen aus dem NVM
    //
    Preferences::init();
    //
    // initialisiere die Hardware
    //
    esp32s2::EspCtrl::init();
  }

  void MainWorker::run()
  {
    //
    // für immer :-)
    //
    while (true)
    {
      //
      // switch mode, ->mode normal....
      //
      MainWorker::defaultLoop();
    }
  }

  void MainWorker::defaultLoop()
  {
    //
    // test der Funktionen
    //
    esp32s2::pcnt_evt_t evt;
    int16_t count = 0;
    portBASE_TYPE res;

    while (true)
    {
      /*
      vTaskDelay(1500 / portTICK_PERIOD_MS);
      vPortYield();
      if (uxQueueMessagesWaiting(esp32s2::EspCtrl::pcnt_evt_queue) > 0)
      {
        ESP_LOGD(tag, "events in queue: %02d!", uxQueueMessagesWaiting(esp32s2::EspCtrl::pcnt_evt_queue));
      }
     */
      res = xQueueReceive(esp32s2::EspCtrl::pcnt_evt_queue, &evt, 200 / portTICK_PERIOD_MS);
      if (res == pdTRUE)
      {
        pcnt_get_counter_value(PCNT_UNIT_0, &count);
        ESP_LOGI(tag, "Event 100 meters path done: unit%d; cnt: %d", evt.unit, evt.value);
      }
    }
  }

} // namespace ChOiler
