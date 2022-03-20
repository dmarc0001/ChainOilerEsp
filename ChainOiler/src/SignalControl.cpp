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
  SignalControlAbstract *SignalControl::ledObject{nullptr};        //! polimorphes Objekt für LED's/Signale

  /**
   * @brief initialisierung der Hardware für die LED
   *
   */
  void SignalControl::init()
  {
    using namespace Prefs;
    ESP_LOGD(tag, "init hardware for signaling...done");

#ifdef RAWLED
    SignalControl::ledObject = new LedControl();
#endif
#ifdef LEDSTRIPE
    SignalControl::ledObject = new LedStripeControl();
#endif
#ifdef LEDPWM
    SignalControl::ledObject = new LedPwmControl();
#endif
    SignalControl::ledObject->init();
    //
    // Timer starten
    //
    SignalControl::startTimer();
    ESP_LOGD(tag, "init hardware for signaling...done");
  }

  void SignalControl::allOff()
  {
    SignalControl::ledObject->allOff();
  }

  void SignalControl::flashControlLED()
  {
    // Contol LED blitzen lassen
    controlLedSwitchedOffDelta = Prefs::BLINK_LED_CONTROL_NORMAL_ON;
  }

  void SignalControl::flashCrossLED()
  {
    controlLedSwitchedOffDelta = Prefs::BLINK_LED_CONTROL_CROSS_ON;
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
    // timer starten, microsekunden ( timerPeriod_us soll es)
    //
    ESP_ERROR_CHECK(esp_timer_start_periodic(SignalControl::timerHandle, timerPeriod_us));
    //
  }

  /**
   * @brief Timer alle timerPeriod_us us
   *
   */
  void SignalControl::timerCallback(void *)
  {
    //
    // alle 100 ms
    //
    using namespace Prefs;
    static bool isBusy{false};
    static uint32_t busyCount{0U};
    static int64_t nextChange{0LL};
    static bool isControlLedOn = false;
    static bool isPumpLedOn = false;
    static bool isRainLedOn = false;
    static bool isApModeOn = false;
    static bool wasAttentionLEDOn = false;

    if (isBusy)
    {
      ++busyCount;
      return;
    }
    isBusy = true;

    int64_t nowTime{esp_timer_get_time()};

    if (Preferences::isAttentionFlag || busyCount > 0)
    {
      static bool attentionLEDIsOn = false;
      wasAttentionLEDOn = true;
      //
      //  Achtungzeichen, LEDS flackern
      //
      if (nowTime > nextChange)
      {
        if (attentionLEDIsOn)
        {
          // Blink led aus
          SignalControl::ledObject->setAttentionLED(false);
          nextChange = nowTime + Prefs::BLINK_LED_ATTENTION_OFF;
          attentionLEDIsOn = false;
        }
        else
        {
          SignalControl::ledObject->setAttentionLED(true);
          nextChange = nowTime + Prefs::BLINK_LED_ATTENTION_ON;
          attentionLEDIsOn = true;
        }
        SignalControl::ledObject->makeChange();
      }
      isBusy = false;
      return;
    }
    else if (wasAttentionLEDOn)
    {
      SignalControl::ledObject->allOff();
      wasAttentionLEDOn = false;
      isBusy = false;
      return;
    }

    if (Preferences::appOpMode == opMode::APMODE)
    {
      if (nowTime > nextChange)
      {
        if (isApModeOn)
          nextChange = nowTime + Prefs::BLINK_LED_CONTROL_AP_OFF;
        else
          nextChange = nowTime + Prefs::BLINK_LED_CONTROL_AP_ON;
        isApModeOn = !isApModeOn;
        SignalControl::ledObject->setAPModeLED(isApModeOn);
        SignalControl::ledObject->makeChange();
      }
      isBusy = false;
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
          if (Preferences::appOpMode == opMode::CROSS)
            SignalControl::ledObject->setControlCrossLED(false);
          else
            SignalControl::ledObject->setControlLED(false);
          isControlLedOn = false;
          controlLedSwitchedOff = 0ULL;
        }
      }
      else
      {
        if (!isControlLedOn)
        {
          // Contrtol LED an
          if (Preferences::appOpMode == opMode::CROSS)
            SignalControl::ledObject->setControlCrossLED(true);
          else
            SignalControl::ledObject->setControlLED(true);
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
        SignalControl::ledObject->setPumpLED(true);
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
          // Pumnp LED AUS
          if (SignalControl::ledObject->fadeOutPumpLED())
          {
            // Dimmen erledigt
            isPumpLedOn = false;
            pumpLedSwitchedOff = 0ULL;
          }
          else
          {
            // das dimmen dauert noch (gib ihm noch 60 ms)
            pumpLedSwitchedOff = nowTime + 60 * 1000;
          }
        }
      }
    }

    if (rainLedSwitchedOff != 0uLL)
    {
      if (isRainLedOn)
      {
        // ausschalten
        SignalControl::ledObject->setRainLED(false);
      }
    }
    SignalControl::ledObject->makeChange();
    isBusy = false;
  }
}
