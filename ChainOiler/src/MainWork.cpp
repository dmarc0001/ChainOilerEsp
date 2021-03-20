#include "MainWork.hpp"
#include <vector>

namespace ChOiler
{
  const char *MainWorker::tag{"MainWorker"}; //! tag fürs debug logging
  std::list<esp32s2::deltaTimeTenMeters_us> MainWorker::speedList(Prefs::SPEED_HISTORY_LEN);

  void MainWorker::init()
  {
    using namespace Prefs;

    printf("controller ist starting, version %s...\n\n", Preferences::getVersion().c_str());
    MainWorker::speedList.clear();
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
      res = xQueueReceive(esp32s2::EspCtrl::speedQueue, &dtime_us, pdMS_TO_TICKS(15));
      {
        if (res == pdTRUE)
        {
          while (MainWorker::speedList.size() > Prefs::SPEED_HISTORY_LEN - 1)
          {
            // am Ende entfernen
            MainWorker::speedList.pop_back();
          }
          // am Anfang einfuegen
          MainWorker::speedList.push_front(dtime_us);
        }
      }
      //
      // zurückgelegte Wegstrecke berechnen
      //
      res = xQueueReceive(esp32s2::EspCtrl::pathLenQueue, &evt, pdMS_TO_TICKS(50));
      if (res == pdTRUE)
      {
        //
        // wenn in der queue ein ergebnis stand
        //
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
    // Durchschnitt über die letzten 4 Sekunden
    // jeder Zeitstempel ist für 10 Meter Strecke
    // die Durchschnittsgeschwindigkeit ist also max über
    // 10 * Prefs::SPEED_HISTORY_LEN
    //
    uint64_t lastTimeStamp{0ULL};
    float distance_sum = 0.0F;
    float deltaTimeSum_sec = 0.0F;
    float averageSpeed = 0.0;
    int computedCount = 0;
    for (auto it = MainWorker::speedList.begin(); it != MainWorker::speedList.end();)
    {
      //
      // zuerst veraltete Einträge finden und entfernen
      // diff in ms berechnen und merken
      //
      uint64_t logDeltaTimeStamp_ms = (esp_timer_get_time() - *it) >> 10;
      //
      // mehr als Prefs::HISTORY_MAX_TIME_MS Milisekunden her, also veraltet?
      //
      if (logDeltaTimeStamp_ms > Prefs::HISTORY_MAX_TIME_MS)
      {
        // zu alt
        // iterator wird neu gesetzt, alles nach dem gelöscht
        //
        it = MainWorker::speedList.erase(it, MainWorker::speedList.end());
        continue;
      }
      //
      // gibt es einen früheren Zeitstempel auf den ich Bezug nehmen kann?
      //
      if (lastTimeStamp == 0ULL)
      {
        //
        // nein, dann muss ich diesen hier setzten
        //
        lastTimeStamp = *it;
        ++it;
        continue;
      }
      //
      // jetzt die Zeitdifferrenz errechnen
      //
      // logDeltaTimeStamp_ms = (lastTimeStamp - *it) >> 10;
      uint32_t deltaTime_ms = static_cast<uint32_t>((lastTimeStamp - *it) >> 10);
      //
      // jetzt habe ich eine zahl < history ms, weil die zu alten loesche ich oben
      // hier völlig zum schätzen der Geschwindigkeit, Korrekturfaktor ist rund 0.95
      //
      float timeDiff_sec = (static_cast<float>(deltaTime_ms) / 1000.0F) * 0.95F;
      //
      // summiere Wegstrecke und zugehhörige Zeit
      // die Zeit dann als Sekunden / float
      distance_sum += 10.0F;
      deltaTimeSum_sec += timeDiff_sec;
      ++computedCount;
      lastTimeStamp = *it;
      ++it;
    }
    //
    // berechne Durchschnittliche Geschwindigkeit für die letzten Sekunden
    //
    if (deltaTimeSum_sec > 0.001)
    {
      // distance durch zeit...
      ESP_LOGD(tag, "distance: %3.3f m time: %3.6f sec", distance_sum, deltaTimeSum_sec);
      averageSpeed = distance_sum / deltaTimeSum_sec;
    }
    ESP_LOGD(tag, "computed average speed: %03.2f m/s == %03.2f km/h, computed: %03d history entrys...", averageSpeed, averageSpeed * 3.6, computedCount);
  }

} // namespace ChOiler
