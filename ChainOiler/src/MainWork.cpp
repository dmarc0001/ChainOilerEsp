#include "MainWork.hpp"
#include "TachoControl.hpp"
#include "LedControl.hpp"
#include "ButtonControl.hpp"
#include "PumpControl.hpp"
#include "RainSensorControl.hpp"

namespace ChOiler
{
  /**
   * @brief instanzieren und initialisieren der stsatischen Variablen
   *
   */
  const char *MainWorker::tag{ "MainWorker" };  //! tag fürs debug logging

  /**
   * @brief Construct a new Main Worker:: Main Worker object
   *
   */
  MainWorker::MainWorker()
      : wakeupCause( ESP_SLEEP_WAKEUP_UNDEFINED )
      , buttonControl( nullptr )
      , ledControl( nullptr )
      , pumpControl( nullptr )
      , tachoControl( nullptr )
      , rainControl( nullptr )
  {
    using namespace Prefs;

    //
    // warum geweckt/resettet
    //
    processStartupCause();
    printf( "controller ist starting, version %s...\n\n", Preferences::getVersion().c_str() );
    Preferences::setAppMode( opMode::AWAKE );
    //
    // lese die Einstellungen aus dem NVM
    //
    Preferences::init();
    //
    // gloabal einmalig für GPIO
    //
    gpio_install_isr_service( 0 );
    //
    // Objekte erzeugen / Hardware initialisieren
    //
    buttonControl = esp32s2::ButtonControl::getInstance();
    ledControl = esp32s2::LedControl::getInstance();
    pumpControl = esp32s2::PumpControl::getInstance()();
    tachoControl = esp32s2::TachoControl::getInstance()();
    rainControl = esp32s2::RainSensorControl::getInstance()();
    ESP_LOGD( tag, "init done." );
  }

  /**
   * @brief Hauptschleife des Programmes
   *
   */
  void MainWorker::run()
  {
    using namespace Prefs;

    uint64_t runTime = esp_timer_get_time() + 1500000ULL;
    //
    ESP_LOGI( tag, "%s: run start...", __func__ );
    //
    // das Startsignal leuchten bis zeit erreicht
    //
    while ( esp_timer_get_time() < runTime )
    {
      ledControl->showAttention();
      vTaskDelay( 1 );
    }
    vTaskDelay( pdMS_TO_TICKS( 100 ) );
    // alle LED aus
    ledControl->allOff();
    Preferences::setAppMode( opMode::NORMAL );
    //
    // hier geth es dann richtig los
    // für immer :-)
    //
    ESP_LOGD( tag, "%s: loop start...", __func__ );
    runTime = esp_timer_get_time() + 2000000ULL;
    while ( true )
    {
      //
      // abhängig vom Status verschiedene Tätigkeiten
      //
      switch ( Preferences::getAppMode() )
      {
        case opMode::NORMAL:
        case opMode::CROSS:
        case opMode::RAIN:
          buttonControl->buttonStati();
          tachoControl->tachoCompute();
          break;
        case opMode::TEST:
          buttonControl->buttonStati();
          vTaskDelay( pdMS_TO_TICKS( 100 ) );
          ESP_LOGD( tag, "TESTMODE..." );
          break;
        case opMode::APMODE:
          buttonControl->buttonStati();
          vTaskDelay( pdMS_TO_TICKS( 400 ) );
          ESP_LOGD( tag, "ACCESSPOINT MODE, WAIT" );
          break;
        default:
          break;
      }
      //
      // ungefähr alle 2 Sekunden Speed Berechnen
      // und wenn nicht AP Mode
      //
      if ( Preferences::getAppMode() != opMode::APMODE )
      {
        if ( esp_timer_get_time() > runTime )
        {
          tachoControl->computeAvgSpeed();
          runTime = esp_timer_get_time() + 2000000ULL;
        }
      }
      taskYIELD();
    }
  }

  /**
   * @brief stellt den Grund des Neustarts fest und leitet evtl Aktionen ein
   *
   */
  void MainWorker::processStartupCause()
  {
    wakeupCause = esp_sleep_get_wakeup_cause();
#ifdef DEBUG
    switch ( wakeupCause )
    {
      case ESP_SLEEP_WAKEUP_EXT0:
        printf( "wakeup source is external RTC_IO signal\n" );
        break;
      case ESP_SLEEP_WAKEUP_EXT1:
        printf( "wakeup source is external RTC_CNTL signal\n" );
        break;
      case ESP_SLEEP_WAKEUP_TIMER:
        printf( "wakeup ist timer\n" );
        break;
      case ESP_SLEEP_WAKEUP_TOUCHPAD:
        printf( "wakeup ist touch sensor\n" );
        break;
      case ESP_SLEEP_WAKEUP_ULP:
        printf( "wakeup is ULP processor\n" );
        break;
      default:
        printf( "wakeup is not defined, number is %d\n", wakeupCause );
        break;
    }
#endif
    if ( wakeupCause == ESP_SLEEP_WAKEUP_EXT0 )
    {
      //
      // Der Tachoimpuls hat geweckt
      //
      printf( "TODO: TACHO WAKEUP restore counters from sram..." );
    }
    else
    {
      //
      // Kompletter Neustart
      //
      printf( "TODO: POWER_ON_WAKEUP restore counters from NVS..." );
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
    esp_sleep_enable_ext0_wakeup( Prefs::INPUT_TACHO, 1 );  // 1 == HIGH
    ESP_LOGD( tag, "%s: controller is going to deep sleep...", __func__ );
    printf( "deep sleep..." );
    Prefs::Preferences::close();
    for ( int i = 5; i > 0; --i )
    {
      ESP_LOGD( tag, "%s: sleep in %02d secounds...", __func__, i );
      vTaskDelay( pdMS_TO_TICKS( 1000 ) );
    }
    ledControl->allOff();
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
    tachoControl->pause();
    Preferences::setAppMode( opMode::APMODE );
    // TODO: Accesspoint config/starten
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
    tachoControl->resume();
    Preferences::setAppMode( opMode::NORMAL );
  }

}  // namespace ChOiler
