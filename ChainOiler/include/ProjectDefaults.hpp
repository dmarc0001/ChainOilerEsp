#pragma once
#include <string>
#include <stdint.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>
#include <driver/rmt.h>

namespace Prefs
{
  //****************************************************************************
  // Konstanten für das Programm
  // Berechningen intern in Metern und Sekunden
  // Konfig in km/h
  //****************************************************************************
  //
  // sonstige Programm Konmstanten
  //
  constexpr uint32_t THREAD_STACK_SIZE = 4096;
  //
  // Version des Speichers bei Versionen, wenn änderungen an speicher
  // muss die Version erhöht werden und im code reagiert werden
  //
  constexpr uint32_t CURRENT_PREFS_VERSION = 1U;
  constexpr uint32_t INVALID_VERSION = 999U;
  constexpr const char *PREFS_PARTITION_LABEL{"prefs"};
  //
  // Konfigurierbare Parameter und deren keys
  //
  constexpr const char *PREFS_VERSION_STR{"prefs_ver"};                //! Name fuer Version
  constexpr const char *PREFS_PARTITION{"prefs"};                      //! partition für Einstelliungen
  constexpr const char *SSID_STR{"ap_ssid"};                           //! SSID name für WLAN-Accesspoint
  constexpr const char *DEFAULT_SSID{"CHAINOILER"};                    //! SSID für WLAN-Accesspoint
  constexpr const char *AP_PASSWORD_STR{"ap_password"};                //! Passwort für WLAN
  constexpr const char *DEFAULT_AP_PASSWORD{"password"};               //! Passwort für WLAN
  constexpr const char *AP_CHANNEL_STR{"ap_cannel"};                   //! accesspoinmt Kanal Einstellung
  constexpr uint8_t DEFAULT_AP_CHANNEL{2};                             //! default WLAN channel
  constexpr const char *AP_MAX_CONNECTIONS_STR{"ap_max_conns"};        //! maximale anzahl gleichzeitiger Verbindungen
  constexpr uint8_t DEFAULT_AP_MAX_CONNECTIONS{4};                     //! maximale verbindungen
  constexpr const char *PULSE_PER_WEEL_ROUND_STR{"pulse_per_round"};   //! Impulse per Radumdrehung
  constexpr double DEFAULT_PULSE_PER_WEEL_ROUND = 109.0D;              //! Defaultwert für Reed, Honda Africa Twin 109
  constexpr const char *WHEEL_CIRCUM_FERENCE_STR{"curcum_ference"};    //! Radumfang
  constexpr double DEFAULT_WHEEL_CIRCUM_FERENCE = 1.81D;               //! Default Umfang Hinterrad
  constexpr const char *OIL_INTERVAL_STR{"oil_interval"};              //! Öl interval in Metern
  constexpr double DEFAULT_OIL_INTERVAL = 2000.0D;                     //! Schmierinterval 4000 Meter
  constexpr const char *RAIN_OIL_INTERVAL_FACTOR_STR{"rain_factor"};   //! Streckenfaktor bei Regen
  constexpr double DEFAULT_RAIN_OIL_INTERVAL_FACTOR = 1.4D;            //! wieviel mal gegenüber normal schmieren
  constexpr const char *CROSS_OIL_INTERVAL_FACTOR_STR{"cross_factor"}; //! Streckenfaktor bei cross
  constexpr double DEFAULT_CROSS_OIL_INTERVAL_FACTOR = 6.0D;           //! wieviel mal öfter beim Crossen schmieren
  constexpr const char *SPEED_PROGRESSION_FACTOR_STR{"speed_progres"}; //! Geschwindigkeits regression
  constexpr double DEFAULT_SPEED_PROGRESSION_FACTOR = -0.76D;          //! der Faktor für die Berechnung der Progression
  constexpr const char *THRESHOLD_RAIN_SENSOR_STR{"rain_threshold"};   //! Regensensor Schwellwert
  constexpr uint32_t DEFAULT_THRESHOLD_RAIN_SENSOR = 512U;             //! Schwellenwert für Regen TODO: Hysterese zum Abschalten
  constexpr const char *PUMP_LED_LITHGING_TIME_STR{"pump_led_time"};   //! Zeit für die Aktivitätsanzeige
  constexpr uint64_t DEFAULT_PUMP_LED_LITHGING_TIME = 250ULL;          //! Leuchtzeit der Pumpen-LED
  constexpr const char *ABSOLUTE_PATH_LEN_STR{"abs_path_len"};         //! akkumulierte Wegstrecke
  //
  // Ein-/Ausgänge GPIO, nicht konfigurierbar
  //
  constexpr gpio_num_t LED_REED_CONTROL = GPIO_NUM_26;          //! Kontrolle für Tacho und Reed Impuls
  constexpr gpio_num_t LED_CONTROL = GPIO_NUM_33;               //! LED für Kontrolle, WLAN, CROSS
  constexpr gpio_num_t LED_RAIN = GPIO_NUM_34;                  //! Regen Betriebsanzeige
  constexpr gpio_num_t LED_PUMP = GPIO_NUM_35;                  //! Aktivitätsanzeige Pumpe
  constexpr gpio_num_t OUTPUT_PUMP_CONTROL = GPIO_NUM_45;       //! Betätigt die Punpe, 20ms Stöße
  constexpr gpio_num_t OUTPUT_RAIN_SW_01 = GPIO_NUM_42;         //! Schaltet Regensensor 01
  constexpr gpio_num_t OUTPUT_RAIN_SW_02 = GPIO_NUM_41;         //! Schaltet Regensensor 02
  constexpr gpio_num_t INPUT_TACHO = GPIO_NUM_4;                //! Tachoimpuls / Reed Kontakt
  constexpr gpio_num_t INPUT_CONTROL_SWITCH = GPIO_NUM_5;       //! Funktionstaster Cross/WLAN
  constexpr gpio_num_t INPUT_RAIN_SWITCH_OPTIONAL = GPIO_NUM_6; //! optionaler Regentaster (TODO:)
  constexpr adc1_channel_t INPUT_ADC_RAIN_00 = ADC1_CHANNEL_0;  //! Eingang Analogschnittstelle GPIO01- PIN01
  constexpr adc1_channel_t INPUT_ADC_RAIN_01 = ADC1_CHANNEL_2;  //! Eingang Analogschnittstelle GPIO03 - PIN03
//
// LED Stripe
//
#ifdef LEDSTRIPE
  constexpr gpio_num_t LED_STRIPE_RMT_TX_GPIO = GPIO_NUM_36;      //! Kontrolle für LED-stripe
  constexpr rmt_channel_t LED_STRIPE_RMT_CHANNEL = RMT_CHANNEL_3; //! welcher remotecontrol channel
  constexpr uint32_t LED_STRIPE_COUNT = 3U;                       //! Anzahl der LED im Streifen
  constexpr uint32_t LED_STRIPE_CONTROL = 2U;                     //! Stripe LED für CONTROL
  constexpr uint32_t LED_STRIPE_RAIN = 1U;                        //! Stripe LED für Regen
  constexpr uint32_t LED_STRIPE_PUMP = 0L;                        //! Stripe LED für Pumpe
  constexpr uint32_t LED_STRIPE_CONTROL_HSVCOLOR = 120U;          //! Farbe für Control Blinken (grün)
  constexpr uint32_t LED_STRIPE_CROSS_HSVCOLOR = 15U;             //! Farbe für Control Blinken (zinnober)
  constexpr uint32_t LED_STRIPE_PUMP_HSVCOLOR = 195U;             //! Farbe für Pumpe nachleuchten (blaucyan)
  constexpr uint32_t LED_STRIPE_RAIN_HSVCOLOR = 75U;              //! Farbe für Regen (grüngelb)
  constexpr uint32_t LED_STRIPE_ATT_HSVCOLOR = 300U;              //! Farbe für ATENTION (magenta)
  constexpr uint32_t LED_STRIPE_AP_HSVCOLOR = 165U;               //! Farbe für ACCESSPOINT nachleuchten (grüncyan)
#endif

  //
  // Wie lange dauert das entprellen in Microsekunden
  //
  constexpr int64_t DEBOUNCE_TIME_US = 50 * 1000;        //! Microsekunden zum entprellen
  constexpr int64_t LONG_CLICK_TIME_US = 3000 * 1000;    //! Mikrosekunden fuer Langen Click (WiFi ON)
  constexpr int64_t TIME_TO_DEEP_SLEEP = 20000 * 1000UL; //! Zeit bis zum Sleep
  //
  // intervalle beim blinken in Microsekunden
  //
  constexpr int64_t BLINK_LED_CONTROL_NORMAL_OFF = 3000 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_NORMAL_ON = 10 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_CROSS_OFF = 2500 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_CROSS_ON = 100 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_AP_OFF = 100 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_AP_ON = 1000 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_TEST_OFF = 60 * 1000;
  constexpr int64_t BLINK_LED_CONTROL_TEST_ON = 60 * 1000;
  constexpr int64_t BLINK_LED_ATTENTION_OFF = 35 * 1000;
  constexpr int64_t BLINK_LED_ATTENTION_ON = 35 * 1000;
  constexpr int64_t BLINK_LED_AWAKE_OFF = 150 * 1000;
  constexpr int64_t BLINK_LED_AWAKE_ON = 80 * 1000;
#ifdef RAWLED
  constexpr int64_t PUMP_LED_DELAY = 3000 * 1000;
#else
  constexpr int64_t PUMP_LED_DELAY = 1200 * 1000;
#endif
  //
  // Konstanten
  //
  constexpr size_t QUEUE_LEN_TACHO = 24;                 //! wie lang ist die queue für Tacho Ereignisse
  constexpr size_t SPEED_HISTORY_LEN = 40;               //! max länge der Tempo-Historie
  constexpr uint32_t PATH_LEN_METERS_PER_ISR = 25;       //! wie weit ist die strecke zwiwchen zwei interrupts
  constexpr float PATH_LEN_METERS_PER_ISR_FLOAT = 25.0F; //! dieselbe Länge als Float
  constexpr uint64_t HISTORY_MAX_TIME_MS = 10000;        //! wie lang ist die history beim speed maximal
  constexpr uint32_t HISTORY_MAX_SAMPLES = 4;            //! wie viele Samples reichen für die Durchschnittsberechnung x* 25 meter....
  constexpr uint8_t PUMP_OFF_ZYCLES = 9;                 //! wenn die Punpe on war, wie viele Zyklen bis wieder an möglich
  constexpr uint32_t P_OFF = 0U;                         //! Status für Pumpe OFF
  constexpr uint32_t P_ON = 1U;                          //! Status für Punpe ON
  constexpr uint32_t LED_OFF = 0U;                       //! Status für LED OFF
  constexpr uint32_t LED_ON = 1U;                        //! Status für LED ON
  constexpr uint8_t NORMAL_OIL_COUNT = 1;                //! Pumpenzyklen für normales ölen
  constexpr uint8_t RAIN_OIL_COUNT = 3;                  //! Pumpenzyklen für regen ölen
  constexpr uint8_t CROSS_OIL_COUNT = 8;                 //! Pumpenzyklen für crossen ölen
} // namespace Prefs
