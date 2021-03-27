#include "AppPreferences.hpp"
#include "ProjectDefaults.hpp"
#include <cmath>

namespace Prefs
{
  const char *Preferences::serialStr = "20210327-110853-build-0474";
  const std::string Preferences::serialString = std::string(Preferences::serialStr);
  const char *Preferences::tag{"Preferences"};                                           //! tag fürs debug logging
  nvs_handle_t Preferences::nvs_handle{0};                                               //! handle für NVS
  bool Preferences::isInit{false};                                                       //! Objekt initialisiert?
  int32_t Preferences::version{currentPrefsVersion};                                     //! Version der Einstellungen
  std::string Preferences::ssid{DEFAULT_SSID};                                           //! SSID des Accespoint
  std::string Preferences::ap_passwd{DEFAULT_AP_PASSWORD};                               //! Password des Accesspoint
  float Preferences::pulsePerRound{DEFAULT_PULSE_PER_WEEL_ROUND};                        //! impulse per Radumdrehung
  float Preferences::circumFerence{DEFAULT_WHEEL_CIRCUM_FERENCE};                        //! Radumfang für Tacho
  float Preferences::oilInterval{DEFAULT_OIL_INTERVAL};                                  //! Das Intrerval für die Ölung
  float Preferences::rainFactor{DEFAULT_RAIN_OIL_INTERVAL_FACTOR};                       //! Streckenfaktor bei Regen
  float Preferences::crossFactor{DEFAULT_CROSS_OIL_INTERVAL_FACTOR};                     //! Streckenfaktor beim crossen
  float Preferences::speedProgression{DEFAULT_SPEED_PROGRESSION_FACTOR};                 //! Mehr öl bei höherer Geschwindigkeit
  int Preferences::rainSensorThreshold{static_cast<int>(DEFAULT_THRESHOLD_RAIN_SENSOR)}; //! Sensor Schwellwert
  uint64_t Preferences::pumpLedTimeout{DEFAULT_PUMP_LED_LITHGING_TIME};                  //! wie lange leuchtet die LED nach
  volatile uint64_t Preferences::lastTachoPulse{0};                                      //! wann traten die letzten Tachoinformationen auf
  volatile fClick Preferences::controlSwitchAction{fClick::NONE};                        //! welche Aktion des buttons liegt an
  volatile fClick Preferences::rainSwitchAction{fClick::NONE};                           //! welche aktion des buttons liegt an
  volatile uint8_t Preferences::pumpCycles{0};                                           //! wie viele pumpenzyklen sollen erfolgen(setzten startet pumpe)
  opMode Preferences::appOpMode{opMode::AWAKE};                                          //! In welchem Zustand ist das Programm
  float Preferences::currentSpeedMeterPerSec{0.0F};                                      //! aktuelle Geschwindigkeit
  float Preferences::currentRouteLenPastOil{0.0F};                                       //! Wegstrecke nach dem Ölen
  bool Preferences::isAttentionFlag{false};                                              //! Flag für Ankündigung/Achtung (soll blinken auslösen)

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
    //
    // Initialize NVS
    //
    ESP_LOGI(tag, "init NVM...");
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      //
      // Partition ist nicht okay, wird geloescht
      // Wiederhole nvs_flash_init
      //
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
      //
      // ging ales gut?
      //
      ESP_ERROR_CHECK(err);
      Preferences::isInit = true;
      Preferences::makeDefaults();
    }
    else
    {
      //
      // ging ales gut?
      //
      ESP_ERROR_CHECK(err);
      Preferences::isInit = true;
    }
    //
    // oeffnen
    //
    ESP_LOGD(tag, "open NVM...");
    err = nvs_open("new_mccoi", NVS_READWRITE, &Preferences::nvs_handle);
    if (err != ESP_OK)
    {
      printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGD(tag, "init NVM...done");
    }
    ESP_LOGD(tag, "read version from NVM...");
    int _vers = Preferences::getIntValue(VERSION_STR, -1);
    ESP_LOGI(tag, "Preferences version from NVM is <%02d>, current version is <%02d>...", _vers, Preferences::version);
    if (Preferences::version != _vers)
    {
      //
      // Neue version der Preferenzen => Alles neu machen
      //  TODO: abhängig vom Versionssprung entscheiden
      //
      Preferences::makeDefaults();
    }
    Preferences::readPreferences();
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
    }
    Preferences::isInit = false;
  }

  /**
   * alles loeschen
   */
  void Preferences::format()
  {
    ESP_LOGD(tag, "format NVM...");
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
    Preferences::ssid = _passwd;
    Preferences::setStringValue(AP_PASSWORD_STR, _passwd.c_str());
  }

  /**
   * @brief lies SSID PAsswort
   * 
   * @return std::string 
   */
  std::string Preferences::getApPasswd()
  {
    return Preferences::ap_passwd;
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
    //
    // debuggen
    //
    ESP_LOGD(tag, "make default propertys...");
    // Version
    Preferences::setIntValue(VERSION_STR, currentPrefsVersion);
    // SSID
    Preferences::setStringValue(SSID_STR, DEFAULT_SSID);
    // AP Passwort
    Preferences::setStringValue(AP_PASSWORD_STR, DEFAULT_AP_PASSWORD);
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
    Preferences::setIntValue(PUMP_LED_LITHGING_TIME_STR, static_cast<uint32_t>(DEFAULT_PUMP_LED_LITHGING_TIME));
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
    Preferences::version = Preferences::getIntValue(VERSION_STR, -1);
    Preferences::ssid = Preferences::getStringValue(SSID_STR);
    Preferences::ap_passwd = Preferences::getStringValue(AP_PASSWORD_STR);
    Preferences::pulsePerRound = Preferences::getFloatValue(PULSE_PER_WEEL_ROUND_STR, DEFAULT_PULSE_PER_WEEL_ROUND);
    Preferences::circumFerence = Preferences::getFloatValue(WHEEL_CIRCUM_FERENCE_STR, DEFAULT_WHEEL_CIRCUM_FERENCE);
    Preferences::oilInterval = Preferences::getFloatValue(OIL_INTERVAL_STR, DEFAULT_OIL_INTERVAL);
    Preferences::rainFactor = Preferences::getFloatValue(RAIN_OIL_INTERVAL_FACTOR_STR, DEFAULT_RAIN_OIL_INTERVAL_FACTOR);
    Preferences::crossFactor = Preferences::getFloatValue(CROSS_OIL_INTERVAL_FACTOR_STR, DEFAULT_CROSS_OIL_INTERVAL_FACTOR);
    Preferences::speedProgression = Preferences::getFloatValue(SPEED_PROGRESSION_FACTOR_STR, DEFAULT_SPEED_PROGRESSION_FACTOR);
    Preferences::rainSensorThreshold = static_cast<uint32_t>(Preferences::getIntValue(THRESHOLD_RAIN_SENSOR_STR, DEFAULT_THRESHOLD_RAIN_SENSOR));
    Preferences::pumpLedTimeout = static_cast<uint64_t>(Preferences::getIntValue(PUMP_LED_LITHGING_TIME_STR, DEFAULT_PUMP_LED_LITHGING_TIME));
    ESP_LOGD(tag, "read all preferences...done");
  }

  /**
   * @brief Berechne Anzahl der Tachoimpulse für 100 Meter
   * 
   * @return int16_t 
   */
  int16_t Preferences::getPulsesFor100Meters()
  {
    float val100Meters = (100.0 / circumFerence) * pulsePerRound;
    int16_t count = static_cast<int16_t>(std::ceil(val100Meters));
    ESP_LOGI(tag, "==== pulses for 100 meters: <%06d>", count);
    return count;
  }

  /**
   * @brief Berechne Anzahl der Tachoimpulse für 10 Meter (für Tacho/speed)
   * 
   * @return int16_t 
   */
  int16_t Preferences::getPulsesFor10Meters()
  {
    float val10Meters = (10.0 / circumFerence) * pulsePerRound;
    int16_t count = static_cast<int16_t>(std::ceil(val10Meters));
    ESP_LOGI(tag, "==== pulses for 10 meters: <%06d>", count);
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
    // TODO:
    // crossmode: Strecke anpassen
    // normalmode: Strecke anpassen
    // regenmode: Strecke anpassen
    // ap mode, komplett umbauen
    //
    if (_mode == Preferences::appOpMode)
    {
      // nix zu tun
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

  //###########################################################################
  //###########################################################################
  //#### private functions                                                 ####
  //###########################################################################
  //###########################################################################

  /**
   * @brief lese INT Variable aus dem NVS
   * 
   * @param _name 
   * @param defaultVal 
   * @return int32_t 
   */
  int32_t Preferences::getIntValue(const char *_name, int32_t defaultVal)
  {
    esp_err_t err;
    int32_t val; // value will default to 0, if not set yet in NVS
    ESP_LOGD(tag, "read int32 value <%s>...", _name);
    if (!Preferences::isInit)
      return (defaultVal);
    err = nvs_get_i32(Preferences::nvs_handle, _name, &val);
    switch (err)
    {
    case ESP_OK:
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "value <%s> is not initialized yet", _name);
      return (defaultVal);
    default:
      ESP_LOGE(tag, "error while reading value <%s>", _name);
      return (defaultVal);
    }
    return (defaultVal);
  }

  /**
   * @brief schreibe INT Variable in den NVS
   * 
   * @param _name 
   * @param _val 
   * @return true 
   * @return false 
   */
  bool Preferences::setIntValue(const char *_name, int32_t _val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_i32(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "write int32 name <%s> value <%06d>...", _name, _val);
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
      return (retVal);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "value <%s> is not initialized yet", _name);
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
   * @brief lese float Variable aus dem NVS
   * 
   * @param _name 
   * @param _defaultValue 
   * @return float 
   */
  float Preferences::getFloatValue(const char *_name, float _defaultValue)
  {
    esp_err_t err;
    float val;
    size_t size = sizeof(val);
    //
    ESP_LOGD(tag, "read float value <%s>...", _name);
    if (!Preferences::isInit)
      return (_defaultValue);
    err = nvs_get_blob(Preferences::nvs_handle, _name, &val, &size);
    switch (err)
    {
    case ESP_OK:
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "value <%s> is not initialized yet", _name);
      return (_defaultValue);
    default:
      ESP_LOGE(tag, "error while reading float value <%s>", _name);
      return (_defaultValue);
    }
    return (_defaultValue);
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

} // namespace Prefs
