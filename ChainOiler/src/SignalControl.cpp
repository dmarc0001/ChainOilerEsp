#include "SignalControl.hpp"
#include <esp_log.h>

namespace esp32s2
{
  /**
   * @brief instanzieren und initialisieren der statischen variablen
   *
   */
  const char *SignalControl::tag{"SignalControl"};                 //! tag fürs debug logging
  volatile int64_t SignalControl::pumpLedSwitchedOff{false};       //! ist die Pumpen LED noch an?
  volatile int64_t SignalControl::controlLedSwitchedOffDelta{0LL}; //! wann soll die Control LED wieder aus?
  volatile int64_t SignalControl::controlLedSwitchedOff{0LL};      //! wann soll die Control LED wieder aus?
  volatile int64_t SignalControl::rainLedSwitchedOff{0LL};         //! wann soll die Regen-LED wieder aus?
  volatile int64_t SignalControl::apModeLedSwitchOff{0LL};         //! wann soll ap-mode ausgeschakltet werden?
  esp_timer_handle_t SignalControl::timerHandle{nullptr};          //! timer handle

  /**
   * @brief initialisierung der Hardware für die LED
   *
   */
  void SignalControl::init()
  {
    using namespace Prefs;
    ESP_LOGD(tag, "init hardware for signaling...done");

#ifdef RAWLED
    esp32s2::LedControl::init();
#endif
#ifdef LEDSTRIPE
    esp32s2::LedStripeControl::init();
#endif
    //
    // Timer starten
    //
    SignalControl::startTimer();
    ESP_LOGD(tag, "init hardware for signaling...done");
  }

  void SignalControl::allOff()
  {
#ifdef RAWLED
    LedControl::allOff();
#endif
#ifdef LEDSTRIPE
    LedStripeControl::allOff();
#endif
  }

#ifdef RAWLED
#endif
#ifdef LEDSTRIPE
#endif

  void SignalControl::flashControlLED()
  {
    // TODO: Contol LED blitzen lassen
    controlLedSwitchedOffDelta = Prefs::BLINK_LED_CONTROL_NORMAL_ON;
  }

  /**
   * @brief eigene Timer routine für die Steuerung der LED
   *
   */
  void SignalControl::startTimer()
  {
    //
    // timer für Punpe starten
    //
    const esp_timer_create_args_t appTimerArgs =
        {
            .callback = &SignalControl::timerCallback,
            .arg = nullptr,
            .dispatch_method = ESP_TIMER_TASK,
            .name = "signal_timer",
            .skip_unhandled_events = false};
    //
    // timer erzeugen
    //
    ESP_ERROR_CHECK(esp_timer_create(&appTimerArgs, &SignalControl::timerHandle));
    //
    // timer starten, microsekunden ( 100 ms soll es)
    //
    ESP_ERROR_CHECK(esp_timer_start_periodic(SignalControl::timerHandle, 100000ULL));
    //
  }

  /**
   * @brief Timer alle 100 ms
   *
   */
  void SignalControl::timerCallback(void *)
  {
    //
    // alle 100 ms
    //
    using namespace Prefs;
    static int64_t nextChange{0LL};
    static bool isControlLedOn = false;
    static bool isPumpLedOn = false;
    static bool isRainLedOn = false;
    static bool isApModeOn = false;

    int64_t nowTime{esp_timer_get_time()};

    if (Preferences::isAttentionFlag)
    {
      static bool attentionLEDIsOn = false;
      //
      // Achtungzeichen, LEDS flackern
      //
      if (nowTime > nextChange)
      {
        if (attentionLEDIsOn)
        {
          // Blink led aus
#ifdef RAWLED
          LedControl::setAttentionLED(false);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setAttentionLED(false);
#endif
          nextChange = nowTime + Prefs::BLINK_LED_ATTENTION_OFF;
          attentionLEDIsOn = false;
        }
        else
        {
#ifdef RAWLED
          LedControl::setAttentionLED(true);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setAttentionLED(true);
#endif
          nextChange = nowTime + Prefs::BLINK_LED_ATTENTION_ON;
          attentionLEDIsOn = true;
        }
#ifdef RAWLED
        LedControl::makeChange();
#endif
#ifdef LEDSTRIPE
        LedStripeControl::makeChange();
#endif
      }
      return;
    }

    if (Preferences::appOpMode == opMode::APMODE)
    {
      if (nowTime > nextChange)
      {
        if (isApModeOn)
        {
          // AP Mode LED aus
#ifdef RAWLED
          LedControl::setAPModeLED(false);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setAPModeLED(false);
#endif
          nextChange = nowTime + Prefs::BLINK_LED_CONTROL_AP_OFF;
          isApModeOn = false;
        }
        else
        {
          // AP Mode LED an
#ifdef RAWLED
          LedControl::setAPModeLED(true);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setAPModeLED(true);
#endif
          nextChange = nowTime + Prefs::BLINK_LED_CONTROL_AP_ON;
          isApModeOn = true;
        }
#ifdef RAWLED
        LedControl::makeChange();
#endif
#ifdef LEDSTRIPE
        LedStripeControl::makeChange();
#endif
      }
      return;
    }

    //
    // Control LED Flackern...
    //
    if (controlLedSwitchedOffDelta != 0ULL)
    {
      controlLedSwitchedOff = nowTime + controlLedSwitchedOffDelta;
      controlLedSwitchedOffDelta = 0LL;
    }

    if (controlLedSwitchedOff)
    {
      if (nowTime > controlLedSwitchedOff)
      {
        if (isControlLedOn)
        {
          // Contol LED aus
#ifdef RAWLED
          LedControl::setControlLED(false);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setControlLED(false);
#endif
          isControlLedOn = false;
          controlLedSwitchedOff = 0ULL;
        }
      }
      else
      {
        if (!isControlLedOn)
        {
          // Contrtol LED an
#ifdef RAWLED
          LedControl::setControlLED(true);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setControlLED(true);
#endif
          isControlLedOn = true;
        }
      }
    }
    else
    {
      if (isControlLedOn)
      {
        // Control LED aus
        isControlLedOn = false;
        controlLedSwitchedOff = 0ULL;
      }
    }

    //
    // ist die Pumpe aktiv?
    //
    if (Preferences::pumpCycles > 0U || Preferences::pumpAction)
    {
      //
      // ist die LED schon an?
      //
      if (!isPumpLedOn)
      {
        // Pumpe LED AN
#ifdef RAWLED
        LedControl::setPumpLED(true);
#endif
#ifdef LEDSTRIPE
        LedStripeControl::setPumpLED(true);
#endif
        isPumpLedOn = true;
      }
      // die Ausschaltzeit setzen, falls pumpCycles 0 wird
      pumpLedSwitchedOff = nowTime + Prefs::PUMP_LED_DELAY;
      Preferences::pumpAction = false;
    }
    else
    {
      // Pumpenzyklen sind 0
      // läuft die Einschaltzeit noch?
      if (pumpLedSwitchedOff != 0ULL)
      {
        if (isPumpLedOn && (nowTime > pumpLedSwitchedOff))
        {
          // Tumnp LED AUS
#ifdef RAWLED
          LedControl::setPumpLED(false);
#endif
#ifdef LEDSTRIPE
          LedStripeControl::setPumpLED(false);
#endif
          isPumpLedOn = false;
          pumpLedSwitchedOff = 0ULL;
        }
      }
    }

    if (rainLedSwitchedOff != 0uLL)
    {
      if (isRainLedOn)
      {
        // ausschalten
#ifdef RAWLED
        LedControl::setRainLED(true);
#endif
#ifdef LEDSTRIPE
        LedStripeControl::setRainLED(true);
#endif
      }
    }
#ifdef RAWLED
    LedControl::makeChange();
#endif
#ifdef LEDSTRIPE
    LedStripeControl::makeChange();
#endif
  }
}
