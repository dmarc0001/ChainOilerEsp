#include "WiFiStuff.hpp"
#include <cstring>

const char *WiFiStuff::tag = "WiFiStuff";
esp_netif_t *WiFiStuff::instance{nullptr};

void WiFiStuff::initWiFi()
{
  ESP_ERROR_CHECK( esp_netif_init() );
  ESP_ERROR_CHECK( esp_event_loop_create_default() );
  WiFiStuff::instance = esp_netif_create_default_wifi_ap();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init( &cfg ) );
  ESP_ERROR_CHECK( esp_event_handler_instance_register( WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiStuff::evtHandler, NULL, NULL ) );

  wifi_config_t wifi_config;
  wifi_config.ap.channel = wifiChannel;
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;  // WIFI_AUTH_WPA_WPA2_PSK;
  wifi_config.ap.max_connection = maxWifiConn;
  wifi_config.ap.ssid_len = lenSSID;
  memcpy( wifi_config.ap.ssid, apSSID, 32 );
  memcpy( wifi_config.ap.password, apPw, 64 );

  if ( strlen( reinterpret_cast< const char * >( &apPw ) ) == 0 )
  {
    wifi_config.ap.authmode = WIFI_AUTH_OPEN;
  }

  ESP_ERROR_CHECK( esp_wifi_set_mode( WIFI_MODE_AP ) );
  ESP_ERROR_CHECK( esp_wifi_set_config( WIFI_IF_AP, &wifi_config ) );
  ESP_ERROR_CHECK( esp_wifi_start() );
#ifdef DEBUG
  uint8_t primaryChannel, secoundChannel;
  wifi_second_chan_t secondChannelEnum;

  if ( ESP_OK == esp_wifi_get_channel( &primaryChannel, &secondChannelEnum ) )
  {
    if ( secondChannelEnum == WIFI_SECOND_CHAN_NONE )
      secoundChannel = 0;
    else if ( secondChannelEnum == WIFI_SECOND_CHAN_ABOVE )
      secoundChannel = primaryChannel - 1;
    else
      secoundChannel = primaryChannel + 1;
    ESP_LOGI( WiFiStuff::tag, "wifi_init_softap finished. SSID:%s password:%s channel:%d, 2nd channel: %d", apSSID, apPw,
              primaryChannel, secoundChannel );
  }
  const char *loc_hostname;
  if ( ESP_OK == esp_netif_get_hostname( WiFiStuff::instance, &loc_hostname ) )
  {
    ESP_LOGD( WiFiStuff::tag, "WiFi interface hostname: %s", loc_hostname );
  }
#endif
}

void WiFiStuff::stopWiFi()
{
  ESP_LOGI( WiFiStuff::tag, "stop WiFi functionality..." );
  ESP_ERROR_CHECK( esp_wifi_deinit() );
  ESP_ERROR_CHECK( esp_wifi_clear_default_wifi_driver_and_handlers( WiFiStuff::instance ) );
  esp_netif_destroy( WiFiStuff::instance );
  WiFiStuff::instance = nullptr;
  ESP_LOGI( WiFiStuff::tag, "stop WiFi functionality...done" );
}

void WiFiStuff::evtHandler( void *arg, esp_event_base_t evtBase, int32_t evtId, void *data )
{
  if ( evtId == WIFI_EVENT_AP_STACONNECTED )
  {
    wifi_event_ap_staconnected_t *event = ( wifi_event_ap_staconnected_t * ) data;
    ESP_LOGI( WiFiStuff::tag, "station (MAC) " MACSTR " join, AID=%d", MAC2STR( event->mac ), event->aid );
  }
  else if ( evtId == WIFI_EVENT_AP_STADISCONNECTED )
  {
    wifi_event_ap_stadisconnected_t *event = ( wifi_event_ap_stadisconnected_t * ) data;
    ESP_LOGI( WiFiStuff::tag, "station (MAC) " MACSTR " leave, AID=%d", MAC2STR( event->mac ), event->aid );
  }
}
