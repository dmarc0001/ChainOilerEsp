#include "TachoControl.hpp"
#include "ProjectDefaults.hpp"
#include "AppPreferences.hpp"
#include "LedControl.hpp"
#include <cmath>

namespace esp32s2
{
  /**
   * @brief instanziere und initialisiere die statischen Variablen
   *
   */
  xQueueHandle TachoControl::pathLenQueue = nullptr;                  //! handle fuer queue
  xQueueHandle TachoControl::speedQueue = nullptr;                    //! handle fuer queue
  std::list< esp32s2::deltaTimeTenMeters_us > TachoControl::speedList;  //! erzeuge leere Liste
  const char *TachoControl::tag{ "EspCtrl" };                         //! tag fürs debug logging
  bool TachoControl::isInit{ false };                                 //! wurde hard/Software initialisiert?

  /**
   * Initialisieere die Hardware
   */
  void TachoControl::init()
  {
    using namespace Prefs;
    //
    ESP_LOGI( tag, "init hardware..." );
    //
    // Queue initialisieren
    //
    pathLenQueue = xQueueCreate( QUEUE_LEN_DISTANCE, sizeof( pcnt_evt_t ) );
    speedQueue = xQueueCreate( QUEUE_LEN_TACHO, sizeof( deltaTimeTenMeters_us ) );
    //
    // Tacho initialisieren
    //
    ESP_LOGD( tag, "init tacho input..." );
    //
    //  Tacho  Eingang
    //
    gpio_config_t config_in = { .pin_bit_mask = BIT64( INPUT_TACHO ),
                                .mode = GPIO_MODE_INPUT,
                                .pull_up_en = GPIO_PULLUP_ENABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE };
    gpio_config( &config_in );
    //
    // ISR Servive installieren
    // verschoben in main
    // gpio_install_isr_service(0);
    //
    pcnt_unit_t unit0 = PCNT_UNIT_0;
    pcnt_unit_t unit1 = PCNT_UNIT_1;
    ESP_LOGD( tag, "init pulse counter unit0, channel 0..." );
    int16_t pulsesFor100Meters = Preferences::getPulsesFor100Meters();
    //
    // Pulszähler Wegstrecke initialisieren
    //
    pcnt_config_t pcnt_config_w = {
        .pulse_gpio_num = INPUT_TACHO,       /* der eingangspin */
        .ctrl_gpio_num = PCNT_PIN_NOT_USED,  /* kein Kontrolleingang */
        .lctrl_mode = PCNT_MODE_KEEP,        /* Zählrichtung bei CTRL 0 (ignoriert, da ctrl -1) */
        .hctrl_mode = PCNT_MODE_KEEP,        /* Zählrichting bei CTRL HIGH (ignoriert da ctrl -1) */
        .pos_mode = PCNT_COUNT_INC,          /* bei positiver Flanke zählen */
        .neg_mode = PCNT_COUNT_DIS,          /* Zähler bei negativer flanke lassen */
        .counter_h_lim = pulsesFor100Meters, /* 100 Meter Wert, dann rücksetzen */
        .counter_l_lim = 0,                  /* beim rückwärtszählen (aber hier nur positiver zähler) */
        .unit = unit0,                       /* unti 0 zum messen der wegstrecke */
        .channel = PCNT_CHANNEL_0,           /* Kanal 0 zum zählen */
    };
    //
    // initialisieren der unit
    //
    ESP_ERROR_CHECK( pcnt_unit_config( &pcnt_config_w ) );
    ESP_LOGD( tag, "init pulse counter unit0, channel 0...done" );
    //
    ESP_LOGD( tag, "init pulse counter unit1, channel 0..." );
    int16_t pulsesFor10Meters = Preferences::getPulsesFor10Meters();
    //
    // Pulszähler Tacho initialisieren
    //
    pcnt_config_t pcnt_config_s = {
        .pulse_gpio_num = INPUT_TACHO,      /* der eingangspin, deselbe wir unit 0 */
        .ctrl_gpio_num = PCNT_PIN_NOT_USED, /* kein Kontrolleingang */
        .lctrl_mode = PCNT_MODE_KEEP,       /* Zählrichtung bei CTRL 0 (ignoriert, da ctrl -1) */
        .hctrl_mode = PCNT_MODE_KEEP,       /* Zählrichting bei CTRL HIGH (ignoriert da ctrl -1) */
        .pos_mode = PCNT_COUNT_INC,         /* bei positiver Flanke zählen */
        .neg_mode = PCNT_COUNT_DIS,         /* Zähler bei negativer flanke lassen */
        .counter_h_lim = pulsesFor10Meters, /* 10 Meter Wert, dann rücksetzen */
        .counter_l_lim = 0,                 /* beim rückwärtszählen (aber hier nur positiver zähler) */
        .unit = unit1,                      /* unti 1 zum messen der wegstrecke */
        .channel = PCNT_CHANNEL_0,          /* Kanal 0 zum zählen */
    };
    //
    // initialisieren der unit
    //
    ESP_ERROR_CHECK( pcnt_unit_config( &pcnt_config_s ) );
    ESP_LOGD( tag, "init pulse counter unit1, channel 0...done" );
    //
    // Configure and enable the input filter
    // 1800 zyklen sind bei 80 mhz 22 us
    // also wird alles ignoriert was kürzer ist
    //
    ESP_LOGD( tag, "init unit0 pulse filter: filter value: %d, PPR: %.2f...", Preferences::getMinimalPulseLength(),
              Preferences::getPulsePerRound() );
    pcnt_set_filter_value( unit0, Preferences::getMinimalPulseLength() );
    pcnt_filter_enable( unit0 );
    //
    ESP_LOGD( tag, "init unbi1 pulse filter: filter value: %d, PPR: %.2f...", Preferences::getMinimalPulseLength(),
              Preferences::getPulsePerRound() );
    pcnt_set_filter_value( unit1, Preferences::getMinimalPulseLength() );
    pcnt_filter_enable( unit1 );
    //
    // Wert für event (ISR) setzten, wert erreicht, ISR rufen
    //
    pcnt_set_event_value( unit0, PCNT_EVT_THRES_0, pulsesFor100Meters );
    pcnt_event_enable( unit0, PCNT_EVT_THRES_0 );
    pcnt_set_event_value( unit1, PCNT_EVT_THRES_0, pulsesFor10Meters );
    pcnt_event_enable( unit1, PCNT_EVT_THRES_0 );
    //
    // event freigeben
    //
    pcnt_event_enable( unit0, PCNT_EVT_H_LIM );
    pcnt_event_enable( unit1, PCNT_EVT_H_LIM );
    //
    // Counter initialisieren
    //
    pcnt_counter_pause( unit0 );
    pcnt_counter_clear( unit0 );
    pcnt_counter_pause( unit1 );
    pcnt_counter_clear( unit1 );
    //
    // ISR installieren und Callback aktivieren
    //
    pcnt_isr_service_install( 0 );
    pcnt_isr_handler_add( unit0, TachoControl::tachoOilerCountISR, nullptr );
    pcnt_isr_handler_add( unit1, TachoControl::speedCountISR, nullptr );
    //
    // alles ist initialisiert, starte die Counter
    //
    pcnt_counter_resume( unit0 );
    pcnt_counter_resume( unit1 );
    ESP_LOGD( tag, "init pulse counter unit0/unit1...done" );
    TachoControl::speedList.clear();
    TachoControl::isInit = true;
  }

  void TachoControl::pause()
  {
    if ( TachoControl::isInit )
    {
      //
      // Counter pausieren und neu initialisieren
      //
      pcnt_counter_pause( PCNT_UNIT_0 );
      pcnt_counter_pause( PCNT_UNIT_1 );
      pcnt_counter_clear( PCNT_UNIT_0 );
      pcnt_counter_clear( PCNT_UNIT_1 );
    }
  }

  void TachoControl::resume()
  {
    if ( TachoControl::isInit )
    {
      //
      // alles ist initialisiert, starte die Counter
      //
      pcnt_counter_clear( PCNT_UNIT_0 );
      pcnt_counter_clear( PCNT_UNIT_1 );
      pcnt_counter_resume( PCNT_UNIT_0 );
      pcnt_counter_resume( PCNT_UNIT_1 );
    }
  }

  /**
   * @brief berechne Durchschnittsgeschwindigkeit der letzten Sekunden
   *
   */
  void TachoControl::tachoCompute()
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
        while ( TachoControl::speedList.size() > Prefs::SPEED_HISTORY_LEN - 1 )
        {
          // am Ende entfernen
          TachoControl::speedList.pop_back();
        }
        // das Neue am Anfang einfuegen
        TachoControl::speedList.push_front( dtime_us );
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
   * @brief Berechne Durchschnittsgeschwindigkeit der letzten Prefs::HISTORY_MAX_TIME_MS Microsekunden
   *
   */
  void TachoControl::computeAvgSpeed()
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
    for ( auto it = TachoControl::speedList.begin(); it != TachoControl::speedList.end(); )
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
        it = TachoControl::speedList.erase( it, TachoControl::speedList.end() );
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
    ESP_LOGD( tag, "computed average speed: %03.2f m/s == %03.2f km/h, computed: %03d history entrys...", averageSpeed,
              averageSpeed * 3.6, computedCount );
  }

  /**
   * @brief Benachrichtige bei gewünschter Entfernung (hier 100 Meter)
   *
   */
  void IRAM_ATTR TachoControl::tachoOilerCountISR( void * )
  {
    pcnt_evt_t evt;
    evt.unit = PCNT_UNIT_0;
    evt.meters = 100;
    int task_awoken = pdFALSE;
    /*
    PCNT_EVT_THRES_1 = BIT(2),           //!< PCNT watch point event: threshold1 value event
    PCNT_EVT_THRES_0 = BIT(3),           //!< PCNT watch point event: threshold0 value event
    PCNT_EVT_L_LIM = BIT(4),             //!< PCNT watch point event: Minimum counter value
    PCNT_EVT_H_LIM = BIT(5),             //!< PCNT watch point event: Maximum counter value
    PCNT_EVT_ZERO = BIT(6),              //!< PCNT watch point event: counter value zero event
    PCNT_EVT_MAX
    */
    //
    // die Daten in die Queue speichern
    //
    pcnt_get_event_value( PCNT_UNIT_0, PCNT_EVT_THRES_0, &evt.value );
    if ( uxQueueMessagesWaiting( TachoControl::pathLenQueue ) < Prefs::QUEUE_LEN_DISTANCE )
    {
      xQueueSendFromISR( TachoControl::pathLenQueue, &evt, &task_awoken );
      if ( task_awoken == pdTRUE )
      {
        portYIELD_FROM_ISR();
      }
    }
  }

  /**
   * @brief Messe Zeit für 10 Meter
   *
   */
  void IRAM_ATTR TachoControl::speedCountISR( void * )
  {
    uint64_t currentTimeStamp = esp_timer_get_time();
    int task_awoken = pdFALSE;
    //
    // die Daten in die Queue speichern
    //
    if ( uxQueueMessagesWaiting( TachoControl::speedQueue ) < Prefs::QUEUE_LEN_TACHO )
    {
      xQueueSendFromISR( TachoControl::speedQueue, &currentTimeStamp, &task_awoken );
      if ( task_awoken == pdTRUE )
      {
        portYIELD_FROM_ISR();
      }
    }
  }

}  // namespace esp32s2
