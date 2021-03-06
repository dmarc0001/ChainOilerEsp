#include "MainWork.hpp"
#include <vector>

namespace ChOiler
{
  const char *MainWorker::tag{"MainWorker"}; //! tag fürs debug logging
  std::vector<esp32s2::deltaTimeTenMeters_us> MainWorker::speedVec(Prefs::SPEED_HISTORY_LEN);

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
    ESP_LOGD(tag, "init done.");
  }

  void MainWorker::run()
  {
    ESP_LOGD(tag, "%s: loop start...", __func__);
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
    esp32s2::deltaTimeTenMeters_us dtime_us;
    int16_t count = 0;
    portBASE_TYPE res;

    bool computed = false;

    while (true)
    {
      //
      // Geschwindigkeitsdaten aus der Queue in den Speed-History-Buffer
      // vector wie queue benutzern, aber ich kann std::queue nicht nehmen
      // da ich wahlfrei zugriff haben will
      //
      res = xQueueReceive(esp32s2::EspCtrl::speedQueue, &dtime_us, pdMS_TO_TICKS(5));
      {
        if (res == pdTRUE)
        {
          if (MainWorker::speedVec.size() > Prefs::SPEED_HISTORY_LEN - 1)
          {
            // am Ende entfernen
            MainWorker::speedVec.pop_back();
          }
          // am Anfang einfuegen
          MainWorker::speedVec.insert(MainWorker::speedVec.begin(), dtime_us);
        }
      }

      //
      // zurückgelegte Wegstrecke berechnen
      //
      res = xQueueReceive(esp32s2::EspCtrl::pathLenQueue, &evt, pdMS_TO_TICKS(50));
      if (res == pdTRUE)
      {
        pcnt_get_counter_value(PCNT_UNIT_0, &count);
        ESP_LOGI(tag, "Event 100 meters path done: unit%d; cnt: %d", evt.unit, evt.value);
      }

      //
      // ungefähr alle 2 Sekunden Berechnen
      //
      if ((esp_timer_get_time() & 0x1f0000) == 0)
      {
        if (!computed)
        {
          computed = true;
          MainWorker::computeAvgSpeed();
        }
        else
        {
          if (computed)
            computed = false;
        }
      }
    }
  }

  void MainWorker::computeAvgSpeed()
  {
    //
    // ungefähr alle 2 Sekunden Berechnen
    //
    uint64_t timeNow = esp_timer_get_time();

    //
    // Durchschnitt über die letzten 4 Sekunden
    // jeder Zeitstempel ist für 10 Meter Strecke
    // die Durchschnittsgeschwindigkeit ist also max über
    // 10 * Prefs::SPEED_HISTORY_LEN
    //
    float distance = 0UL;
    float deltaTimeSum = 0UL;
    for (auto it = MainWorker::speedVec.begin(); it != MainWorker::speedVec.end();)
    {
      //
      // aelter als 4 Sekunden?
      //
      uint64_t timeDiff_us = timeNow - *it;
      if (timeDiff_us > (4 * 1000000))
      {
        it = MainWorker::speedVec.erase(it, MainWorker::speedVec.end());
        continue;
      }
      //
      // summiere Wegstrecke und zugehhörige Zeit
      //
      distance += 10.0;
      deltaTimeSum += static_cast<float>(timeDiff_us / 1000000.0);
      ++it;
    }
    float averageSpeed = 0.0;
    //
    // berechne Durchschnittliche Geschwindigkeit für die letzten Sekunden
    //
    if (deltaTimeSum > 0.1)
    {
      averageSpeed = distance / deltaTimeSum;
    }
    ESP_LOGD(tag, "computed average speed: %03.2f m/s == %03.2f km/h, vector count: %03d...", averageSpeed, averageSpeed * 3.6, speedVec.size());
  }

} // namespace ChOiler
