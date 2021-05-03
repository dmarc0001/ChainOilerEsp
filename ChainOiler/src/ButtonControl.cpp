#include "ButtonControl.hpp"
#include "LedControl.hpp"
#include "MainWork.hpp"
#include <esp_log.h>

namespace esp32s2
{
  /**
   * @brief statische Variablen instanzieren und initialisieren
   *
   */
  const char *ButtonControl::tag{ "ButtonControl" };  //! tag fürs debug logging

  ButtonControl *ButtonControl::getInstance()
  {
    {
      static ButtonControl inst;  // Guaranteed to be destroyed.
      return &inst;
    }
  }

  /**
   * @brief initialisiere Hardware für die Schalter
   *
   */
  ButtonControl::ButtonControl()
      : controlSwitchDown( false ), lastControlSwitchAction( 0ULL ), rainSwitchDown( false ), lastRainSwitchAction( 0ULL )
  {
    using namespace Prefs;
    ESP_LOGI( tag, "init button control hardware..." );
    //
    //  Tacho und Knopf (Knopf-GPIO_INTR_ANYEDGE)
    //
    gpio_config_t config_in = { .pin_bit_mask = BIT64( INPUT_CONTROL_SWITCH ) | BIT64( INPUT_RAIN_SWITCH_OPTIONAL ),
                                .mode = GPIO_MODE_INPUT,
                                .pull_up_en = GPIO_PULLUP_ENABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_ANYEDGE };
    gpio_config( &config_in );
    //
    // Handler für die beiden Ports
    //
    gpio_isr_handler_add( INPUT_CONTROL_SWITCH, ButtonControl::buttonIsr, ( void * ) &isr_control );
    gpio_isr_handler_add( INPUT_RAIN_SWITCH_OPTIONAL, ButtonControl::buttonIsr, ( void * ) &isr_rain );
    ESP_LOGD( tag, "init button control hardware...done" );
  }

  /**
   * @brief Destroy the Button Control:: Button Control object
   *
   */
  ButtonControl::~ButtonControl()
  {
    using namespace Prefs;
    gpio_isr_handler_remove( INPUT_CONTROL_SWITCH );
    gpio_isr_handler_remove( INPUT_RAIN_SWITCH_OPTIONAL );
    gpio_config_t config_in = { .pin_bit_mask = BIT64( INPUT_CONTROL_SWITCH ) | BIT64( INPUT_RAIN_SWITCH_OPTIONAL ),
                                .mode = GPIO_MODE_DISABLE,
                                .pull_up_en = GPIO_PULLUP_DISABLE,
                                .pull_down_en = GPIO_PULLDOWN_DISABLE,
                                .intr_type = GPIO_INTR_DISABLE };
    gpio_config( &config_in );
  }

  /**
   * @brief prüfe ob sich bei den tasten etwas getan hat
   *
   */
  void ButtonControl::buttonStati()
  {
    using namespace Prefs;

    //
    // Control switch gedrückt länger als LONG ?
    //
    if ( ButtonControl::controlDownSince() > 0ULL )
    {
      if ( !Preferences::getAttentionFlag() )
      {
        Preferences::setAttentionFlag( true );
      }
    }
    else if ( Preferences::getAttentionFlag() )
    {
      Preferences::setAttentionFlag( false );
    }
    //
    // gibt es eine Aktion des Control Switch?
    //
    if ( Preferences::getControlSwitchAction() != fClick::NONE )
    {
      //
      // Kurzer Klick an CONTROL
      //
      if ( Preferences::getControlSwitchAction() == fClick::SHORT )
      {
        ESP_LOGD( tag, "CONTROL Button short down" );
        LedControl::allOff();
        if ( Preferences::getAppMode() == opMode::APMODE )
        {
          // unschalten in Normal
          ChOiler::MainWorker::switchFromAccessPointMode();
          // button löschen
          Preferences::setControlSwitchAction( fClick::NONE );
          return;
        }
        //
        // im CROSS/NORMAL mode geht nur hinund her via SHORT
        // REGEN ist bei CROSS deaktiviert
        //
        if ( Preferences::getAppMode() == opMode::CROSS )
        {
          ESP_LOGD( tag, "set NORMAL mode" );
          Preferences::setAppMode( opMode::NORMAL );
        }
        else
        {
          ESP_LOGD( tag, "set CROSS mode" );
          Preferences::setAppMode( opMode::CROSS );
        }
      }
      //
      // langer Klick an CONTROL
      //
      else if ( Preferences::getControlSwitchAction() == fClick::LONG )
      {
        ESP_LOGI( tag, "CONTROL Button long down" );
        ESP_LOGD( tag, "set ACCESS POINT mode" );
        LedControl::allOff();
        ChOiler::MainWorker::switchToAccessPointMode();
      }
      //
      // als erledigt markieren
      //
      Preferences::setControlSwitchAction( fClick::NONE );
    }
    //
    // gibt es eine Aktion des Regenschalters?
    //
    if ( Preferences::getRainSwitchAction() != fClick::NONE )
    {
      //
      // Es gab ein Ereignis
      //
      if ( Preferences::getAppMode() == opMode::NORMAL )
      {
        // von NORMAL darf es zu regen gehen, von CROSS nicht
        ESP_LOGD( tag, "set RAIN mode" );
        Preferences::setAppMode( opMode::RAIN );
      }
      else if ( Preferences::getAppMode() == opMode::RAIN )
      {
        ESP_LOGD( tag, "set NORMAL mode from RAIN" );
        Preferences::setAppMode( opMode::NORMAL );
      }
      // Taste löschen
      Preferences::setRainSwitchAction( fClick::NONE );
    }
  }

  /**
   * @brief wie lange ist der Control schalter seit SHORT schon gedrückt
   *
   * @return uint64_t
   */
  uint64_t ButtonControl::controlDownSince()
  {
    //
    // wenn taste unten, bin ich im Bereich über SHORT?
    // beim langen drücken für's blinken
    //
    if ( controlSwitchDown == 0 )
    {
      if ( esp_timer_get_time() > ( ButtonControl::lastControlSwitchAction + Prefs::LONG_CLICK_TIME_US ) )
      {
        return ButtonControl::lastControlSwitchAction;
      }
    }
    return ( 0ULL );
  }

  /**
   * @brief ISR für Schalter Betätigungen
   *
   * @param arg
   */
  void IRAM_ATTR ButtonControl::buttonIsr( void *arg )
  {
    using namespace Prefs;

    gpio_num_t *_num = static_cast< gpio_num_t * >( arg );
    uint64_t now = esp_timer_get_time();
    int level;

    switch ( *_num )
    {
      case Prefs::INPUT_CONTROL_SWITCH:
        level = gpio_get_level( Prefs::INPUT_CONTROL_SWITCH );
        //
        // Was ist passiert? Level 1 bedeutet Knopf gelöst
        //
        if ( ( level == 1 ) /*&& (ButtonControl::controlSwitchDown == 0)*/ )
        {
          //
          // button gelöst, war gedrückt
          // mindestens für kurzen klick gedrückt?
          //
          if ( now > ( ButtonControl::lastControlSwitchAction + DEBOUNCE_TIME_US ) )
          {
            // schon mal gültiger Tastendruck
            if ( now > ( ButtonControl::lastControlSwitchAction + LONG_CLICK_TIME_US ) )
            {
              Preferences::setControlSwitchAction( fClick::LONG );
            }
            else
            {
              Preferences::setControlSwitchAction( fClick::SHORT );
            }
          }
        }
        ButtonControl::controlSwitchDown = level;
        ButtonControl::lastControlSwitchAction = now;
        break;

      case Prefs::INPUT_RAIN_SWITCH_OPTIONAL:
        level = gpio_get_level( Prefs::INPUT_RAIN_SWITCH_OPTIONAL );
        //
        // Was ist passiert? Level 0 bedeutet Knopf gedrückt
        //
        if ( level == 1 && ( ButtonControl::rainSwitchDown == 0 ) )
        {
          //
          // button gelöst, war gedrückt
          //
          if ( now > ( ButtonControl::lastRainSwitchAction + DEBOUNCE_TIME_US ) )
          {
            // short click
            Preferences::setRainSwitchAction( fClick::SHORT );
          }
        }
        ButtonControl::rainSwitchDown = level;
        ButtonControl::lastRainSwitchAction = now;
        break;
      default:
        break;
    }
  }

}  // namespace esp32s2
