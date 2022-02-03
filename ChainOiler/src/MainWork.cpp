#include "MainWork.hpp"

namespace ChOiler
{
  /**
   * @brief instanzieren und initialisieren der stsatischen Variablen
   *
   */
  const char *MainWorker::tag{ "MainWorker" };                        //! tag fürs debug logging
  std::list< esp32s2::deltaTimeTenMeters_us > MainWorker::speedList;  //! erzeuge leere Liste
  WiFiAccessPoint MainWorker::AccessPoint;                            //! statisches Objekt
  constexpr uint64_t timeForMessage = 3500000ULL;                     //! Zeit zur nächsten Nachricht
  constexpr uint64_t timeForSpeedMeasure = 2000000ULL;                //! zeit bis zur Geschwindigkeitsmessung
  constexpr uint64_t timeForOilerCheck = 3200000ULL;                  //! zeit bis zur Kontrole ob Geölt werden muss

  /**
   * @brief initialisiere das Programm
   *
   */
  void MainWorker::init()
  {
    using namespace Prefs;

    printf( "controller ist starting, version %s...\n\n", Preferences::getVersion().c_str() );
    MainWorker::speedList.clear();
    Preferences::setAppMode( opMode::AWAKE );
    //
    // lese die Einstellungen aus dem NVM
    //
    Preferences::init();
    //
    // initialisiere die Hardware
    //
    gpio_install_isr_service( 0 );  // gloabal einmalig für GPIO
    esp32s2::ButtonControl::init();
    esp32s2::LedControl::init();
    esp32s2::PumpControl::init();
    esp32s2::TachoControl::init();
    esp32s2::RainSensorControl::init();
    ESP_LOGD( tag, "init done." );
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
    uint64_t waitTimeForPlaceholderMessage{ 0ULL };
    //
    ESP_LOGI( tag, "%s: run start...", __func__ );
    //
    // das Startsignal leuchten
    //
    while ( esp_timer_get_time() < computeSpeedTime )
    {
      esp32s2::LedControl::showAttention();
      vTaskDelay( 1 );
    }
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
    esp32s2::LedControl::allOff();
    Preferences::setAppMode( opMode::NORMAL );
    //
    // hier geth es dann richtig los
    // für immer :-)
    //
    ESP_LOGI( tag, "%s: loop start...", __func__ );
    computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
    //
    while ( true )
    {
      switch ( Preferences::getAppMode() )
      {
        case opMode::NORMAL:
        case opMode::CROSS:
        case opMode::RAIN:
          MainWorker::buttonStati();
          MainWorker::tachoCompute();
#ifdef DEBUG
          if ( waitTimeForPlaceholderMessage < esp_timer_get_time() )
          {
            switch ( Preferences::getAppMode() )
            {
              default:
              case opMode::NORMAL:
                ESP_LOGD( tag, "NORMAL MODE..." );
                break;
              case opMode::CROSS:
                ESP_LOGD( tag, "CROSS MODE..." );
                break;
              case opMode::RAIN:
                ESP_LOGD( tag, "RAIN MODE..." );
                break;
            }
            waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
          }
#endif
          break;
        case opMode::TEST:
          MainWorker::buttonStati();
#ifdef DEBUG
          if ( waitTimeForPlaceholderMessage < esp_timer_get_time() )
          {
            ESP_LOGD( tag, "TESTMODE..." );
            waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
          }
#endif
          break;
        case opMode::APMODE:
          MainWorker::buttonStati();
#ifdef DEBUG
          if ( waitTimeForPlaceholderMessage < esp_timer_get_time() )
          {
            ESP_LOGD( tag, "ACCESSPOINT MODE, WAIT" );
            waitTimeForPlaceholderMessage = esp_timer_get_time() + timeForMessage;
          }
#endif
          break;
        default:
          break;
      }
      //
      // ungefähr alle paar Sekunden Berechnen
      // und wenn nicht AP Mode
      //
      if ( Preferences::getAppMode() != opMode::APMODE )
      {
        if ( esp_timer_get_time() > computeSpeedTime )
        {
          MainWorker::computeAvgSpeed();
          computeSpeedTime = esp_timer_get_time() + timeForSpeedMeasure;
        }
        if ( esp_timer_get_time() > computeOilerCheckTime )
        {
          computeOilerCheckTime = esp_timer_get_time() + timeForOilerCheck;
          MainWorker::checkOilState();
        }
      }
      taskYIELD();
    }
  }

  void MainWorker::checkOilState()
  {
    //
    // TODO: lass berechnen ob jetzt geölt werden muss
    // und dann gib nach Prefs::Preferences::
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
    //
    // Geschwindigkeitsdaten aus der Queue in den Speed-History-Buffer
    // vector wie queue benutzern, aber ich kann std::queue nicht nehmen
    // da ich wahlfrei zugriff haben will
    //
    res = xQueueReceive( TachoControl::speedQueue, &dtime_us, pdMS_TO_TICKS( 10 ) );
    {
      if ( res == pdTRUE )
      {
        while ( MainWorker::speedList.size() > Prefs::SPEED_HISTORY_LEN - 1 )
        {
          // am Ende entfernen
          MainWorker::speedList.pop_back();
        }
        // das Neue am Anfang einfuegen
        MainWorker::speedList.push_front( dtime_us );
      }
    }
    //
    // zurückgelegte Wegstrecke berechnen
    //
    res = xQueueReceive( TachoControl::pathLenQueue, &evt, pdMS_TO_TICKS( 10 ) );
    if ( res == pdTRUE )
    {
      //
      // wenn in der queue ein ergebnis stand
      //
      // pcnt_get_counter_value(PCNT_UNIT_0, &count);
      ESP_LOGI( tag, "Event %d meters path done: unit%d; cnt: %d", evt.meters, evt.unit, evt.value );
      Prefs::Preferences::addRouteLenPastOil( evt.meters );
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
    //
    if ( ButtonControl::controlDownSince() > 0ULL )
    {
      if ( !Preferences::getAttentionFlag() )
      {
        Preferences::setAttentionFlag( true );
      }
    }
    else if ( Preferences::getAttentionFlag() )
    {
      Preferences::setAttentionFlag( false );
    }
    //
    // gibt es eine Aktion des Control Switch?
    //
    if ( Preferences::getControlSwitchAction() != fClick::NONE )
    {
      //
      // Kurzer Klick an CONTROL
      //
      if ( Preferences::getControlSwitchAction() == fClick::SHORT )
      {
        ESP_LOGD( tag, "CONTROL Button short down" );
        LedControl::allOff();
        if ( Preferences::getAppMode() == opMode::APMODE )
        {
          // unschalten in Normal
          MainWorker::switchFromAccessPointMode();
          // button löschen
          Preferences::setControlSwitchAction( fClick::NONE );
          return;
        }
        //
        // im CROSS/NORMAL mode geht nur hin und her via SHORT
        // REGEN ist bei CROSS deaktiviert
        //
        if ( Preferences::getAppMode() == opMode::CROSS )
        {
          ESP_LOGD( tag, "set NORMAL mode" );
          Preferences::setAppMode( opMode::NORMAL );
        }
        else
        {
          ESP_LOGD( tag, "set CROSS mode" );
          Preferences::setAppMode( opMode::CROSS );
        }
      }
      //
      // langer Klick an CONTROL
      //
      else if ( Preferences::getControlSwitchAction() == fClick::LONG )
      {
        ESP_LOGI( tag, "CONTROL Button long down" );
        ESP_LOGD( tag, "set ACCESS POINT mode" );
        LedControl::allOff();
        MainWorker::switchToAccessPointMode();
      }
      //
      // als erledigt markieren
      //
      Preferences::setControlSwitchAction( fClick::NONE );
    }
    //
    // gibt es eine Aktion des Regenschalters?
    //
    if ( Preferences::getRainSwitchAction() != fClick::NONE )
    {
      //
      // Es gab ein Ereignis
      //
      if ( Preferences::getAppMode() == opMode::NORMAL )
      {
        // von NORMAL darf es zu regen gehen, von CROSS nicht
        ESP_LOGD( tag, "set RAIN mode" );
        Preferences::setAppMode( opMode::RAIN );
      }
      else if ( Preferences::getAppMode() == opMode::RAIN )
      {
        ESP_LOGD( tag, "set NORMAL mode from RAIN" );
        Preferences::setAppMode( opMode::NORMAL );
      }
      // Taste löschen
      Preferences::setRainSwitchAction( fClick::NONE );
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
    uint64_t lastTimeStamp{ 0ULL };
    float distance_sum = 0.0F;
    float deltaTimeSum_sec = 0.0F;
    float averageSpeed = 0.0;
    int computedCount = 0;
    for ( auto it = MainWorker::speedList.begin(); it != MainWorker::speedList.end(); )
    {
      //
      // zuerst veraltete Einträge finden und entfernen
      // diff in ms berechnen und merken
      //
      uint64_t logDeltaTimeStamp_ms = ( esp_timer_get_time() - *it ) >> 10;
      //
      // mehr als Prefs::HISTORY_MAX_TIME_MS Milisekunden her, also veraltet?
      //
      if ( logDeltaTimeStamp_ms > Prefs::HISTORY_MAX_TIME_MS )
      {
        // zu alt
        // iterator wird neu gesetzt, alles nach dem gelöscht
        //
        it = MainWorker::speedList.erase( it, MainWorker::speedList.end() );
        continue;
      }
      //
      // gibt es einen früheren Zeitstempel auf den ich Bezug nehmen kann?
      //
      if ( lastTimeStamp == 0ULL )
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
      uint32_t deltaTime_ms = static_cast< uint32_t >( ( lastTimeStamp - *it ) >> 10 );
      //
      // jetzt habe ich eine zahl < history ms, weil die zu alten loesche ich oben
      // hier völlig zum schätzen der Geschwindigkeit, Korrekturfaktor ist rund 0.95
      //
      float timeDiff_sec = ( static_cast< float >( deltaTime_ms ) / 1000.0F ) * 0.95F;
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
    if ( deltaTimeSum_sec > 0.001 )
    {
      // distance durch zeit...
      ESP_LOGD( tag, "distance: %3.3f m time: %3.6f sec", distance_sum, deltaTimeSum_sec );
      averageSpeed = distance_sum / deltaTimeSum_sec;
      Prefs::Preferences::setCurrentSpeed( averageSpeed );
    }
    ESP_LOGD( tag, "av speed: %03.2f m/s == %03.2f km/h, computed: %03d history entrys...", averageSpeed, averageSpeed * 3.6,
              computedCount );
  }

  /**
   * gehe in den Tiefen Schlaf
   */
  void MainWorker::goDeepSleep()
  {
    //
    // Tachoimpuls soll wechen
    //
    esp_sleep_enable_ext0_wakeup( Prefs::INPUT_TACHO, 1 );  // 1 == HIGH
    ESP_LOGD( tag, "%s: controller is going to deep sleep...", __func__ );
    printf( "deep sleep..." );
    Prefs::Preferences::close();
    for ( int i = 5; i > 0; --i )
    {
      ESP_LOGD( tag, "%s: sleep in %02d secounds...", __func__, i );
      vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
    esp32s2::LedControl::allOff();
    printf( "..Good night.\n" );
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
    if ( Preferences::getAppMode() == opMode::APMODE )
    {
      // war schon erledigt
      return;
    }
    esp32s2::TachoControl::pause();
    Preferences::setAppMode( opMode::APMODE );
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
    if ( Preferences::getAppMode() != opMode::APMODE )
    {
      // war schon erledigt
      return;
    }
    // TODO: Webserver beenden/entfernen
    // TODO: Accesspoint beenden
    esp32s2::TachoControl::resume();
    Preferences::setAppMode( opMode::NORMAL );
  }

}  // namespace ChOiler
