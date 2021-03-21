#include "MainWork.hpp"

namespace ChOiler
{
  const char *MainWorker::tag{"MainWorker"}; //! tag fürs debug logging
  std::list<esp32s2::deltaTimeTenMeters_us> MainWorker::speedList(Prefs::SPEED_HISTORY_LEN);

  /**
   * @brief initialisiere das Programm
   * 
   */
  void MainWorker::init()
  {
    using namespace Prefs;

    printf("controller ist starting, version %s...\n\n", Preferences::getVersion().c_str());
    MainWorker::speedList.clear();
    Preferences::setAppMode(opMode::AWAKE);
    //
    // lese die Einstellungen aus dem NVM
    //
    Preferences::init();
    //
    // initialisiere die Hardware
    //
    gpio_install_isr_service(0); // gloabal einmalig für GPIO
    esp32s2::ButtonControl::init();
    esp32s2::LedControl::init();
    esp32s2::PumpControl::init();
    esp32s2::TachoControl::init();
    esp32s2::RainSensorControl::init();
    ESP_LOGD(tag, "init done.");
  }

  /**
   * @brief Hauptschleife des Programmes
   * 
   */
  void MainWorker::run()
  {
    using namespace Prefs;

    uint64_t runTime = esp_timer_get_time() + 1500000ULL;
    bool computed = false;
    //
    ESP_LOGI(tag, "%s: run start...", __func__);
    //
    // das Startsignal leuchten
    //
    while (esp_timer_get_time() < runTime)
    {
      esp32s2::LedControl::showAttention();
      vTaskDelay(1);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    esp32s2::LedControl::allOff();
    Preferences::setAppMode(opMode::NORMAL);
    //
    // hier geth es dann richtig los
    // für immer :-)
    //
    ESP_LOGD(tag, "%s: loop start...", __func__);
    while (true)
    {
      switch (Preferences::getAppMode())
      {
      case opMode::NORMAL:
        MainWorker::tachoCompute();
        MainWorker::buttonStati();
        break;

      case opMode::CROSS:
      case opMode::RAIN:
      case opMode::TEST:
      case opMode::APMODE:
      default:
        break;
      }

      //
      // ungefähr alle 2 Sekunden Berechnen
      // und wenn nicht AP Mode
      //
      if (((esp_timer_get_time() & 0x1f0000) == 0) &&
          Preferences::getAppMode() != opMode::APMODE)
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

  /**
   * @brief berechne Durchschnittsgeschwindigkeit der letzten Sekunden
   * 
   */
  void MainWorker::tachoCompute()
  {
    using namespace esp32s2;

    pcnt_evt_t evt;
    deltaTimeTenMeters_us dtime_us;
    portBASE_TYPE res;
    //int16_t count = 0;
    //
    // Geschwindigkeitsdaten aus der Queue in den Speed-History-Buffer
    // vector wie queue benutzern, aber ich kann std::queue nicht nehmen
    // da ich wahlfrei zugriff haben will
    //
    res = xQueueReceive(TachoControl::speedQueue, &dtime_us, pdMS_TO_TICKS(15));
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
    res = xQueueReceive(TachoControl::pathLenQueue, &evt, pdMS_TO_TICKS(50));
    if (res == pdTRUE)
    {
      //
      // wenn in der queue ein ergebnis stand
      //
      //pcnt_get_counter_value(PCNT_UNIT_0, &count);
      ESP_LOGI(tag, "Event %d meters path done: unit%d; cnt: %d", evt.meters, evt.unit, evt.value);
      Prefs::Preferences::addRouteLenPastOil(evt.meters);
    }
  }

  /**
   * @brief prüfe ob sich bei den tasten etwas getan hat
   * 
   */
  void MainWorker::buttonStati()
  {
    using namespace esp32s2;
    using namespace Prefs;

    if (ButtonControl::controlDownSince() > 0ULL)
    {
      //
      // contol blinken
      //
    }
    if (Preferences::getControlSwitchAction() != fClick::NONE)
    {
      if (Preferences::getControlSwitchAction() == fClick::SHORT)
      {
        ESP_LOGI(tag, "CONTROL Button short down");
        // Crossmode hin und her schalten
      }
      else if (Preferences::getControlSwitchAction() == fClick::LONG)
      {
        ESP_LOGI(tag, "CONTROL Button long down");
      }
      Preferences::setControlSwitchAction(fClick::NONE);
    }

    if (Preferences::getRainSwitchAction() != fClick::NONE)
    {
      //
      // Was ist passiert? Level 0 bedeutet Knopf gedrückt
      //
      ESP_LOGI(tag, "RAIN  Button down");
      // TODO: was damit anstellen
      Preferences::setRainSwitchAction(fClick::NONE);
    }
  }

  /**
   * @brief Berechne Durchschnittsgeschwindigkeit der letzten Prefs::HISTORY_MAX_TIME_MS Microsekunden
   * 
   */
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
      Prefs::Preferences::setCurrentSpeed(averageSpeed);
    }
    ESP_LOGD(tag, "computed average speed: %03.2f m/s == %03.2f km/h, computed: %03d history entrys...", averageSpeed, averageSpeed * 3.6, computedCount);
  }

  /**
   * gehe in den Tiefen Schlaf
   */
  void MainWorker::goDeepSleep()
  {
    //
    // Tachoimpuls soll wechen
    //
    esp_sleep_enable_ext0_wakeup(Prefs::INPUT_TACHO, 1); // 1 == HIGH
    ESP_LOGD(tag, "%s: controller is going to deep sleep...", __func__);
    printf("deep sleep...");
    Prefs::Preferences::close();
    for (int i = 5; i > 0; --i)
    {
      ESP_LOGD(tag, "%s: sleep in %02d secounds...", __func__, i);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }
    esp32s2::LedControl::allOff();
    printf("..Good night.\n");
    //
    // Wiederbelebung erst durch Tachoimpuls
    //
    esp_deep_sleep_start();
  }

} // namespace ChOiler
