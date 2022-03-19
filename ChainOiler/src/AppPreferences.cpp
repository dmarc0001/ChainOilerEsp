#include "AppPreferences.hpp"
#include "ProjectDefaults.hpp"
#include <cmath>
#include <esp_log.h>

namespace Prefs
{
  const char *Preferences::serialStr = "20220319-141445-build-1039";
  const std::string Preferences::serialString = std::string(Preferences::serialStr);
  const char *Preferences::tag{"Preferences"};                              //! tag fürs debug logging
  nvs_handle_t Preferences::nvs_handle{0U};                                 //! handle für NVS
  bool Preferences::isInit{false};                                          //! Objekt initialisiert?
  uint32_t Preferences::version{INVALID_VERSION};                           //! Version der Einstellungen
  std::string Preferences::ssid{DEFAULT_SSID};                              //! SSID des Accespoint
  std::string Preferences::ap_passwd{DEFAULT_AP_PASSWORD};                  //! Password des Accesspoint
  uint8_t Preferences::ap_cannel{DEFAULT_AP_CHANNEL};                       //! Kanal des WiFi AP
  uint8_t Preferences::ap_max_connections{DEFAULT_AP_MAX_CONNECTIONS};      //! maximale Verbindungen wlan AP
  float Preferences::pulsePerRound{DEFAULT_PULSE_PER_WEEL_ROUND};           //! impulse per Radumdrehung
  float Preferences::circumFerence{DEFAULT_WHEEL_CIRCUM_FERENCE};           //! Radumfang für Tacho
  float Preferences::oilInterval{DEFAULT_OIL_INTERVAL};                     //! Das Intrerval für die Ölung
  float Preferences::rainFactor{DEFAULT_RAIN_OIL_INTERVAL_FACTOR};          //! Streckenfaktor bei Regen
  float Preferences::crossFactor{DEFAULT_CROSS_OIL_INTERVAL_FACTOR};        //! Streckenfaktor beim crossen
  float Preferences::speedProgression{DEFAULT_SPEED_PROGRESSION_FACTOR};    //! Mehr öl bei höherer Geschwindigkeit
  uint32_t Preferences::rainSensorThreshold{DEFAULT_THRESHOLD_RAIN_SENSOR}; //! Sensor Schwellwert
  uint64_t Preferences::pumpLedTimeout{DEFAULT_PUMP_LED_LITHGING_TIME};     //! wie lange leuchtet die LED nach
  volatile uint64_t Preferences::lastTachoPulse{0};                         //! wann traten die letzten Tachoinformationen auf
  volatile fClick Preferences::controlSwitchAction{fClick::NONE};           //! welche Aktion des buttons liegt an
  volatile fClick Preferences::rainSwitchAction{fClick::NONE};              //! welche aktion des buttons liegt an
  volatile uint8_t Preferences::pumpCycles{0};                              //! wie viele pumpenzyklen sollen erfolgen(setzten startet pumpe)
  volatile bool Preferences::pumpAction{false};                             //! marker für neue Pumpenzyklen, pumpCycles ist zu schnell auf 0 für LED
  opMode Preferences::appOpMode{opMode::AWAKE};                             //! In welchem Zustand ist das Programm
  float Preferences::currentSpeedMeterPerSec{0.0F};                         //! aktuelle Geschwindigkeit
  float Preferences::currentRouteLenPastOil{0.0F};                          //! Wegstrecke nach dem Ölen
  double Preferences::ackumulatedRouteLen{0.0D};                            //! Akkumulierte Wegstrecke
  bool Preferences::isAttentionFlag{false};                                 //! Flag für Ankündigung/Achtung (soll blinken auslösen)
  portMUX_TYPE Preferences::oilCycleMutex;                                  //! Mutex für zugriff auf Olpumpen zyklen

  /**
   * @brief gib version als string zurück
   *
   * @return const std::string&
   */
  const std::string &Preferences::getVersion()
  {
    return (Preferences::serialString);
  }

  /**
   * initialisiere den Speicher zur Benutzung
   */
  void Preferences::init()
  {
    Preferences::oilCycleMutex = portMUX_INITIALIZER_UNLOCKED;
    uint32_t tmp_vers = INVALID_VERSION;
    //
    // Initialize NVS
    //
    ESP_LOGI(tag, "init NVM...");
    // nvs_flash_erase_partition(PREFS_PARTITION_LABEL);
    esp_err_t err = nvs_flash_init_partition(PREFS_PARTITION_LABEL);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_LOGE(tag, "NVM init failed, erase nvm flash...");
      //
      // Partition ist nicht okay, wird geloescht
      // Wiederhole nvs_flash_init
      //
      ESP_ERROR_CHECK(nvs_flash_erase_partition(PREFS_PARTITION_LABEL));
      err = nvs_flash_init_partition(PREFS_PARTITION_LABEL);
      //
      // ging ales gut?
      //
      ESP_ERROR_CHECK(err);
      Preferences::isInit = true;
    }
    else
    {
      //
      // ging ales gut?
      //
      ESP_ERROR_CHECK(err);
      Preferences::isInit = true;
    }
    ESP_LOGI(tag, "init NVM...done!");
    //
    // oeffnen
    //
    ESP_LOGD(tag, "open NVM...");
    err = nvs_open_from_partition(PREFS_PARTITION_LABEL, PREFS_PARTITION, NVS_READONLY, &Preferences::nvs_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGD(tag, "open NVM...done");
    }

    // Preferences::makeDefaults();

    ESP_LOGD(tag, "====================================================================");
    ESP_LOGD(tag, "read preferences version from NVM...");
    ESP_LOGD(tag, "preferences version from code is <%02d>...", CURRENT_PREFS_VERSION);
    ESP_LOGD(tag, "preferences wrong version code is <%02d>...", tmp_vers);
    tmp_vers = Preferences::getIntValue(PREFS_VERSION_STR, INVALID_VERSION);
    ESP_LOGD(tag, "preferences readed version from NVM is <%02d>...", tmp_vers);
    ESP_LOGD(tag, "====================================================================");
    if ((tmp_vers == INVALID_VERSION) || (CURRENT_PREFS_VERSION != tmp_vers))
    {
      //
      // Neue version der Preferenzen => Alles neu machen
      //
      ESP_LOGW(tag, "new prefs version, make defaults...");
      Preferences::makeDefaults();
    }
    else
    {
      ESP_LOGD(tag, "known prefs version, read from NVS...");
    }
    Preferences::readPreferences();
    // nvs schliessen
    nvs_close(Preferences::nvs_handle);
    nvs_flash_deinit_partition(PREFS_PARTITION_LABEL);
    Preferences::isInit = false;
  }

  /**
   * Speicher schliessen
   */
  void Preferences::close()
  {
    ESP_LOGD(tag, "close preferences...");
    if (Preferences::isInit)
    {
      nvs_close(Preferences::nvs_handle);
      nvs_flash_deinit_partition(PREFS_PARTITION_LABEL);
    }
    Preferences::isInit = false;
  }

  /**
   * alles loeschen
   */
  void Preferences::format()
  {
    ESP_LOGD(tag, "format/erase NVM...");
    Preferences::close();
    ESP_ERROR_CHECK(nvs_flash_erase());
    Preferences::init();
  }

  /**
   * @brief schreibe SSID
   *
   * @param _ssid
   */
  void Preferences::setApSSID(std::string _ssid)
  {
    Preferences::ssid = _ssid;
    // max 32 Zeichen lang
    if (Preferences::ssid.size() > 32)
    {
      Preferences::ssid.resize(32);
    }
    Preferences::setStringValue(DEFAULT_SSID, _ssid.c_str());
  }

  /**
   * @brief Lies SSID
   *
   * @return std::string
   */
  std::string Preferences::getApSSID()
  {
    return Preferences::ssid;
  }

  /**
   * @brief Schreibe SSID Passwort
   *
   * @param _passwd
   */
  void Preferences::setApPasswd(std::string _passwd)
  {
    Preferences::ap_passwd = _passwd;
    // max 64 Zeichen lang
    if (Preferences::ap_passwd.size() > 64)
    {
      Preferences::ap_passwd.resize(64);
    }
    Preferences::setStringValue(AP_PASSWORD_STR, _passwd.c_str());
  }

  /**
   * @brief lies SSID Passwort
   *
   * @return std::string
   */
  std::string Preferences::getApPasswd()
  {
    return Preferences::ap_passwd;
  }

  /**
   * @brief lies den WiFi Kanal aus
   *
   * @return uint8_t
   */
  uint8_t Preferences::getWiFiChannel()
  {
    return Preferences::ap_cannel;
  }

  /**
   * @brief setzte den WiFi Kanal
   *
   * @param ch
   */
  void Preferences::setWiFiChannel(uint8_t ch)
  {
    Preferences::ap_cannel = ch;
  }

  /**
   * @brief wie viele Verbindungen gleichzeitig max
   *
   * @return uint8_t
   */
  uint8_t Preferences::getMaxConnections()
  {
    return Preferences::ap_max_connections;
  }

  /**
   * @brief setze anzahl gleichzeitiger Verbindungen, maximal jedoch 10
   *
   * @param max
   */
  void Preferences::setMaxConnections(uint8_t max)
  {
    Preferences::ap_max_connections = max > 10 ? 10 : max;
  }

  /**
   * @brief setzte Impulse per Radumdrehung
   *
   * @param _val
   */
  void Preferences::setPulsePerRound(float _val)
  {
    Preferences::pulsePerRound = _val;
    Preferences::setFloatValue(PULSE_PER_WEEL_ROUND_STR, _val);
  }

  /**
   * @brief lese Tachoimpulse per Umdrehung
   *
   * @return float
   */
  float Preferences::getPulsePerRound()
  {
    return Preferences::pulsePerRound;
  }

  /**
   * @brief schreibe Radumfang
   *
   * @param _val
   */
  void Preferences::setCircumFerence(float _val)
  {
    Preferences::circumFerence = _val;
    Preferences::setFloatValue(WHEEL_CIRCUM_FERENCE_STR, _val);
  }

  /**
   * @brief lies Radumfang
   *
   * @return float
   */
  float Preferences::getCircumFerence()
  {
    return Preferences::circumFerence;
  }

  /**
   * @brief schreibe Ölintervall normal als Entfernung in Metern
   *
   * @param _val
   */
  void Preferences::setOilInterval(float _val)
  {
    Preferences::oilInterval = _val;
    Preferences::setFloatValue(OIL_INTERVAL_STR, _val);
  }

  /**
   * @brief lese Normal Ölinterval in Metern
   *
   * @return float
   */
  float Preferences::getOilInterval()
  {
    return Preferences::oilInterval;
  }

  /**
   * @brief gib den Faktor für Ölintervall bei Regen an
   *
   * @param _val
   */
  void Preferences::setRainFactor(float _val)
  {
    Preferences::rainFactor = _val;
    Preferences::setFloatValue(RAIN_OIL_INTERVAL_FACTOR_STR, _val);
  }

  /**
   * @brief lies den Faktor für Ölintervall bei Regen
   *
   * @return float
   */
  float Preferences::getRainFactor()
  {
    return Preferences::rainFactor;
  }

  /**
   * @brief schreibe den Faktor für Ölintervall beim crossen
   *
   * @param _val
   */
  void Preferences::setCrossFactor(float _val)
  {
    Preferences::crossFactor = _val;
    Preferences::setFloatValue(CROSS_OIL_INTERVAL_FACTOR_STR, _val);
  }

  /**
   * @brief gib den Faktor für Ölintervall beim Crossen
   *
   * @return float
   */
  float Preferences::getCrossFactor()
  {
    return Preferences::crossFactor;
  }

  /**
   * @brief setze den Faktor für Geschwindigkeitsabhängige mehr-Ölung
   *
   * @param _val
   */
  void Preferences::setSpeedProgression(float _val)
  {
    Preferences::speedProgression = _val;
    Preferences::setFloatValue(SPEED_PROGRESSION_FACTOR_STR, _val);
  }

  /**
   * @brief lies den Faktor für mehr öl abhängig von der Geschwindigkeit
   *
   * @return float
   */
  float Preferences::getSpeedProgression()
  {
    return Preferences::speedProgression;
  }

  /**
   * @brief schreibe den Sensor Schwellenwert des Regensensors
   *
   * @param _val
   */
  void Preferences::setSensorThreshold(int _val)
  {
    Preferences::rainSensorThreshold = _val;
    Preferences::setIntValue(THRESHOLD_RAIN_SENSOR_STR, static_cast<uint32_t>(_val));
  }

  /**
   * @brief lies den Regensensor Schwellenwert
   *
   * @return int
   */
  int Preferences::getSensorThreshold()
  {
    return Preferences::rainSensorThreshold;
  }

  /**
   * @brief schreibe timeout für pumpen LED in us
   *
   * @param _tm
   */
  void Preferences::setPumpLedTimeout(uint64_t _tm)
  {
    Preferences::pumpLedTimeout = _tm;
    Preferences::setIntValue(PUMP_LED_LITHGING_TIME_STR, _tm);
  }

  /**
   * @brief lese timeout für pumpen LED in us
   *
   * @return uint32_t
   */
  uint64_t Preferences::getPumpLedTimeout()
  {
    return Preferences::pumpLedTimeout;
  }

  /**
   * @brief erzeuge/schreibe Voreinstellungen
   *
   */
  void Preferences::makeDefaults()
  {
    esp_err_t err;
    // nvs_handle_t nvs_local_handle{Preferences::nvs_handle};
    //
    // debuggen
    //
    ESP_LOGD(tag, "=================================================");
    ESP_LOGD(tag, "make default propertys...");
    ESP_LOGD(tag, "=================================================");
    //
    // RW öffnen
    //
    err = nvs_open_from_partition(PREFS_PARTITION_LABEL, PREFS_PARTITION, NVS_READWRITE, &Preferences::nvs_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGD(tag, "open NVM...done");
    }
    // lösche alle alten Werte
    ESP_LOGD(tag, "erase old entrys...");
    nvs_erase_all(Preferences::nvs_handle);
    err = nvs_commit(Preferences::nvs_handle);
    // dummy
    Preferences::setIntValue("dummy", static_cast<uint32_t>(123456789U));
    // SSID
    Preferences::setStringValue(SSID_STR, DEFAULT_SSID);
    // AP Passwort
    Preferences::setStringValue(AP_PASSWORD_STR, DEFAULT_AP_PASSWORD);
    // AP Kanal
    Preferences::setIntValue(AP_CHANNEL_STR, DEFAULT_AP_CHANNEL);
    // AP max connections
    Preferences::setIntValue(AP_MAX_CONNECTIONS_STR, DEFAULT_AP_MAX_CONNECTIONS);
    // pulses per round
    Preferences::setFloatValue(PULSE_PER_WEEL_ROUND_STR, DEFAULT_PULSE_PER_WEEL_ROUND);
    // Radumfang
    Preferences::setFloatValue(WHEEL_CIRCUM_FERENCE_STR, DEFAULT_WHEEL_CIRCUM_FERENCE);
    // Öl interval
    Preferences::setFloatValue(OIL_INTERVAL_STR, DEFAULT_OIL_INTERVAL);
    // Faktor für Regen
    Preferences::setFloatValue(RAIN_OIL_INTERVAL_FACTOR_STR, DEFAULT_RAIN_OIL_INTERVAL_FACTOR);
    // Cross Faktor
    Preferences::setFloatValue(CROSS_OIL_INTERVAL_FACTOR_STR, DEFAULT_CROSS_OIL_INTERVAL_FACTOR);
    // Speed progression
    Preferences::setFloatValue(SPEED_PROGRESSION_FACTOR_STR, DEFAULT_SPEED_PROGRESSION_FACTOR);
    // sensor schwellwert
    Preferences::setIntValue(THRESHOLD_RAIN_SENSOR_STR, DEFAULT_THRESHOLD_RAIN_SENSOR);
    // Pumpen nachleuchten
    Preferences::setIntValue(PUMP_LED_LITHGING_TIME_STR, DEFAULT_PUMP_LED_LITHGING_TIME);
    // absolute Wegstrecke
    Preferences::setDoubleValue(ABSOLUTE_PATH_LEN_STR, 0.0D);
    // Version
    Preferences::setIntValue(PREFS_VERSION_STR, CURRENT_PREFS_VERSION);
    //
    // Commit written values.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    //
    ESP_LOGD(tag, "make default propertys...done");
    ESP_LOGD(tag, "commit updates in NVS...");
    err = nvs_commit(Preferences::nvs_handle);
    ESP_ERROR_CHECK(err);
    ESP_LOGD(tag, "commit updates in NVS...done");
    // das RW Handle schliessen
    nvs_close(Preferences::nvs_handle);
    ESP_LOGD(tag, "================== R E B O O T ===============================");
#ifdef DEBUG
    vTaskDelay(pdMS_TO_TICKS(3000));
#else
    vTaskDelay(pdMS_TO_TICKS(50));
#endif
    esp_restart();
    /*
    // Handle zurückholen
    Preferences::nvs_handle = nvs_local_handle;
    nvs_stats_t nvs_stats;
    nvs_get_stats(PREFS_PARTITION_LABEL, &nvs_stats);
    printf("Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n", nvs_stats.used_entries, nvs_stats.free_entries,
           nvs_stats.total_entries);
    ESP_LOGD(tag, "=================================================");
    */
  }

  /**
   * @brief sichere Einstellungen in den NVS
   *
   * @return true
   * @return false
   */
  bool Preferences::writePreferences()
  {
    esp_err_t err;
    nvs_handle_t nvs_local_handle{Preferences::nvs_handle};
    bool wasNVSInit = Preferences::isInit;
    //
    // zum Zeitpunkt des Aufrufes sollte der NVM geschlossen sein
    // aber zur Sicherheit erst checken ob dem so ist
    //
    if (!Preferences::isInit)
    {
      ESP_LOGI(tag, "init NVM...");
      esp_err_t err = nvs_flash_init_partition(PREFS_PARTITION_LABEL);
      if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
      {
        ESP_LOGE(tag, "NVM init failed, erase nvm flash...");
        //
        // Partition ist nicht okay, wird geloescht
        // Wiederhole nvs_flash_init
        //
        if (ESP_OK != nvs_flash_erase_partition(PREFS_PARTITION_LABEL))
        {
          ESP_LOGE(tag, "NVM flash erase (while write preferences) failed!");
          return false;
        }
        if (ESP_OK != nvs_flash_init_partition(PREFS_PARTITION_LABEL))
        {
          ESP_LOGE(tag, "NVM 2nd flash init (while write preferences) failed!");
          return false;
        }
        Preferences::isInit = true;
      }
      else
      {
        //
        // ging ales gut?
        //
        if (ESP_OK != err)
        {
          ESP_LOGE(tag, "NVM flash init (while write preferences) failed!");
          return false;
        }
        Preferences::isInit = true;
      }
      ESP_LOGI(tag, "init NVM...done!");
    }
    //
    // oeffnen
    //
    ESP_LOGD(tag, "open NVM...");
    err = nvs_open_from_partition(PREFS_PARTITION_LABEL, PREFS_PARTITION, NVS_READWRITE, &Preferences::nvs_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(tag, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
      return false;
    }
    else
    {
      ESP_LOGD(tag, "open NVM...done");
    }
    //
    // debuggen
    //
    ESP_LOGD(tag, "=================================================");
    ESP_LOGD(tag, "save current propertys...");
    ESP_LOGD(tag, "=================================================");
    // dummy
    Preferences::setIntValue("dummy", static_cast<uint32_t>(123456789U));
    // SSID
    Preferences::setStringValue(SSID_STR, Preferences::ssid);
    // AP Passwort
    Preferences::setStringValue(AP_PASSWORD_STR, Preferences::ap_passwd);
    // AP Kanal
    Preferences::setIntValue(AP_CHANNEL_STR, Preferences::ap_cannel);
    // AP max connections
    Preferences::setIntValue(AP_MAX_CONNECTIONS_STR, Preferences::ap_max_connections);
    // pulses per round
    Preferences::setFloatValue(PULSE_PER_WEEL_ROUND_STR, Preferences::pulsePerRound);
    // Radumfang
    Preferences::setFloatValue(WHEEL_CIRCUM_FERENCE_STR, Preferences::circumFerence);
    // Öl interval
    Preferences::setFloatValue(OIL_INTERVAL_STR, Preferences::oilInterval);
    // Faktor für Regen
    Preferences::setFloatValue(RAIN_OIL_INTERVAL_FACTOR_STR, Preferences::rainFactor);
    // Cross Faktor
    Preferences::setFloatValue(CROSS_OIL_INTERVAL_FACTOR_STR, Preferences::crossFactor);
    // Speed progression
    Preferences::setFloatValue(SPEED_PROGRESSION_FACTOR_STR, Preferences::speedProgression);
    // sensor schwellwert
    Preferences::setIntValue(THRESHOLD_RAIN_SENSOR_STR, Preferences::rainSensorThreshold);
    // Pumpen nachleuchten
    Preferences::setIntValue(PUMP_LED_LITHGING_TIME_STR, Preferences::pumpLedTimeout);
    // absolute Wegstrecke
    Preferences::setDoubleValue(ABSOLUTE_PATH_LEN_STR, Preferences::ackumulatedRouteLen);
    // Version
    Preferences::setIntValue(PREFS_VERSION_STR, CURRENT_PREFS_VERSION);
    //
    // Commit written values.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    //
    ESP_LOGD(tag, "commit updates in NVS...");
    err = nvs_commit(Preferences::nvs_handle);
    if (err != ESP_OK)
    {
      ESP_LOGE(tag, "NVM commit (while write preferences) failed!");
    }
    ESP_LOGD(tag, "commit updates in NVS...done");
    // das RW Handle schliessen
    nvs_close(Preferences::nvs_handle);
    // Handle zurückholen
    Preferences::nvs_handle = nvs_local_handle;

    nvs_stats_t nvs_stats;
    nvs_get_stats(PREFS_PARTITION_LABEL, &nvs_stats);
    printf("Count: UsedEntries = (%d), FreeEntries = (%d), AllEntries = (%d)\n", nvs_stats.used_entries, nvs_stats.free_entries,
           nvs_stats.total_entries);

    ESP_LOGD(tag, "=================================================");
    //
    // falls NVS nicht ofen war (sollte standartfall sein)
    // NVS wieder deinitialisieren, Resourcen frei geben
    //
    if (!wasNVSInit)
    {
      // nvs wieder schliessen
      nvs_close(Preferences::nvs_handle);
      nvs_flash_deinit_partition(PREFS_PARTITION_LABEL);
      Preferences::isInit = false;
    }
    if (err != ESP_OK)
      return false;
    return true;
  }

  /**
   * @brief lese aktuelle Einstellungen aus dem NVS
   *
   */
  void Preferences::readPreferences()
  {
    //
    // alle Einstellugnen einlesen
    //
    ESP_LOGD(tag, "read all preferences...");
    Preferences::version = Preferences::getIntValue(PREFS_VERSION_STR, INVALID_VERSION);
    Preferences::ssid = Preferences::getStringValue(SSID_STR);
    Preferences::ap_passwd = Preferences::getStringValue(AP_PASSWORD_STR);
    Preferences::ap_cannel = static_cast<uint8_t>(Preferences::getIntValue(AP_CHANNEL_STR, DEFAULT_AP_CHANNEL));
    Preferences::ap_max_connections =
        static_cast<uint8_t>(Preferences::getIntValue(AP_MAX_CONNECTIONS_STR, DEFAULT_AP_MAX_CONNECTIONS));
    Preferences::pulsePerRound = Preferences::getFloatValue(PULSE_PER_WEEL_ROUND_STR, DEFAULT_PULSE_PER_WEEL_ROUND);
    Preferences::circumFerence = Preferences::getFloatValue(WHEEL_CIRCUM_FERENCE_STR, DEFAULT_WHEEL_CIRCUM_FERENCE);
    Preferences::oilInterval = Preferences::getFloatValue(OIL_INTERVAL_STR, DEFAULT_OIL_INTERVAL);
    Preferences::rainFactor = Preferences::getFloatValue(RAIN_OIL_INTERVAL_FACTOR_STR, DEFAULT_RAIN_OIL_INTERVAL_FACTOR);
    Preferences::crossFactor = Preferences::getFloatValue(CROSS_OIL_INTERVAL_FACTOR_STR, DEFAULT_CROSS_OIL_INTERVAL_FACTOR);
    Preferences::speedProgression = Preferences::getFloatValue(SPEED_PROGRESSION_FACTOR_STR, DEFAULT_SPEED_PROGRESSION_FACTOR);
    Preferences::rainSensorThreshold = Preferences::getIntValue(THRESHOLD_RAIN_SENSOR_STR, DEFAULT_THRESHOLD_RAIN_SENSOR);
    Preferences::pumpLedTimeout = Preferences::getIntValue(PUMP_LED_LITHGING_TIME_STR, DEFAULT_PUMP_LED_LITHGING_TIME);
    Preferences::ackumulatedRouteLen = Preferences::getDoubleValue(ABSOLUTE_PATH_LEN_STR, 0.0D);
    ESP_LOGD(tag, "read all preferences...done");
  }

  /**
   * @brief Berechne Anzahl der Tachoimpulse für 100 Meter
   *
   * @return int16_t
   */
  uint16_t Preferences::getPulsesFor100Meters()
  {
    float val100Meters = (100.0 / circumFerence) * pulsePerRound;
    uint16_t count = static_cast<uint16_t>(std::ceil(val100Meters));
    ESP_LOGI(tag, "==== pulses for 100 meters: <%06d>", count);
    return count;
  }

  /**
   * @brief Berechne Anzahl der Tachoimpulse für 10 Meter (für Tacho/speed)
   *
   * @return int16_t
   */
  uint16_t Preferences::getPulsesFor25Meters()
  {
    float val10Meters = (25.0 / circumFerence) * pulsePerRound;
    uint16_t count = static_cast<uint16_t>(std::ceil(val10Meters));
    ESP_LOGI(tag, "==== pulses for 25 meters: <%06d>", count);
    return (count >= 1) ? count : 1;
  }

  /**
   * @brief berechne die kleinste Impulslänge bei den Eisntellungen für Tacho
   *
   * @return uint16_t
   */
  uint16_t Preferences::getMinimalPulseLength()
  {
    //
    // pulsePerRound / circumFence == Pulse per Meter
    // 70.0 Meter per Sekunde ist 250 km/h
    // ich suche die Halbimpuszeit * 2
    // also (pulsePerMeter * speed * 2) hoch -1
    // Faktor 1000.0 bedeutet millisekunden Berechnung
    float pulsLength = 1000.0 / ((pulsePerRound / circumFerence) * 70.0 * 2.0);
    //
    // Die Zählerfrequenz  zum Filtern ins 80 MHz
    // also muss der Filterwert kleiner gleich pulsLength sein.
    //
    float pulsSystem = 1000.0 / 80000000;
    //
    // wie viele Pulse für pulsLengt
    //
    float pulseCount = std::floor(pulsLength / pulsSystem);
    //
    // entweder korrrekter Wert oder der fast Maximale
    //
    return pulseCount > 1022 ? 1023 : static_cast<uint16_t>(pulseCount);
  }

  /**
   * @brief in welchem Status ist das Programm (Zustände nach Automatentheorie...)
   *
   * @return opMode
   */
  opMode Preferences::getAppMode()
  {
    return Preferences::appOpMode;
  }

  /**
   * @brief setzte Zustand/Modus des Programmes
   *
   * @param _mode
   */
  void Preferences::setAppMode(opMode _mode)
  {
    //
    // TODO: crossmode: Strecke anpassen
    // TODO: normalmode: Strecke anpassen
    // TODO: regenmode: Strecke anpassen
    // TODO: ap mode, komplett umbauen
    //
    if (_mode == Preferences::appOpMode)
    {
      // nix zu tun, war schon
      return;
    }
    //
    // Accesspoint Mode umschalten?
    //
    if (_mode == opMode::APMODE || Preferences::appOpMode == opMode::APMODE)
    {
      if (_mode == opMode::APMODE)
      {
        //
      }
      else
      {
        // evtl booten oder AP und webserver löschen
        esp_restart();
      }
    }
    Preferences::appOpMode = _mode;
  }

  /**
   * @brief war eine/welche Aktion des Regenschalters
   *
   * @return fClick
   */
  fClick Preferences::getRainSwitchAction()
  {
    return Preferences::rainSwitchAction;
  }

  /**
   * @brief setzte/lösche Aktion des Regenschalters
   *
   * @param _stat
   */
  void Preferences::setRainSwitchAction(fClick _stat)
  {
    Preferences::rainSwitchAction = _stat;
  }

  /**
   * @brief war eine/weelche Aktion des Control Schalters
   *
   * @return fClick
   */
  fClick Preferences::getControlSwitchAction()
  {
    return Preferences::controlSwitchAction;
  }

  /**
   * @brief setze/lösche Aktion des Control Schalters
   *
   * @param _stat
   */
  void Preferences::setControlSwitchAction(fClick _stat)
  {
    Preferences::controlSwitchAction = _stat;
  }

  /**
   * @brief trage aktuzelle Geschwindigkeit ein
   *
   * @param _spd
   */
  void Preferences::setCurrentSpeed(float _spd)
  {
    //
    // TODO: speed Progression berechnen
    //
    Preferences::currentSpeedMeterPerSec = _spd;
  }

  /**
   * @brief letzte gemessene Gescheindigkeit
   *
   * @return float
   */
  float Preferences::getCurrentSpeed()
  {
    return Preferences::currentSpeedMeterPerSec;
  }

  /**
   * @brief schreibe die Strecke nach der letzten Ölung?
   *
   * @param _routeLen
   */
  void Preferences::setRouteLenPastOil(float _routeLen)
  {
    Preferences::currentRouteLenPastOil = _routeLen;
  }

  /**
   * @brief Addiere zur Strecke einen Wert
   *
   * @param _routeLen
   */
  void Preferences::addRouteLenPastOil(float _routeLen)
  {
    Preferences::currentRouteLenPastOil += _routeLen;
    Preferences::ackumulatedRouteLen += static_cast<double>(_routeLen);
  }

  /**
   * @brief Wie lang ist die bisherige Strecke nach der Ölung?
   *
   * @return float
   */
  float Preferences::getRouteLenPastOil()
  {
    return Preferences::currentRouteLenPastOil;
  }

  double Preferences::getAckumulatedRouteLen()
  {
    return Preferences::ackumulatedRouteLen;
  }

  /**
   * @brief setzte das Achtung-Flag
   *
   * @param _flag
   */
  void Preferences::setAttentionFlag(bool _flag)
  {
    Preferences::isAttentionFlag = _flag;
  }

  /**
   * @brief lese das achtung flag
   *
   * @return true
   * @return false
   */
  bool Preferences::getAttentionFlag()
  {
    return Preferences::isAttentionFlag;
  }

  void Preferences::setPumpCycles(uint8_t _cycles)
  {
    portENTER_CRITICAL(&Preferences::oilCycleMutex);
    Preferences::pumpCycles = _cycles;
    Preferences::pumpAction = true;
    portEXIT_CRITICAL(&Preferences::oilCycleMutex);
  }

  void Preferences::addPumpCycles(uint8_t _cycles)
  {
    portENTER_CRITICAL(&Preferences::oilCycleMutex);
    Preferences::pumpCycles += _cycles;
    Preferences::pumpAction = true;
    portEXIT_CRITICAL(&Preferences::oilCycleMutex);
  }

  uint8_t Preferences::getPumpCycles()
  {
    uint8_t retval;
    portENTER_CRITICAL(&Preferences::oilCycleMutex);
    retval = Preferences::pumpCycles;
    portEXIT_CRITICAL(&Preferences::oilCycleMutex);
    return (retval);
  }

  //###########################################################################
  //###########################################################################
  //#### private functions                                                 ####
  //###########################################################################
  //###########################################################################
  uint8_t Preferences::getIntValue(const char *_name, uint8_t _defaultVal)
  {
    esp_err_t err;
    uint8_t val = _defaultVal;
    ESP_LOGD(tag, "read uint8 value <%s>...", _name);
    if (!Preferences::isInit)
      return (_defaultVal);
    err = nvs_get_u8(Preferences::nvs_handle, _name, &val);
    switch (err)
    {
    case ESP_OK:
      ESP_LOGD(tag, "read uint8 value <%s> = <%06u>...", _name, val);
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "read uint8 value <%s> is not initialized yet", _name);
      break;
    case ESP_ERR_NVS_INVALID_HANDLE:
      ESP_LOGE(tag, "read uint8 value <invalid handle>");
      break;
    case ESP_ERR_NVS_INVALID_NAME:
      ESP_LOGE(tag, "read uint8 value <invalid name>");
      break;
    case ESP_ERR_NVS_INVALID_LENGTH:
      ESP_LOGE(tag, "read uint8 value <invalid data length>");
      break;
    default:
      ESP_LOGE(tag, "error while reading value <%s>", _name);
      break;
    }
    return (_defaultVal);
  }

  bool Preferences::setIntValue(const char *_name, uint8_t _val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_u8(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "write uint8 name <%s> value <%06u>...", _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

  uint64_t Preferences::getIntValue(const char *_name, uint64_t _defaultVal)
  {
    esp_err_t err;
    uint64_t val = _defaultVal;
    ESP_LOGD(tag, "read uint64 value <%s>...", _name);
    if (!Preferences::isInit)
      return (_defaultVal);
    err = nvs_get_u64(Preferences::nvs_handle, _name, &val);
    switch (err)
    {
    case ESP_OK:
      ESP_LOGD(tag, "read uint64 value <%s> = <%06u>...", _name, static_cast<uint32_t>(val));
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "read uint64 value <%s> is not initialized yet", _name);
      break;
    case ESP_ERR_NVS_INVALID_HANDLE:
      ESP_LOGE(tag, "read uint64 value <invalid handle>");
      break;
    case ESP_ERR_NVS_INVALID_NAME:
      ESP_LOGE(tag, "read uint64 value <invalid name>");
      break;
    case ESP_ERR_NVS_INVALID_LENGTH:
      ESP_LOGE(tag, "read uint64 value <invalid data length>");
      break;
    default:
      ESP_LOGE(tag, "error while reading value <%s>", _name);
      break;
    }
    return (_defaultVal);
  }

  bool Preferences::setIntValue(const char *_name, uint64_t _val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_u64(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "write int32 name <%s> value <%06u>...", _name, static_cast<uint32_t>(_val));
    if (err == ESP_OK)
      return true;
    return false;
  }

  /**
   * @brief lese INT Variable aus dem NVS
   *
   * @param _name
   * @param defaultVal
   * @return int32_t
   */
  uint32_t Preferences::getIntValue(const char *_name, uint32_t _defaultVal)
  {
    esp_err_t err;
    uint32_t val = _defaultVal;
    ESP_LOGD(tag, "read uint32 value <%s>...", _name);
    if (!Preferences::isInit)
      return (_defaultVal);
    err = nvs_get_u32(Preferences::nvs_handle, _name, &val);
    switch (err)
    {
    case ESP_OK:
      ESP_LOGD(tag, "read uint32 value <%s> = <%06u>...", _name, val);
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "read uint32 value <%s> is not initialized yet", _name);
      break;
    case ESP_ERR_NVS_INVALID_HANDLE:
      ESP_LOGE(tag, "read uint32 value <invalid handle>");
      break;
    case ESP_ERR_NVS_INVALID_NAME:
      ESP_LOGE(tag, "read uint32 value <invalid name>");
      break;
    case ESP_ERR_NVS_INVALID_LENGTH:
      ESP_LOGE(tag, "read uint32 value <invalid data length>");
      break;
    default:
      ESP_LOGE(tag, "error while reading value <%s>", _name);
      break;
    }
    return (_defaultVal);
  }

  /**
   * @brief schreibe INT Variable in den NVS
   *
   * @param _name
   * @param _val
   * @return true
   * @return false
   */
  bool Preferences::setIntValue(const char *_name, uint32_t _val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_u32(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "write int32 name <%s> value <%06u>...", _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

  /**
   * @brief lese STRING aus dem NVS
   *
   * @param _name
   * @return std::string
   */
  std::string Preferences::getStringValue(const char *_name)
  {
    esp_err_t err;
    std::string retVal;
    char resultArray[64];
    size_t size = sizeof(resultArray);

    ESP_LOGD(tag, "read string value <%s>...", _name);
    if (!Preferences::isInit)
      return (nullptr);
    err = nvs_get_str(Preferences::nvs_handle, _name, &(resultArray[0]), &size);
    switch (err)
    {
    case ESP_OK:
      retVal = std::string(&(resultArray[0]));
      ESP_LOGD(tag, "read string value <%s> = <%s>...", _name, retVal.c_str());
      return (retVal);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "read string value <%s> is not initialized yet", _name);
      return (nullptr);
    default:
      ESP_LOGE(tag, "error while reading string value <%s>", _name);
      return (nullptr);
    }
    return (nullptr);
  }

  /**
   * @brief schreibe STRING in den NVS
   *
   * @param _name
   * @param _val
   * @return true
   * @return false
   */
  bool Preferences::setStringValue(const char *_name, const char *_val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_str(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "write string name <%s> value <%s>...", _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

  /**
   * @brief schreibe std::string in den NVS
   *
   * @param _name
   * @param _val
   * @return true
   * @return false
   */
  bool Preferences::setStringValue(const char *_name, const std::string &_val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_str(Preferences::nvs_handle, _name, _val.c_str());
    ESP_LOGD(tag, "write string name <%s> value <%s>...", _name, _val.c_str());
    if (err == ESP_OK)
      return true;
    return false;
  }

  /**
   * @brief lese float Variable aus dem NVS
   *
   * @param _name
   * @param _defaultValue
   * @return float
   */
  float Preferences::getFloatValue(const char *_name, float _defaultVal)
  {
    esp_err_t err;
    float val;
    size_t size = sizeof(val);
    //
    ESP_LOGD(tag, "read float value <%s>...", _name);
    if (!Preferences::isInit)
      return (_defaultVal);
    err = nvs_get_blob(Preferences::nvs_handle, _name, &val, &size);
    switch (err)
    {
    case ESP_OK:
      ESP_LOGD(tag, "read float value <%s> = <%f>...", _name, val);
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "read float value <%s> is not initialized yet", _name);
      return (_defaultVal);
    default:
      ESP_LOGE(tag, "error while reading float value <%s>", _name);
      return (_defaultVal);
    }
    return (_defaultVal);
  }

  /**
   * @brief schreibe FLOAT variable in den NVS
   *
   * @param _name
   * @param _val
   * @return true
   * @return false
   */
  bool Preferences::setFloatValue(const char *_name, float _val)
  {
    esp_err_t err;
    //
    if (!Preferences::isInit)
      return false;
    err = nvs_set_blob(Preferences::nvs_handle, _name, &_val, sizeof(_val));
    ESP_LOGD(tag, "write float <%s> value <%.2f>...", _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

  float Preferences::getDoubleValue(const char *_name, double _defaultVal)
  {
    esp_err_t err;
    double val;
    size_t size = sizeof(val);
    //
    ESP_LOGD(tag, "read double value <%s>...", _name);
    if (!Preferences::isInit)
      return (_defaultVal);
    err = nvs_get_blob(Preferences::nvs_handle, _name, &val, &size);
    switch (err)
    {
    case ESP_OK:
      ESP_LOGD(tag, "read double value <%s> = <%f>...", _name, static_cast<double>(val));
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "read double value <%s> is not initialized yet", _name);
      return (_defaultVal);
    default:
      ESP_LOGE(tag, "error while reading double value <%s>", _name);
      return (_defaultVal);
    }
    return (_defaultVal);
  }

  bool Preferences::setDoubleValue(const char *_name, double _val)
  {
    esp_err_t err;
    //
    if (!Preferences::isInit)
      return false;
    err = nvs_set_blob(Preferences::nvs_handle, _name, &_val, sizeof(_val));
    ESP_LOGD(tag, "write double <%s> value <%.2f>...", _name, static_cast<double>(_val));
    if (err == ESP_OK)
      return true;
    return false;
  }

} // namespace Prefs
