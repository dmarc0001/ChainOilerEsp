#include "main.hpp"
#include "WiFiStuff.hpp"

static const char *tag = "http-test";

uint8_t apSSID[ 32 ]{ "http-test-srv" };
uint8_t apPw[ 64 ]{ "ganz-geheim" };
size_t lenSSID = 13;

void app_main()
{
  //
  // warum wurde gestartet?
  //
  esp_reset_reason_t reason = esp_reset_reason();
  if ( reason != ESP_RST_POWERON )
  {
    //
    // das war nicht "normal"
    //
    ESP_LOGI( tag, "==================================================================" );
    ESP_LOGI( tag, "Restart was not power-on. Problem?" );
    ESP_LOGI( tag, "Restart reason: %d", reason );
    ESP_LOGI( tag, "==================================================================" );
    vTaskDelay( pdMS_TO_TICKS( 1500 ) );
  }

  ESP_LOGD( tag, "controller start..." );
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if ( ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND )
  {
    ESP_ERROR_CHECK( nvs_flash_erase() );
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK( ret );

  ESP_LOGI( tag, "ESP_WIFI_MODE_AP" );
  WiFiStuff::initWiFi();

  while ( true )
  {
    //
    // nix machen, faul sein
    //
    vTaskDelay( pdMS_TO_TICKS( 1500 ) );
    ESP_LOGD( tag, "..." );
  }
}
