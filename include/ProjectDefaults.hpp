#pragma once
#include <Arduino.h>

namespace Preferences
{
  /****************************************************************************
   * Konstanten für das Programm +
   ***************************************************************************/
  //
  // Version des Speichers bei Versionen, wenn änderungen an speicher
  // muss die Version erhöhr werden und im code reagiert werden
  //
  constexpr int prefsVersion = 1;
  //
  // Konfigurierbare Parameter
  //
  constexpr char defaultSSID[] = "CHAINOILER";    //! SSID für WLAN-Accesspoint
  constexpr char defaultPassword[] = "password";  //! Passwort für WLAN
  constexpr double pulsePerWeelRound = 1.0;       //! Defaultwert für Reed, Honda Africa Twin 109
  constexpr double weelCircumFerence = 1.996;     //! Default Umfang Hinterrad
  constexpr double normalOilInterval = 4000.0;    //! Schmierinterval 4000 Meter
  constexpr double rainOilIntervalFactor = 1.4;   //! wieviel mal gegenüber normal schmieren
  constexpr double crossOilIntervalFactor = 8.0;  //! wieviel mal öfter beim Crossen schmieren
  constexpr int threshodRainSensor = 512;         //! Schwellenwert für Regen TODO: Hysterese zum Abschalten
  //
  // die Schlüsselnamen für den Speicher
  // Einstelungen für den Controller
  //
  constexpr char wasInitKey[] = "wasInit";  //! wurde schon initialisiert?

  //
  // Ausgänge GPIO, nicht konfigurierbar
  //
  constexpr uint8_t LED_INTERNAL = 2;   //! Blaue, interne LED
  constexpr uint8_t LED_CONTROL = 12;   //! LED für Kontrolle, WLAN, CROSS
  constexpr uint8_t LED_RAIN = 13;      //! Regen Betriebsanzeige
  constexpr uint8_t LED_PUMP = 14;      //! Aktivitätsanzeige Pumpe
  constexpr uint8_t PUMP_CONTROL = 15;  //! Betätigt die Punpe, 20ms Stöße
  //
  // Eingänge GPIO, nicht konfigurierbar
  //
  constexpr uint8_t INPUT_TACHO = 16;           //! Tachoimpuls / Reed Kontakt
  constexpr uint8_t INPUT_FUNCTION_SWITCH = 4;  //! Funktionstaster Cross/WLAN
  constexpr uint8_t INPUT_RAIN_SWITCH = 5;      //! Funktionstaster Regen (optional)
  constexpr uint8_t INPUT_ANALOG = A0;          //! Eingang Analogschnittstelle
  //
  // Wie lange dauert das entprellen in Millisekunden
  //
  constexpr uint32_t deBounceTimeMs = 100;    //! Millisekunden zum entprellen
  constexpr uint32_t longClickTimeMs = 4000;  //! Milisekunden fuer Langen Click (WiFi ON)
  //
  // intervalle beim blinken
  //
  constexpr uint32_t BLINK_LED_CONTROL_NORMAL_OFF = 3000;
  constexpr uint32_t BLINK_LED_CONTROL_NORMAL_ON = 10;
  constexpr uint32_t BLINK_LED_CONTROL_CROSS_OFF = 100;
  constexpr uint32_t BLINK_LED_CONTROL_CROSS_ON = 4000;
  constexpr uint32_t BLINK_LED_CONTROL_AP_OFF = 100;
  constexpr uint32_t BLINK_LED_CONTROL_AP_ON = 1000;
  constexpr uint32_t BLINK_LED_CONTROL_TEST_OFF = 60;
  constexpr uint32_t BLINK_LED_CONTROL_TEST_ON = 60;

  //
  // Kommandos REST API
  //
  constexpr char CMD_SET_RGBW[] = "set_rgbw";

}  // namespace Preferences
