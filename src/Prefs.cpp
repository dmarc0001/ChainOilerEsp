#include "Prefs.hpp"

namespace Preferences
{
  //! Voreinstellungen beim Start
  const char *Prefs::serialStr = "20210126-232148-build-0185";
  volatile bool Prefs::isTachoAction{ false };
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
    // TODO: Nichtflüchtigen Speucher init, auslesen oder neu beschreiben
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
   * Ist der Funktionsschalter gedrückt?
   */
  bool Prefs::getFunctionSwitchDown()
  {
    return functionSwitchDown;
  }

  fClick Prefs::getLastAction()
  {
    if ( functionSwitchDown )
    {
      return fClick::NONE;
    }
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