#include "MainWork.hpp"

namespace ChOiler
{
  /**
   * @brief instanzieren und initialisieren der stsatischen Variablen
   *
   */
  const char *MainWorker::tag{"MainWorker"};                                    //! tag fürs debug logging
  esp_sleep_wakeup_cause_t MainWorker::wakeupCause{ESP_SLEEP_WAKEUP_UNDEFINED}; //! startgrund
  std::list<esp32s2::deltaTimeTenMeters_us> MainWorker::speedList;              //! erzeuge leere Liste
  WiFiAccessPoint MainWorker::AccessPoint;                                      //! statisches Objekt
  constexpr uint64_t timeForMessage = 3500000ULL;                               //! Zeit zur nächsten Nachricht
  constexpr uint64_t timeForSpeedMeasure = 2000000ULL;                          //! zeit bis zur Geschwindigkeitsmessung
  constexpr uint64_t timeForOilerCheck = 3200000ULL;                            //! zeit bis zur Kontrole ob Geölt werden muss

  /**
   * @brief initialisiere das Programm
   *
   */
  void MainWorker::init()
  {
    using namespace Prefs;

    printf("controller ist starting, version %s...\n\n", Preferences::getVersion().c_str());
    //
    // warum geweckt/resettet
    //
    ESP_LOGD(tag, "===================================================================");
    MainWorker::processStartupCause();
    ESP_LOGD(tag, "===================================================================\n\n");
    // 100 ms warten
    vTaskDelay(pdMS_TO_TICKS(100));
    //
    MainWorker::speedList.clear();
    Preferences::setAppMode(opMode::AWAKE);
    //
    // lese die Einstellungen aus dem NVM
    //
    Preferences::init();
    //
    // gloabal einmalig für GPIO IST Services einschalten
    //
    gpio_install_isr_service(0);
    //
    // initialisiere die Hardware
    //
    //
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

    uint64_t computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
    uint64_t computeOilerCheckTime = esp_timer_get_time() + timeForOilerCheck;
    uint64_t ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_NORMAL_OFF;
#ifdef DEBUG
    uint64_t waitTimeForPlaceholderMessage{0ULL};
#endif
    //
    ESP_LOGI(tag, "%s: run start...", __func__);
    //
    // das Startsignal leuchten/blinken
    //
    while (esp_timer_get_time() < computeSpeedTime)
    {
      esp32s2::LedControl::showAttention();
      vTaskDelay(1);
    }
    // 100 ms warten
    vTaskDelay(pdMS_TO_TICKS(100));
    // LES aus, NORMAL Modus setzten
    esp32s2::LedControl::allOff();
    Preferences::setAppMode(opMode::NORMAL);
    //
    // hier geth es dann richtig los
    // SChleife für immer :-)
    //
    ESP_LOGI(tag, "%s: loop start...", __func__);
    // nächste Geschwindigkeitsmessung ist:
    computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
    //
    // #####################################################################
    //
    while (true)
    {
      // will ich was melden?
      bool attention = Preferences::getAttentionFlag();
      if (attention)
      {
        esp32s2::LedControl::showAttention();
        ledNextActionTime = esp_timer_get_time() + BLINK_LED_ATTENTION_ON;
      }
      // je nach Modus
      switch (Preferences::getAppMode())
      {
      case opMode::NORMAL:
        if (!attention && (esp_timer_get_time() > ledNextActionTime))
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_NORMAL_ON + BLINK_LED_CONTROL_NORMAL_OFF;
          esp32s2::LedControl::setControlLED(BLINK_LED_CONTROL_NORMAL_ON);
        }
        MainWorker::buttonStati();
        MainWorker::tachoCompute();
#ifdef DEBUG
        if (waitTimeForPlaceholderMessage < esp_timer_get_time())
        {
          ESP_LOGD(tag, "NORMAL MODE...");
          waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
        }
#endif
        break;

      case opMode::CROSS:
        if (!attention && (esp_timer_get_time() > ledNextActionTime))
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_CROSS_ON + BLINK_LED_CONTROL_CROSS_OFF;
          esp32s2::LedControl::setControlLED(BLINK_LED_CONTROL_CROSS_ON);
        }
        MainWorker::buttonStati();
        MainWorker::tachoCompute();
#ifdef DEBUG
        if (waitTimeForPlaceholderMessage < esp_timer_get_time())
        {
          ESP_LOGD(tag, "CROSS MODE...");
          waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
        }
#endif
        break;

      case opMode::RAIN:
        if (!attention && (esp_timer_get_time() > ledNextActionTime))
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_NORMAL_ON + BLINK_LED_CONTROL_NORMAL_OFF;
          esp32s2::LedControl::setControlLED(BLINK_LED_CONTROL_NORMAL_ON);
        }
        MainWorker::buttonStati();
        MainWorker::tachoCompute();
#ifdef DEBUG
        if (waitTimeForPlaceholderMessage < esp_timer_get_time())
        {
          ESP_LOGD(tag, "RAIN MODE...");
          waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
        }
#endif
        break;

      case opMode::TEST:
        if (!attention && (esp_timer_get_time() > ledNextActionTime))
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_TEST_ON + BLINK_LED_CONTROL_TEST_OFF;
          esp32s2::LedControl::setControlLED(BLINK_LED_CONTROL_TEST_ON);
        }
        MainWorker::buttonStati();
#ifdef DEBUG
        if (waitTimeForPlaceholderMessage < esp_timer_get_time())
        {
          ESP_LOGD(tag, "TESTMODE...");
          waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
        }
#endif
        break;
      case opMode::APMODE:
        if (!attention && (esp_timer_get_time() > ledNextActionTime))
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_AP_ON + BLINK_LED_CONTROL_AP_OFF;
          esp32s2::LedControl::setAPModeLED(BLINK_LED_CONTROL_AP_ON);
        }
        MainWorker::buttonStati();
#ifdef DEBUG
        if (waitTimeForPlaceholderMessage < esp_timer_get_time())
        {
          ESP_LOGD(tag, "ACCESSPOINT MODE, WAIT");
          waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
        }
#endif
        break;

      default:
        ESP_LOGE(tag, "UNKNOWN OPMODE!");
        break;
      }
      //
      // ungefähr alle paar Sekunden Berechnen
      // und wenn nicht AP Mode
      //
      if (Preferences::getAppMode() != opMode::APMODE)
      {
        if (esp_timer_get_time() > computeSpeedTime)
        {
          MainWorker::computeAvgSpeed();
          computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
        }
        if (esp_timer_get_time() > computeOilerCheckTime)
        {
          computeOilerCheckTime = esp_timer_get_time() + timeForOilerCheck;
          MainWorker::checkOilState();
        }
      }
      taskYIELD();
      vTaskDelay(1);

    } // while schleife
    //
    // #####################################################################
    //
  }

  /**
   * @brief TEste, ob der Oeler aktiviert werden muss
   *
   */
  void MainWorker::checkOilState()
  {
    using namespace Prefs;
    //
    // lass berechnen ob jetzt geölt werden muss
    // und dann gib nach Prefs::Preferences::addPumpCycles(uint8_t);
    //
    // wie weit war ich?
    float distanceSinceLastOil = Preferences::getRouteLenPastOil();
    // wann ölen?
    float oilInterval = Preferences::getOilInterval();
    ESP_LOGD(tag, "distance is %04.02fm, interval is %04.2fm, absolute: %04.2fm...", static_cast<double>(distanceSinceLastOil), static_cast<double>(oilInterval), Preferences::getAckumulatedRouteLen());
    //
    // wenn es soweit ist, gib Öl
    //
    if (distanceSinceLastOil > oilInterval)
    {
      // TODO: nach Berechnung der Wegstrecken nochmal prüfen ob das so bleibt
      ESP_LOGD(tag, "=== HEAP SIZE: %04u ===", esp_get_free_internal_heap_size());
      ESP_LOGI(tag, "========== oil interval reached ============");
      if (Preferences::getAppMode() == opMode::CROSS)
      {
        Preferences::addPumpCycles(CROSS_OIL_COUNT);
      }
      else if (Preferences::getAppMode() == opMode::RAIN)
      {
        Preferences::addPumpCycles(RAIN_OIL_COUNT);
      }
      else
      {
        Preferences::addPumpCycles(NORMAL_OIL_COUNT);
      }
      esp32s2::LedControl::setPumpLED(true);
      Preferences::setRouteLenPastOil(0);
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
    //
    // Geschwindigkeitsdaten aus der Queue in den Speed-History-Buffer
    // vector wie queue benutzern, aber ich kann std::queue nicht nehmen
    // da ich wahlfrei zugriff haben will
    //
    if (xQueueReceive(TachoControl::speedQueue, &dtime_us, pdMS_TO_TICKS(5)) == pdTRUE)
    {
      while (MainWorker::speedList.size() > Prefs::SPEED_HISTORY_LEN - 1)
      {
        // am Ende entfernen
        MainWorker::speedList.pop_back();
      }
      // das Neue am Anfang einfuegen
      MainWorker::speedList.push_front(dtime_us);
    }
    //
    // zurückgelegte Wegstrecke berechnen
    //
    if (xQueueReceive(TachoControl::pathLenQueue, &evt, pdMS_TO_TICKS(5)) == pdTRUE)
    {
      //
      // wenn in der queue ein ergebnis stand
      //
      // ESP_LOGD(tag, "Event %d meters path done: unit%d; cnt: %d", evt.meters, evt.unit, evt.value);
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

    //
    // Control switch gedrückt länger als LONG ?
    // dann soll es lustig flackern
    //
    if (!Preferences::getAttentionFlag() && (ButtonControl::controlDownSince() != 0ULL) && (ButtonControl::controlDownSince() > LONG_CLICK_TIME_US))
    {
      // ein Ereignis, der knopp ist lange unten und noch nicht wieder hoch
      Preferences::setAttentionFlag(true);
    }
    else if (Preferences::getAttentionFlag() && ((ButtonControl::controlDownSince() == 0ULL) || (ButtonControl::controlDownSince() < LONG_CLICK_TIME_US)))
    {
      // das Ereignis ist vorbei
      Preferences::setAttentionFlag(false);
    }
    //
    // gibt es eine Aktion des Control Switch?
    //
    if (Preferences::getControlSwitchAction() != fClick::NONE)
    {
      //
      // Kurzer Klick an CONTROL
      //
      if (Preferences::getControlSwitchAction() == fClick::SHORT)
      {
        ESP_LOGD(tag, "CONTROL Button short down");
        LedControl::allOff();
        if (Preferences::getAppMode() == opMode::APMODE)
        {
          // unschalten in Normal
          MainWorker::switchFromAccessPointMode();
          // button löschen
          Preferences::setControlSwitchAction(fClick::NONE);
          return;
        }
        //
        // im CROSS/NORMAL mode geht nur hin und her via SHORT
        // REGEN ist bei CROSS deaktiviert
        //
        if (Preferences::getAppMode() == opMode::CROSS)
        {
          ESP_LOGI(tag, "set NORMAL mode");
          Preferences::setAppMode(opMode::NORMAL);
        }
        else
        {
          ESP_LOGI(tag, "set CROSS mode");
          Preferences::setAppMode(opMode::CROSS);
        }
      }
      //
      // langer Klick an CONTROL
      //
      else if (Preferences::getControlSwitchAction() == fClick::LONG)
      {
        ESP_LOGD(tag, "CONTROL Button long down");
        if (Preferences::getAppMode() == opMode::APMODE)
        {
          ESP_LOGI(tag, "set NORMAL mode");
          MainWorker::switchFromAccessPointMode();
        }
        else
        {
          ESP_LOGI(tag, "set ACCESS POINT mode");
          MainWorker::switchToAccessPointMode();
        }
      }
      //
      // als erledigt markieren
      //
      Preferences::setControlSwitchAction(fClick::NONE);
    }
    //
    // gibt es eine Aktion des Regenschalters?
    //
    if (Preferences::getRainSwitchAction() != fClick::NONE)
    {
      //
      // Es gab ein Ereignis
      //
      if (Preferences::getAppMode() == opMode::NORMAL)
      {
        // von NORMAL darf es zu regen gehen, von CROSS nicht
        ESP_LOGI(tag, "set RAIN mode");
        Preferences::setAppMode(opMode::RAIN);
      }
      else if (Preferences::getAppMode() == opMode::RAIN)
      {
        ESP_LOGI(tag, "set NORMAL mode from RAIN");
        Preferences::setAppMode(opMode::NORMAL);
      }
      // Taste löschen
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
      // ESP_LOGD(tag, "distance: %3.3f m time: %3.6f sec", distance_sum, deltaTimeSum_sec);
      averageSpeed = distance_sum / deltaTimeSum_sec;
      Prefs::Preferences::setCurrentSpeed(averageSpeed);
    }
    ESP_LOGD(tag, "av speed: %03.2f m/s == %03.2f km/h, computed: %03d history entrys...", averageSpeed, averageSpeed * 3.6,
             computedCount);
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

  void MainWorker::switchToAccessPointMode()
  {
    using namespace Prefs;
    using namespace esp32s2;

    //
    // wie ist der aktuelle Stand
    //
    if (Preferences::getAppMode() == opMode::APMODE)
    {
      // war schon erledigt
      return;
    }
    esp32s2::TachoControl::pause();
    Preferences::setAppMode(opMode::APMODE);
    //
    // WLAN als Accesspoint starten
    //
    MainWorker::AccessPoint.wifiInitAp();
    // TODO: Webserver starten
  }

  void MainWorker::switchFromAccessPointMode()
  {
    using namespace Prefs;
    using namespace esp32s2;

    //
    // wie ist der aktuelle Stand
    //
    if (Preferences::getAppMode() != opMode::APMODE)
    {
      // war schon erledigt
      return;
    }
    // TODO: Webserver beenden/entfernen
    MainWorker::AccessPoint.shutdownWifi();
    esp32s2::TachoControl::resume();
    Preferences::setAppMode(opMode::NORMAL);
  }

  /**
   * @brief stellt den Grund des Neustarts fest und leitet evtl Aktionen ein
   *
   */
  void MainWorker::processStartupCause()
  {
    MainWorker::wakeupCause = esp_sleep_get_wakeup_cause();
#ifdef DEBUG
    switch (MainWorker::wakeupCause)
    {
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      ESP_LOGD(tag, "wakeup undefined.");
      break;
    case ESP_SLEEP_WAKEUP_ALL:
      ESP_LOGD(tag, "not a wakeup cause (ESP_SLEEP_WAKEUP_ALL)");
      break;
    case ESP_SLEEP_WAKEUP_EXT0:
      ESP_LOGD(tag, "wakeup source is external RTC_IO signal");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      ESP_LOGD(tag, "wakeup source is external RTC_CNTL signal");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      ESP_LOGD(tag, "wakeup ist timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      ESP_LOGD(tag, "wakeup ist touch sensor");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      ESP_LOGD(tag, "wakeup is ULP processor");
      break;
    case ESP_SLEEP_WAKEUP_GPIO:
      ESP_LOGD(tag, "Wakeup caused by GPIO (light sleep only)");
      break;
    case ESP_SLEEP_WAKEUP_UART:
      ESP_LOGD(tag, "Wakeup caused by UART (light sleep only)");
      break;
    case ESP_SLEEP_WAKEUP_WIFI:
      ESP_LOGD(tag, "Wakeup caused by WIFI (light sleep only)");
      break;
    case ESP_SLEEP_WAKEUP_COCPU:
      ESP_LOGD(tag, "Wakeup caused by COCPU int");
      break;
    case ESP_SLEEP_WAKEUP_COCPU_TRAP_TRIG:
      ESP_LOGD(tag, "Wakeup caused by COCPU crash");
      break;
    case ESP_SLEEP_WAKEUP_BT:
      ESP_LOGD(tag, "Wakeup caused by BT (light sleep only)");
      break;
    default:
      ESP_LOGD(tag, "wakeup is not defined, number is %d", MainWorker::wakeupCause);
      break;
    }
#endif
    if (MainWorker::wakeupCause == ESP_SLEEP_WAKEUP_EXT0)
    {
      //
      // Der Tachoimpuls hat geweckt
      //
      ESP_LOGW(tag, "TODO: TACHO WAKEUP restore counters from sram...");
    }
    else
    {
      //
      // Kompletter Neustart
      //
      ESP_LOGW(tag, "TODO: SOMTHING ELSE WAKEUP restore counters from NVS...");
    }
  }

} // namespace ChOiler
