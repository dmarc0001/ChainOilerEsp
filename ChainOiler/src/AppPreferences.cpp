#include "AppPreferences.hpp"
#include "ProjectDefaults.hpp"
#include <cmath>

namespace Prefs
{
  const char *Preferences::serialStr = "20210320-211937-build-0372";
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
    ESP_LOGD(tag, "%s: init NVM...", __func__);
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
    ESP_LOGD(tag, "%s: open NVM...", __func__);
    err = nvs_open("new_mc_coi", NVS_READWRITE, &Preferences::nvs_handle);
    if (err != ESP_OK)
    {
      printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    else
    {
      ESP_LOGD(tag, "%s: init NVM...done", __func__);
    }
    ESP_LOGD(tag, "%s: read version from NVM...", __func__);
    int _vers = Preferences::getIntValue(VERSION_STR, -1);
    ESP_LOGD(tag, "%s: version from NVM is <%02d>, current version is <%02d>...", __func__, _vers, Preferences::version);
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
    Preferences::close();
    ESP_ERROR_CHECK(nvs_flash_erase());
    Preferences::init();
  }

  void Preferences::setApSSID(std::string _ssid)
  {
    Preferences::ssid = _ssid;
    Preferences::setStringValue(DEFAULT_SSID, _ssid.c_str());
  }

  std::string Preferences::getApSSID()
  {
    return Preferences::ssid;
  }

  void Preferences::setApPasswd(std::string _passwd)
  {
    Preferences::ssid = _passwd;
    Preferences::setStringValue(AP_PASSWORD_STR, _passwd.c_str());
  }

  std::string Preferences::getApPasswd()
  {
    return Preferences::ap_passwd;
  }

  void Preferences::setPulsePerRound(float _val)
  {
    Preferences::pulsePerRound = _val;
    Preferences::setFloatValue(PULSE_PER_WEEL_ROUND_STR, _val);
  }

  float Preferences::getPulsePerRound()
  {
    return Preferences::pulsePerRound;
  }

  void Preferences::setCircumFerence(float _val)
  {
    Preferences::circumFerence = _val;
    Preferences::setFloatValue(WHEEL_CIRCUM_FERENCE_STR, _val);
  }

  float Preferences::getCircumFerence()
  {
    return Preferences::circumFerence;
  }

  void Preferences::setOilInterval(float _val)
  {
    Preferences::oilInterval = _val;
    Preferences::setFloatValue(OIL_INTERVAL_STR, _val);
  }

  float Preferences::getOilInterval()
  {
    return Preferences::oilInterval;
  }

  void Preferences::setRainFactor(float _val)
  {
    Preferences::rainFactor = _val;
    Preferences::setFloatValue(RAIN_OIL_INTERVAL_FACTOR_STR, _val);
  }

  float Preferences::getRainFactor()
  {
    return Preferences::rainFactor;
  }

  void Preferences::setCrossFactor(float _val)
  {
    Preferences::crossFactor = _val;
    Preferences::setFloatValue(CROSS_OIL_INTERVAL_FACTOR_STR, _val);
  }

  float Preferences::getCrossFactor()
  {
    return Preferences::crossFactor;
  }

  void Preferences::setSpeedProgression(float _val)
  {
    Preferences::speedProgression = _val;
    Preferences::setFloatValue(SPEED_PROGRESSION_FACTOR_STR, _val);
  }

  float Preferences::getSpeedProgression()
  {
    return Preferences::speedProgression;
  }

  void Preferences::setSensorThreshold(int _val)
  {
    Preferences::rainSensorThreshold = _val;
    Preferences::setIntValue(THRESHOLD_RAIN_SENSOR_STR, static_cast<uint32_t>(_val));
  }

  int Preferences::getSensorThreshold()
  {
    return Preferences::rainSensorThreshold;
  }

  void Preferences::makeDefaults()
  {
    esp_err_t err;
    //
    // debuggen
    //
    ESP_LOGD(tag, "%s: make default propertys...", __func__);
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
    //
    // Commit written values.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    //
    ESP_LOGD(tag, "%s: make default propertys...done", __func__);
    ESP_LOGD(tag, "%s: commit updates in NVS...", __func__);
    err = nvs_commit(Preferences::nvs_handle);
    ESP_ERROR_CHECK(err);
    ESP_LOGD(tag, "%s: commit updates in NVS...done", __func__);
  }

  void Preferences::readPreferences()
  {
    //
    // alle Einstellugnen einlesen
    //
    ESP_LOGD(tag, "%s: read all preferences...", __func__);
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
    ESP_LOGD(tag, "%s: read all preferences...done", __func__);
  }

  int16_t Preferences::getPulsesFor100Meters()
  {
    float val100Meters = (100.0 / circumFerence) * pulsePerRound;
    int16_t count = static_cast<int16_t>(std::ceil(val100Meters));
    ESP_LOGI(tag, "%s: ==== pulses for 100 meters: <%06d>", __func__, count);
    return count;
  }

  int16_t Preferences::getPulsesFor10Meters()
  {
    float val10Meters = (10.0 / circumFerence) * pulsePerRound;
    int16_t count = static_cast<int16_t>(std::ceil(val10Meters));
    ESP_LOGI(tag, "%s: ==== pulses for 10 meters: <%06d>", __func__, count);
    return (count >= 1) ? count : 1;
  }

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

  //###########################################################################
  //###########################################################################
  //#### private functions                                                 ####
  //###########################################################################
  //###########################################################################

  int32_t Preferences::getIntValue(const char *_name, int32_t defaultVal)
  {
    esp_err_t err;
    int32_t val; // value will default to 0, if not set yet in NVS
    ESP_LOGD(tag, "%s: read int32 value <%s>...", __func__, _name);
    if (!Preferences::isInit)
      return (defaultVal);
    err = nvs_get_i32(Preferences::nvs_handle, _name, &val);
    switch (err)
    {
    case ESP_OK:
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "%s: value <%s> is not initialized yet", __func__, _name);
      return (defaultVal);
    default:
      ESP_LOGE(tag, "%s: error while reading value <%s>", __func__, _name);
      return (defaultVal);
    }
    return (defaultVal);
  }

  bool Preferences::setIntValue(const char *_name, int32_t _val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_i32(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "%s: write int32 name <%s> value <%06d>...", __func__, _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

  std::string Preferences::getStringValue(const char *_name)
  {
    esp_err_t err;
    std::string retVal;
    char resultArray[64];
    size_t size = sizeof(resultArray);

    ESP_LOGD(tag, "%s: read string value <%s>...", __func__, _name);
    if (!Preferences::isInit)
      return (nullptr);
    err = nvs_get_str(Preferences::nvs_handle, _name, &(resultArray[0]), &size);
    switch (err)
    {
    case ESP_OK:
      retVal = std::string(&(resultArray[0]));
      return (retVal);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "%s: value <%s> is not initialized yet", __func__, _name);
      return (nullptr);
    default:
      ESP_LOGE(tag, "%s: error while reading string value <%s>", __func__, _name);
      return (nullptr);
    }
    return (nullptr);
  }

  bool Preferences::setStringValue(const char *_name, const char *_val)
  {
    esp_err_t err;
    if (!Preferences::isInit)
      return false;
    err = nvs_set_str(Preferences::nvs_handle, _name, _val);
    ESP_LOGD(tag, "%s: write string name <%s> value <%s>...", __func__, _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

  float Preferences::getFloatValue(const char *_name, float _defaultValue)
  {
    esp_err_t err;
    float val;
    size_t size = sizeof(val);
    //
    ESP_LOGD(tag, "%s: read float value <%s>...", __func__, _name);
    if (!Preferences::isInit)
      return (_defaultValue);
    err = nvs_get_blob(Preferences::nvs_handle, _name, &val, &size);
    switch (err)
    {
    case ESP_OK:
      return (val);
    case ESP_ERR_NVS_NOT_FOUND:
      ESP_LOGE(tag, "%s: value <%s> is not initialized yet", __func__, _name);
      return (_defaultValue);
    default:
      ESP_LOGE(tag, "%s: error while reading float value <%s>", __func__, _name);
      return (_defaultValue);
    }
    return (_defaultValue);
  }

  bool Preferences::setFloatValue(const char *_name, float _val)
  {
    esp_err_t err;
    //
    if (!Preferences::isInit)
      return false;
    err = nvs_set_blob(Preferences::nvs_handle, _name, &_val, sizeof(_val));
    ESP_LOGD(tag, "%s: write float <%s> value <%.2f>...", __func__, _name, _val);
    if (err == ESP_OK)
      return true;
    return false;
  }

} // namespace Prefs
