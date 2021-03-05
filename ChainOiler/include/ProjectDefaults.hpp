#pragma once
#include <string>
#include <stdint.h>
#include <driver/rtc_io.h>
#include <driver/adc.h>

namespace Prefs
{
  /****************************************************************************
   * Konstanten für das Programm +
   ***************************************************************************/
  //
  // Version des Speichers bei Versionen, wenn änderungen an speicher
  // muss die Version erhöht werden und im code reagiert werden
  //
  constexpr int32_t currentPrefsVersion = 1;
  //
  // Konfigurierbare Parameter und deren keys
  //
  constexpr char VERSION_STR[] = "prefs_version";                  //! Name fuer Version
  constexpr char SSID_STR[] = "ap_ssid";                           //! SSID name für WLAN-Accesspoint
  constexpr char DEFAULT_SSID[] = "CHAINOILER";                    //! SSID für WLAN-Accesspoint
  constexpr char AP_PASSWORD_STR[] = "ap_password";                //! Passwort für WLAN
  constexpr char DEFAULT_AP_PASSWORD[] = "password";               //! Passwort für WLAN
  constexpr char PULSE_PER_WEEL_ROUND_STR[] = "pulse_per_round";   //! Impulse per Radumdrehung
  constexpr double DEFAULT_PULSE_PER_WEEL_ROUND = 109.0;           //! Defaultwert für Reed, Honda Africa Twin 109
  constexpr char WHEEL_CIRCUM_FERENCE_STR[] = "curcum_ference";    //! Radumfang
  constexpr double DEFAULT_WHEEL_CIRCUM_FERENCE = 1.81;            //! Default Umfang Hinterrad
  constexpr char OIL_INTERVAL_STR[] = "oil_interval";              //! Öl interval in Metern
  constexpr double DEFAULT_OIL_INTERVAL = 4000.0;                  //! Schmierinterval 4000 Meter
  constexpr char RAIN_OIL_INTERVAL_FACTOR_STR[] = "rain_factor";   //! Streckenfaktor bei Regen
  constexpr double DEFAULT_RAIN_OIL_INTERVAL_FACTOR = 1.4;         //! wieviel mal gegenüber normal schmieren
  constexpr char CROSS_OIL_INTERVAL_FACTOR_STR[] = "cross_factor"; //! Streckenfaktor bei cross
  constexpr double DEFAULT_CROSS_OIL_INTERVAL_FACTOR = 6.0;        //! wieviel mal öfter beim Crossen schmieren
  constexpr char SPEED_PROGRESSION_FACTOR_STR[] = "speed_progres"; //! Geschwindigkeits regression
  constexpr double DEFAULT_SPEED_PROGRESSION_FACTOR = -0.76;       //! der Faktor für die Berechnung der Progression
  constexpr char THRESHOLD_RAIN_SENSOR_STR[] = "rain_threshold";   //! Regensensor Schwellwert
  constexpr uint32_t DEFAULT_THRESHOLD_RAIN_SENSOR = 512;          //! Schwellenwert für Regen TODO: Hysterese zum Abschalten
  constexpr char PUMP_LED_LITHGING_TIME_STR[] = "pump_led_time";   //! Zeit für die Aktivitätsanzeige
  constexpr uint32_t DEFAULT_PUMP_LED_LITHGING_TIME = 300;         //! Leuchtzeit der Pumpen-LED
  //
  // Ein-/Ausgänge GPIO, nicht konfigurierbar
  //
  constexpr gpio_num_t LED_REED_CONTROL = GPIO_NUM_26;         //! Kontrolle für Tacho und Reed Impuls
  constexpr gpio_num_t LED_CONTROL = GPIO_NUM_33;              //! LED für Kontrolle, WLAN, CROSS
  constexpr gpio_num_t LED_RAIN = GPIO_NUM_34;                 //! Regen Betriebsanzeige
  constexpr gpio_num_t LED_PUMP = GPIO_NUM_35;                 //! Aktivitätsanzeige Pumpe
  constexpr gpio_num_t OUTPUT_PUMP_CONTROL = GPIO_NUM_45;      //! Betätigt die Punpe, 20ms Stöße
  constexpr gpio_num_t OUTPUT_RAIN_SW_01 = GPIO_NUM_42;        //! Schaltet Regensensor 01
  constexpr gpio_num_t OUTPUT_RAIN_SW_02 = GPIO_NUM_41;        //! Schaltet Regensensor 02
  constexpr gpio_num_t INPUT_TACHO = GPIO_NUM_4;               //! Tachoimpuls / Reed Kontakt
  constexpr gpio_num_t INPUT_FUNCTION_SWITCH = GPIO_NUM_5;     //! Funktionstaster Cross/WLAN
  constexpr adc1_channel_t INPUT_ADC_RAIN_00 = ADC1_CHANNEL_0; //! Eingang Analogschnittstelle GPIO01- PIN01
  constexpr adc1_channel_t INPUT_ADC_RAIN_01 = ADC1_CHANNEL_1; //! Eingang Analogschnittstelle GPIO02 - PIN02
  //
  // Wie lange dauert das entprellen in Millisekunden
  //
  constexpr uint32_t DEBOUNCE_TIME_MS = 100;       //! Millisekunden zum entprellen
  constexpr uint32_t longClickTimeMs = 4000;       //! Milisekunden fuer Langen Click (WiFi ON)
  constexpr uint32_t TIME_TO_DEEP_SLEEP = 20000UL; // Zeit bis zum Sleep
  //
  // intervalle beim blinken in Millisekunden
  //
  constexpr uint32_t BLINK_LED_CONTROL_NORMAL_OFF = 3000;
  constexpr uint32_t BLINK_LED_CONTROL_NORMAL_ON = 10;
  constexpr uint32_t BLINK_LED_CONTROL_CROSS_OFF = 100;
  constexpr uint32_t BLINK_LED_CONTROL_CROSS_ON = 4000;
  constexpr uint32_t BLINK_LED_CONTROL_AP_OFF = 100;
  constexpr uint32_t BLINK_LED_CONTROL_AP_ON = 1000;
  constexpr uint32_t BLINK_LED_CONTROL_TEST_OFF = 60;
  constexpr uint32_t BLINK_LED_CONTROL_TEST_ON = 60;
  constexpr uint32_t BLINK_LED_ATTENTION_OFF = 35;
  constexpr uint32_t BLINK_LED_ATTENTION_ON = 35;
  constexpr uint32_t BLINK_LED_AWAKE_OFF = 150;
  constexpr uint32_t BLINK_LED_AWAKE_ON = 80;
  //
  // Konstanten
  //
  constexpr uint32_t MEASURE_ROUTE_METERS = 350; //! wie weit messe ich für die Geschwindigkeit

  //
  // Kommandos REST API
  //
  constexpr char CMD_SET_RGBW[] = "set_rgbw";
} // namespace Prefs
