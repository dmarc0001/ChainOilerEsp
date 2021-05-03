#include "LedControl.hpp"
#include <esp_err.h>

namespace esp32s2
{
  /**
   * @brief instanzieren und initialisieren der statischen variablen
   *
   */
  const char *LedControl::tag{ "LedControl" };  //! tag fürs debug logging
  // esp_timer_handle_t LedControl::timerHandle{ nullptr };  //! timer handle
  LedControl LedControl::inst;  //! globale Instanz

  LedControl *LedControl::getInstance()
  {
    {
      // static LedControl inst;  // Guaranteed to be destroyed.
      return &inst;
    }
  }

  /**
   * @brief Construct a new Led Control:: Led Control object
   *
   */
  LedControl::LedControl()
      : lastChanged( 0ULL ), pumpLedSwitchOffTime( 0ULL ), nextControlLedFlash( 0ULL ), timerHandle( nullptr ), ledStateField( 0 )
  {
    using namespace Prefs;

    //
    // GPIO Konfigurieren
    //
    ESP_LOGI( tag, "init hardware for LED..." );
    //
    // LED
    //
    gpio_config_t config_led = {
        .pin_bit_mask = BIT64( LED_REED_CONTROL ) | BIT64( LED_CONTROL ) | BIT64( LED_RAIN ) | BIT64( LED_PUMP ),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE };
    gpio_config( &config_led );
    startTimer();
    ESP_LOGD( tag, "init hardware for LED...done" );
  }

  /**
   * @brief Destroy the Led Control:: Led Control object
   *
   */
  LedControl::~LedControl()
  {
    using namespace Prefs;

    esp_timer_stop( timerHandle );

    gpio_config_t config_led = {
        .pin_bit_mask = BIT64( LED_REED_CONTROL ) | BIT64( LED_CONTROL ) | BIT64( LED_RAIN ) | BIT64( LED_PUMP ),
        .mode = GPIO_MODE_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE };
    gpio_config( &config_led );
  }

  /**
   * @brief eigene Timer routine für die Steuerung der LED
   *
   */
  void LedControl::startTimer()
  {
    //
    // timer für Punpe starten
    //
    const esp_timer_create_args_t appTimerArgs = {
        .callback = &LedControl::timerCallback, .arg = nullptr, .dispatch_method = ESP_TIMER_TASK, .name = "led_timer" };
    //
    // timer erzeugen
    //
    ESP_ERROR_CHECK( esp_timer_create( &appTimerArgs, &timerHandle ) );
    //
    // timer starten, microsekunden ( 100 ms soll es)
    //
    ESP_ERROR_CHECK( esp_timer_start_periodic( timerHandle, 100000 ) );
    //
  }

  /**
   * @brief Alle LED ausschalten
   *
   */
  void LedControl::allOff()
  {
    gpio_set_level( Prefs::LED_CONTROL, 0 );
    gpio_set_level( Prefs::LED_RAIN, 0 );
    gpio_set_level( Prefs::LED_PUMP, 0 );
    nextControlLedFlash = 0ULL;
    ledStateField &= ~( Prefs::whichLed::WICH_LED_CONTROL | Prefs::whichLed::WICH_LED_RAIN | Prefs::whichLed::WICH_LED_PUMP );
  }

  /**
   * @brief blinken mit allen LED (z.B. beim Booten)
   *
   */
  void LedControl::showAttention()
  {
    static bool attentionLEDIsOn = false;
    uint64_t timeDiff = esp_timer_get_time() - lastChanged;
    //
    if ( attentionLEDIsOn && timeDiff > Prefs::BLINK_LED_ATTENTION_ON )
    {
      attentionLEDIsOn = false;
      gpio_set_level( Prefs::LED_CONTROL, 0 );
      gpio_set_level( Prefs::LED_RAIN, 1 );
      gpio_set_level( Prefs::LED_PUMP, 1 );
      lastChanged = esp_timer_get_time();
    }
    else if ( !attentionLEDIsOn && timeDiff > Prefs::BLINK_LED_ATTENTION_OFF )
    {
      attentionLEDIsOn = true;
      gpio_set_level( Prefs::LED_CONTROL, 1 );
      gpio_set_level( Prefs::LED_RAIN, 0 );
      gpio_set_level( Prefs::LED_PUMP, 0 );
      lastChanged = esp_timer_get_time();
    }
  }

  /**
   * @brief schalte Regen LED an/aus
   *
   * @param _set
   */
  void LedControl::setRainLED( bool _set )
  {
    if ( _set )
    {
      ledStateField |= Prefs::whichLed::WICH_LED_RAIN;
      gpio_set_level( Prefs::LED_RAIN, 1 );
    }
    else
    {
      ledStateField &= ~Prefs::whichLed::WICH_LED_RAIN;
      gpio_set_level( Prefs::LED_RAIN, 0 );
    }
  }

  /**
   * @brief schalte Pumpen LED
   *
   * @param _set
   */
  void LedControl::setPumpLED( bool _set )
  {
    if ( _set )
    {
      // wann soll das Leuchten aufhören?
      pumpLedSwitchOffTime = esp_timer_get_time() + Prefs::Preferences::getPumpLedTimeout();
      ledStateField |= Prefs::whichLed::WICH_LED_PUMP;
      gpio_set_level( Prefs::LED_PUMP, 1 );
    }
    else
    {
      pumpLedSwitchOffTime = 0ULL;
      ledStateField &= ~Prefs::whichLed::WICH_LED_PUMP;
      gpio_set_level( Prefs::LED_PUMP, 0 );
    }
  }

  /**
   * @brief schalte control LED
   *
   * @param _set
   */
  void LedControl::setContolLED( bool _set )
  {
    if ( _set )
    {
      ledStateField |= Prefs::whichLed::WICH_LED_CONTROL;
      gpio_set_level( Prefs::LED_CONTROL, 1 );
    }
    else
    {
      ledStateField &= ~Prefs::whichLed::WICH_LED_CONTROL;
      gpio_set_level( Prefs::LED_CONTROL, 0 );
    }
  }

  /**
   * @brief Callback für den Timer (100 ms)
   *
   */
  void LedControl::timerCallback( void * )
  {
    using namespace Prefs;
    //
    // pumpen LED Nachlauf beendet?
    //
    if ( inst.pumpLedSwitchOffTime > 0ULL )
    {
      if ( esp_timer_get_time() > inst.pumpLedSwitchOffTime )
      {
        inst.setPumpLED( false );
      }
    }
    //
    // je nach Zustand arbeiten
    //
    switch ( Preferences::getAppMode() )
    {
      case opMode::NORMAL:
        inst.processControlLEDFlash( opMode::NORMAL );
        inst.processLEDNormalMode();
        break;
      case opMode::CROSS:
        inst.processControlLEDFlash( opMode::CROSS );
        inst.processLEDCrossMode();
        break;
      case opMode::RAIN:
        inst.processControlLEDFlash( opMode::CROSS );
        inst.processLEDRainMode();
        break;
      case opMode::APMODE:
        inst.processLEDApMode();
        break;
      default:
        break;
    }
  }

  /**
   * @brief Timer routine im mormalen Mode
   *
   */
  void LedControl::processLEDNormalMode()
  {
    //
    // control led durch blinken gesetzt
    // regen hier AUS
    //
    if ( ledStateField & Prefs::whichLed::WICH_LED_RAIN )
    {
      setRainLED( 0 );
    }
  }

  /**
   * @brief zyklisch alle 100 ms in CROSS Mode
   *
   */
  void LedControl::processLEDCrossMode()
  {
    //
    // control led durch blinken gesetzt
    // regen hier aus
    //
    if ( ledStateField & Prefs::whichLed::WICH_LED_RAIN )
    {
      setRainLED( 0 );
    }
  }

  /**
   * @brief zyklisch alle 100 ms in RAIN Mode
   *
   */
  void LedControl::processLEDRainMode()
  {
    //
    // control led durch blinken gesetzt
    // regen LED hier AN
    //
    if ( !( ledStateField & Prefs::whichLed::WICH_LED_RAIN ) )
    {
      setRainLED( 1 );
    }
  }

  /**
   * @brief zyklisch alle 100 ms in AccesPoint Mode
   *
   */
  void LedControl::processLEDApMode()
  {
    using namespace Prefs;
    //
    // das blitzen der control und Regen LED im wechsel als Zeichen das es läuft
    //
    if ( esp_timer_get_time() > LedControl::nextControlLedFlash )
    {
      // da muss was passieren
      if ( ledStateField & Prefs::whichLed::WICH_LED_CONTROL )
      {
        // ist on soll off
        // ESP_LOGD(tag, "Control LED off, Rain LED on");
        setContolLED( 0 );
        setRainLED( 1 );
        nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_AP_OFF;
      }
      else
      {
        // ist off soll on
        // ESP_LOGD(tag, "Control LED on, Rain LED of");
        setContolLED( 1 );
        setRainLED( 0 );
        nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_AP_ON;
      }
    }
  }

  /**
   * @brief zyklisch alle 100 ms zum blinken der Controll LED in normal und crossmode
   *
   * @param _mode
   */
  void LedControl::processControlLEDFlash( Prefs::opMode _mode )
  {
    using namespace Prefs;
    //
    // das blitzen der control LED als Zeichen das es läuft
    //
    if ( esp_timer_get_time() > nextControlLedFlash )
    {
      // da muss was passieren
      if ( ledStateField & Prefs::whichLed::WICH_LED_CONTROL )
      {
        // ist on soll off
        // ESP_LOGD(tag, "Control LED off");
        setContolLED( 0 );
        if ( Prefs::Preferences::getAttentionFlag() )
        {
          nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_ATTENTION_OFF;
        }
        else if ( _mode == opMode::NORMAL || _mode == opMode::RAIN )
          nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_OFF;
        else
          nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_ON;
      }
      else
      {
        // ist off soll on
        // ESP_LOGD(tag, "Control LED on");
        setContolLED( 1 );
        if ( Prefs::Preferences::getAttentionFlag() )
        {
          nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_ATTENTION_ON;
        }
        else if ( _mode == opMode::NORMAL || _mode == opMode::RAIN )
          nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_ON;
        else
          nextControlLedFlash = esp_timer_get_time() + Prefs::BLINK_LED_CONTROL_NORMAL_OFF;
      }
    }
  }
}  // namespace esp32s2
