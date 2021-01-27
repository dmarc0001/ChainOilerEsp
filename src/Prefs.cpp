#include "Prefs.hpp"

namespace Preferences
{
  //! Voreinstellungen beim Start
  const char *Prefs::serialStr = "20210127-191828-build-0238";
  volatile bool Prefs::isTachoAction{ false };
  static uint32_t timeForPumpLedFlash{ pumpLedLightingTime };
  fClick Prefs::lastAction{ fClick::NONE };
  uint32_t Prefs::lastActionDownTime{ 0L };
  uint32_t Prefs::lastActionUpTime{ 0L };
  opMode Prefs::mode{ opMode::NORMAL };
  volatile uint32_t Prefs::tachoPulseCount{ 0 };
  uint32_t Prefs::tachoPulseActionOn{ 0 };
  bool Prefs::functionSwitchDown{ false };

  void Prefs::initPrefs()
  {
    // TODO: Preferenzen aus Festspeicher lesen oder defaults setzten
    // TODO: Nichtflüchtigen Speicher init, auslesen oder neu beschreiben

    //
    tachoPulseActionOn = static_cast< uint32_t >( floor( normalOilInterval / ( weelCircumFerence * pulsePerWeelRound ) ) );
  }

  /**
   * Tacho Action ist gesetzt!
   */
  void Prefs::setTachoAction( bool val )
  {
    // Semaphoren im esp8266 nicht verfügbar
    isTachoAction = val;
  }

  /**
   * ISt eine Aktion gesetzt von der ISR?
   */
  bool Prefs::getTachoAction()
  {
    return isTachoAction;
  };

  /**
   * Wie lange soll die LED leuchten
   */
  uint32_t Prefs::getTimeForPumpLedFlash()
  {
    return timeForPumpLedFlash;
  }
  /**
   * Ist der Funktionsschalter gedrückt?
   */
  bool Prefs::getFunctionSwitchDown()
  {
    return functionSwitchDown;
  }

  bool Prefs::getLongClickTimeElapsed()
  {
    if ( functionSwitchDown )
    {
      // Taste unten, erwarte weiteres
      // die zweit für long abgelaufen?
      uint32_t timeDiff = millis() - lastActionDownTime;
      if ( timeDiff > longClickTimeMs )
      {
        return true;
      }
    }
    return false;
  }

  fClick Prefs::getLastAction()
  {
    //
    // Taste noch unten
    //
    if ( functionSwitchDown )
    {
      if ( lastActionUpTime > lastActionDownTime )
      {
        uint32_t timeDiff = lastActionUpTime - lastActionDownTime;
        if ( timeDiff > longClickTimeMs )
        {
          lastAction = fClick::LONG;
        }
      }
      return fClick::NONE;
    }
    else
    {
      //
      // das entprellt die Taste
      //
      if ( lastActionUpTime > lastActionDownTime )
      {
        uint32_t timeDiff = lastActionUpTime - lastActionDownTime;
        if ( timeDiff > deBounceTimeMs )
        {
          //
          // Okay entprellt
          //
          if ( timeDiff > longClickTimeMs )
          {
            lastAction = fClick::LONG;
          }
          else
          {
            lastAction = fClick::SHORT;
          }
          return lastAction;
        }
      }
    }
    return fClick::NONE;
  }

  void Prefs::clearLastAction()
  {
    lastAction = fClick::NONE;
    lastActionUpTime = lastActionDownTime = millis();
  }

  void Prefs::setOpMode( opMode _mode )
  {
    mode = _mode;
  }

  opMode Prefs::getOpMode()
  {
    return mode;
  }

  void Prefs::makeDefaults()
  {
    using namespace Preferences;
    tachoPulseCount = 0;
    //
    // Aktion nach normalOilInterval Metern bei Radumfang weelCircumFerence und pulsePerWeelRound Pulsen per Umdrehung
    //
    tachoPulseActionOn = static_cast< uint32_t >( floor( normalOilInterval / ( weelCircumFerence * pulsePerWeelRound ) ) );
  }

}  // namespace Preferences