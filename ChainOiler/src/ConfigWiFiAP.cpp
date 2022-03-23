#include "ConfigWiFoAP.hpp"
#include <lwip/err.h>
#include <esp_err.h>
#include <esp_log.h>

namespace ChOiler
{
  const char *WiFiAccessPoint::tag{"WiFiAccessPoint"}; // der TAG fürs debuggen
  bool WiFiAccessPoint::nvsIsInit{false};              //! nvs ist nicht initialiseiert;

  /**
   * @brief Construct a new Wi Fi Access Point:: Wi Fi Access Point object
   *
   */
  WiFiAccessPoint::WiFiAccessPoint(){};

  /**
   * @brief Eventhandler für WiFi Funktionen
   *
   * @param arg
   * @param event_base
   * @param event_id
   * @param event_data
   */
  void WiFiAccessPoint::wifiEventHandler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
  {
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
      wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
      ESP_LOGD(WiFiAccessPoint::tag, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
      wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
      ESP_LOGD(WiFiAccessPoint::tag, " station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
    else
    {
      ESP_LOGD(WiFiAccessPoint::tag, "WiFiEvent");
    }
  }

  /**
   * @brief initialisiere den Accesspoint
   *
   * @return true
   * @return false
   */
  bool WiFiAccessPoint::wifiInitAp(void)
  {
    bool result = true;
    using namespace Prefs;
    wifi_config_t wifi_config;
    //
    // System für WiFi initialisieren
    //
    ESP_LOGD(tag, "init nvs partition...");
    result = WiFiAccessPoint::initNVS();
    ESP_LOGD(tag, "init nvs partition...done");
    //
    ESP_LOGD(tag, "init netif...");
    if (ESP_OK != esp_netif_init())
    {
      //
      // Fehler bei NETIF-INIT
      //
      ESP_LOGE(tag, "%s, %d:esp_netif_init() failed!", __FILE__, __LINE__);
      result = false;
    }

    ESP_LOGD(tag, "create event loop...");
    if (result && (ESP_OK != esp_event_loop_create_default()))
    {
      //
      // Fehler beim erzeugen des default WLAN AP
      //
      ESP_LOGE(tag, "%s, %d: esp_event_loop_create_default() failed!", __FILE__, __LINE__);
      result = false;
    }

    if (result)
    {
      ESP_LOGD(tag, "make default accesspoint...");
      // default wifi accesspoint machen
      esp_netif_create_default_wifi_ap();
      wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
      // initialisieren, wenn machbar
      ESP_LOGD(tag, "init wifi...");
      if (result && (ESP_OK != esp_wifi_init(&cfg)))
      {
        ESP_LOGE(tag, "%s, %d: esp_wifi_init() failed!", __FILE__, __LINE__);
        result = false;
      }
      ESP_LOGD(tag, "init event handler...");
      // event Handler initialisieren
      if (result && (ESP_OK != esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiAccessPoint::wifiEventHandler,
                                                                   nullptr, nullptr)))
      {
        ESP_LOGE(tag, "%s, %d: esp_wifi_init() failed!", __FILE__, __LINE__);
        result = false;
      }
    }
    if (result)
    {
      ESP_LOGD(tag, "config my wifi accesspoint...");
      // Meine wifi Konfiguration machen
      void *param = &wifi_config.ap.ssid[0];
      wifi_config.ap.ssid_len = Preferences::getApSSID().copy(static_cast<char *>(param), 32, 0);
      wifi_config.ap.channel = Preferences::getWiFiChannel();
      param = &wifi_config.ap.password[0];
      Preferences::getApPasswd().copy(static_cast<char *>(param), 64, 0);
      param = nullptr;
      wifi_config.ap.max_connection = Preferences::getMaxConnections();
      wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
      if (strlen(Preferences::getApSSID().c_str()) == 0)
      {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
      }
    }
    //
    // meinen accesspoint initialisieren
    //
    // ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_AP ) );
    ESP_LOGD(tag, "set AP mode...");
    if (result && (ESP_OK != esp_wifi_set_mode(WIFI_MODE_AP)))
    {
      ESP_LOGE(tag, "%s, %d: esp_wifi_set_mode() failed!", __FILE__, __LINE__);
      result = false;
    }
    //
    // Konfiguration setzen
    //
    // ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_AP, &wifi_config ) );
    ESP_LOGD(tag, "apply my config...");
    if (result && (ESP_OK != esp_wifi_set_config(WIFI_IF_AP, &wifi_config)))
    {
      ESP_LOGE(tag, "%s, %d: esp_wifi_set_config() failed!", __FILE__, __LINE__);
      result = false;
    }
    //
    // wifi starten um den webserver starten zu können
    //
    // ESP_ERROR_CHECK( esp_wifi_start() );
    ESP_LOGD(tag, "start wifi...");
    if (result && (ESP_OK != esp_wifi_start()))
    {
      ESP_LOGE(tag, "%s, %d: esp_wifi_start() failed!", __FILE__, __LINE__);
      result = false;
    }
    if (result)
    {
      // debugging
      ESP_LOGD(tag, "wifi_init_softap finished. SSID:%s password:%s channel:%d", Preferences::getApSSID().c_str(),
               Preferences::getApPasswd().c_str(), static_cast<int>(Preferences::getWiFiChannel()));
    }
    return result;
  }

  /**
   * @brief Wifi abschalten
   *
   * @return true
   * @return false
   */
  bool WiFiAccessPoint::shutdownWifi(void)
  {
    esp_err_t api_result;
    ESP_LOGD(tag, "shutdownWiFi() ...");

    //
    // kann WIFI runtergefahren werden?
    //
    api_result = esp_wifi_scan_stop();
    if (api_result == ESP_OK)
    {
      ESP_LOGD(tag, "shutdown WIFI...");
      if (ESP_OK != esp_wifi_stop())
      {
        ESP_LOGE(tag, "ERROR shutdown WIFI...");
        return false;
      }
      ESP_LOGD(tag, "shutdown WIFI...OK");

      ESP_LOGD(tag, "deinit WIFI...");
      if (ESP_OK != esp_wifi_deinit())
      {
        ESP_LOGE(tag, "ERROR deinit WIFI...");
        return false;
      }
      ESP_LOGD(tag, "deinit WIFI...OK");
    }
    else if (api_result == ESP_ERR_WIFI_NOT_INIT)
    {
      ESP_LOGE(tag, "wifi was not init...");
      return true;
    }
    else if (api_result == ESP_ERR_WIFI_NOT_STARTED)
    {
      ESP_LOGE(tag, "wifi was not started...");
      ESP_LOGD(tag, "deinit WIFI...");
      if (ESP_OK != esp_wifi_deinit())
      {
        ESP_LOGE(tag, "ERROR deinit WIFI...");
        return false;
      }
      ESP_LOGD(tag, "deinit WIFI...OK");
    }
    ESP_LOGD(tag, "deinit WIFI NVS...");
    nvs_flash_deinit_partition(Prefs::WIFI_NVS_PARTITION);
    if (ESP_OK != esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, nullptr))
    {
      ESP_LOGE(tag, "%s, %d: event unregister failed!", __FILE__, __LINE__);
    }
    WiFiAccessPoint::nvsIsInit = false;
    return true;
  }

  bool WiFiAccessPoint::initNVS()
  {
    WiFiAccessPoint::nvsIsInit = false;

    esp_err_t err = nvs_flash_init_partition(Prefs::WIFI_NVS_PARTITION);
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
      ESP_LOGE(tag, "WLAN NVM init failed, erase nvm flash...");
      //
      // Partition ist nicht okay, wird geloescht
      // Wiederhole nvs_flash_init
      //
      ESP_ERROR_CHECK(nvs_flash_erase_partition(Prefs::WIFI_NVS_PARTITION));
      err = nvs_flash_init_partition(Prefs::WIFI_NVS_PARTITION);
      //
      // ging ales gut?
      //
      ESP_ERROR_CHECK(err);
      WiFiAccessPoint::nvsIsInit = true;
    }
    else
    {
      //
      // ging ales gut?
      //
      ESP_ERROR_CHECK(err);
      WiFiAccessPoint::nvsIsInit = true;
    }
    ESP_LOGI(tag, "init WLAN NVM...done!");
    return WiFiAccessPoint::nvsIsInit;
  }
} // namespace ChOiler
