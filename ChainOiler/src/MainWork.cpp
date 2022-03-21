#include "MainWork.hpp"
#include <esp_log.h>

namespace ChOiler
{
  /**
   * @brief instanzieren und initialisieren der stsatischen Variablen
   *
   */
  const char *MainWorker::tag{"MainWorker"};                                    //! tag fürs debug logging
  esp_sleep_wakeup_cause_t MainWorker::wakeupCause{ESP_SLEEP_WAKEUP_UNDEFINED}; //! startgrund
  std::list<MainWorker::speedMicroSec> MainWorker::speedList;                   //! erzeuge leere Liste
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

    ESP_LOGD(tag, "controller ist starting, version %s...", Preferences::getVersion().c_str());
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
    esp32s2::ButtonControl::init();     // Hardware für den Knopf
    esp32s2::SignalControl::init();     // Signalisierung (LED)
    esp32s2::PumpControl::init();       // Pumpen Ansteuerung
    esp32s2::TachoControl::init();      // Tachomessung
    esp32s2::RainSensorControl::init(); // Regensensor
    ESP_LOGD(tag, "init done.");
  }

  /**
   * @brief Hauptschleife des Programmes, läuft als eigener TASK
   *
   */
  void MainWorker::run(void *)
  {
    using namespace Prefs;

    static int64_t computeSpeedTime{0LL};
    static int64_t computeOilerCheckTime{0LL};
    static int64_t ledNextActionTime{0LL};
    static int64_t rainLedNextActionTime{0LL};
    esp_err_t err{0};
    TaskHandle_t workerHandle{nullptr};
    int64_t nowTime = esp_timer_get_time();

    //
    // das Taskhandle für diesen Task erfragen, brauche ich für den Wachhund
    //
    workerHandle = xTaskGetIdleTaskHandle();

    ESP_LOGI(tag, "%s: run start...", __func__);
    //
    // das Startsignal leuchten/blinken
    //
    Preferences::setAttentionFlag(true);
    // blinken zum Start des Task
    computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
    while (esp_timer_get_time() < computeSpeedTime)
    {
      // das sollte so 2 Sekunden dauern (timeForSpeedMeasure)
      vTaskDelay(100);
    }
    // LED aus, NORMAL Modus setzten
    Preferences::setAttentionFlag(false);
    esp32s2::SignalControl::allOff();
    //
    Preferences::setAppMode(opMode::NORMAL);
    //
    // hier geth es dann richtig los
    // SChleife für immer :-)
    //
    ESP_LOGD(tag, "%s: loop start...", __func__);
#ifdef DEBUG
    // TODO: nur zum Testen
    Preferences::addPumpCycles(8);
#endif
    //
    // Aktionszeitpunkte festlegen
    //
    nowTime = esp_timer_get_time();
    computeSpeedTime = nowTime + timeForSpeedMeasure;
    computeOilerCheckTime = nowTime + timeForOilerCheck;
    ledNextActionTime = nowTime + BLINK_LED_CONTROL_NORMAL_OFF;
    rainLedNextActionTime = nowTime + BLINK_LED_RAIN_OFF;
    //
    // watchdog initialisieren und scharf machen
    //
    ESP_LOGI(tag, "init watchdog for task...");
    err = esp_task_wdt_init(10000, false);
    if (err != ESP_OK)
    {
      ESP_LOGW(tag, "esp_task_wdt_init failed...");
      err = esp_task_wdt_add(workerHandle);
      ESP_LOGD(tag, "thread added...");
      if (err != ESP_OK)
      {
        ESP_LOGE(tag, "esp_task_wdt_add failed...");
        if (err != ESP_OK)
        {
          err = esp_task_wdt_reset();
          ESP_LOGE(tag, "esp_task_wdt_reset failed...");
        }
      }
      if (err == ESP_OK)
      {
        ESP_LOGI(tag, "esp watchdog init successful...");
      }
    }
    //
    // #####################################################################
    // #### forever and forever                                         ####
    // #####################################################################
    //
    while (true)
    {
      // wachund zurücksetzen
      esp_task_wdt_reset();
      //
      // je nach Modus
      //
      switch (Preferences::getAppMode())
      {
      case opMode::NORMAL:
        if (esp_timer_get_time() > ledNextActionTime)
        {
          // Blitzen initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_NORMAL_ON + BLINK_LED_CONTROL_NORMAL_OFF;
          esp32s2::SignalControl::flashControlLED();
        }
        MainWorker::buttonStati();
        MainWorker::tachoCompute();
        break;

      case opMode::CROSS:
        if (esp_timer_get_time() > ledNextActionTime)
        {
          // blitzen initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_CROSS_ON + BLINK_LED_CONTROL_CROSS_OFF;
          esp32s2::SignalControl::flashCrossLED();
        }
        MainWorker::buttonStati();
        MainWorker::tachoCompute();
        break;

      case opMode::RAIN:
        if (esp_timer_get_time() > ledNextActionTime)
        {
          // blitzen initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_NORMAL_ON + BLINK_LED_CONTROL_NORMAL_OFF;
          esp32s2::SignalControl::flashControlLED();
          if (esp_timer_get_time() > rainLedNextActionTime)
          {
            esp32s2::SignalControl::flashRainLED();
            // das bewirkt, dass wenn regenmode aktiv ist, die LED dauerleuchtet
            // und wenn nicht geht sie aleine aus
            rainLedNextActionTime = esp_timer_get_time() + BLINK_LED_RAIN_OFF;
          }
        }
        MainWorker::buttonStati();
        MainWorker::tachoCompute();
        break;

      case opMode::TEST:
        if (esp_timer_get_time() > ledNextActionTime)
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_TEST_ON + BLINK_LED_CONTROL_TEST_OFF;
          esp32s2::SignalControl::flashControlLED();
        }
        MainWorker::buttonStati();
        break;
      case opMode::APMODE:
        if (esp_timer_get_time() > ledNextActionTime)
        {
          // Blinken initiieren
          ledNextActionTime = esp_timer_get_time() + BLINK_LED_CONTROL_AP_ON + BLINK_LED_CONTROL_AP_OFF;
          esp32s2::SignalControl::flashControlLED();
        }
        MainWorker::buttonStati();
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
          esp_task_wdt_reset();
          MainWorker::computeAvgSpeed();
          taskYIELD();
          computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
        }
        if (esp_timer_get_time() > computeOilerCheckTime)
        {
          // den Wachhund füttern
          esp_task_wdt_reset();
          computeOilerCheckTime = esp_timer_get_time() + timeForOilerCheck;
          MainWorker::checkOilState();
        }
      }
      taskYIELD();
      vTaskDelay(pdMS_TO_TICKS(20));
    } // while schleife
    //
    // #####################################################################
    //
  }

  /**
   * @brief Teste, ob der Oeler aktiviert werden muss
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
    //
    // wenn es soweit ist, gib Öl
    //
    if (distanceSinceLastOil > oilInterval)
    {
      // TODO: nach Berechnung der Wegstrecken nochmal prüfen ob das so bleibt
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
      // bei einem pumpenstoss könnte es zu knapp sein (Pumpe 20 ms -> led 100 ms )
      Preferences::setRouteLenPastOil(0.0F);
      ESP_LOGI(tag, "========== oil interval reached oil %02d cycles ============", Preferences::getPumpCycles());
    }
  }

  /**
   * @brief berechne Durchschnittsgeschwindigkeit der letzten Sekunden
   *
   */
  void MainWorker::tachoCompute()
  {
    using namespace esp32s2;
    tachoQueueEntry_t entry{0LL, 0UL};
    //
    // Geschwindigkeitsdaten aus der Queue in den Speed-History-Buffer
    // vector wie queue benutzern, aber ich kann std::queue nicht nehmen
    // da ich wahlfrei zugriff haben will
    //
    while (xQueueReceive(TachoControl::speedQueue, &entry, 10) == pdTRUE)
    {
      while (MainWorker::speedList.size() > Prefs::SPEED_HISTORY_LEN - 1)
      {
        // am Ende entfernen
        MainWorker::speedList.pop_back();
      }
      // das Neue am Anfang einfuegen
      MainWorker::speedList.push_front(entry.timestamp_us);
      // zurückgelegte Strecke einfügen
      Prefs::Preferences::addRouteLenPastOil(entry.meters);
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
    // Durchschnitt über die letzten Sekunden
    // jeder Zeitstempel ist für PATH_LEN_METERS_PER_ISR Meter Strecke
    // die Durchschnittsgeschwindigkeit ist also max über
    // PATH_LEN_METERS_PER_ISR * Prefs::SPEED_HISTORY_LEN
    //
    static int64_t lastTimeStamp_us{0LL};
    float distance_sum_meter{0.0F};
    float deltaTimeSum_sec{0.0F};
    float averageSpeed{0.0};
    int64_t deltaTimeSum_ms{0LL};
    uint32_t computedCount{0LL};

    //
    // über die Liste iterieren
    //
    for (auto it = MainWorker::speedList.begin(); it != MainWorker::speedList.end(); ++it)
    {
      int64_t currentTimestamp = *it;
      if ((lastTimeStamp_us == 0) || (lastTimeStamp_us < currentTimestamp))
      {
        //
        // der erste ist nur die Anfangsmarkierung
        //
        lastTimeStamp_us = currentTimestamp;
        continue;
      }
      ++computedCount;
      //
      // jeder Eitrag steht für Zeit pro 25 Meter Wegstrecke
      // Einträge in microsekunden
      //
      // Entfernng summieren
      distance_sum_meter += Prefs::PATH_LEN_METERS_PER_ISR_FLOAT;
      // Zeit Differenz summieren, wobei *it der neuere Eintrag ist
      int64_t delta = (lastTimeStamp_us - currentTimestamp) >> 10;
      deltaTimeSum_ms += delta;
      //
      //  wird die History zu lang? oder sind mehr samples als ich brauche?
      //
      if ((((esp_timer_get_time() - lastTimeStamp_us) >> 10) > Prefs::HISTORY_MAX_TIME_MS) || (computedCount >= Prefs::HISTORY_MAX_SAMPLES))
      {
        //
        // zu alt, oder genug samples für den Durchschnitt
        // iterator wird neu gesetzt, alles nach dem gelöscht
        //
        it = MainWorker::speedList.erase(it, MainWorker::speedList.end());
        break;
      }
      // alten Stand übernehmen
      lastTimeStamp_us = currentTimestamp;
    }
    lastTimeStamp_us = 0ULL;
    //
    // hier sollte ich die Strecke und die Zeit haben
    // oder die _history war zu kurz
    //
    if (computedCount < 2U)
    {
      Prefs::Preferences::setCurrentSpeed(0.0F);
      ESP_LOGD(tag, "av speed: 0.00 m/s == 0.00 km/h, computed: %03u history entrys...", computedCount);
      return;
    }
    //
    // umrechnen in Sekunden, mit Korrektur der >> 10 in tacho compute
    //
    deltaTimeSum_sec = (static_cast<float>(deltaTimeSum_ms) / 1000.0F * 0.95F);
    //
    // berechne Durchschnittliche Geschwindigkeit für die letzten Sekunden
    //
    if (deltaTimeSum_sec > 0.001f)
    {
      // distance durch zeit...
      // ESP_LOGD(tag, "distance: %3.3f m time: %3.6f sec (%02d/%06u)", distance_sum_meter, deltaTimeSum_sec, MainWorker::speedList.size(), static_cast<uint32_t>(deltaTimeSum_ms));
      averageSpeed = static_cast<float>(distance_sum_meter) / deltaTimeSum_sec;
      Prefs::Preferences::setCurrentSpeed(averageSpeed);
    }
    // ESP_LOGD(tag, "av speed: %03.2f m/s == %03.2f km/h, computed: %03u history entrys...", averageSpeed, (averageSpeed * 3.6), computedCount);
    ESP_LOGD(tag, "av speed: %03.2f m/s == %03.2f km/h...", averageSpeed, (averageSpeed * 3.6));
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
          SignalControl::flashControlLED();
        }
        else
        {
          ESP_LOGI(tag, "set CROSS mode");
          Preferences::setAppMode(opMode::CROSS);
          SignalControl::flashCrossLED();
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
    esp32s2::SignalControl::allOff();
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
