#include "Prefs.hpp"

namespace Preferences
{
  const char *Prefs::serialStr = "20210126-104601-build-0136";

  volatile bool Prefs::isTachoAction{ false };
  fClick Prefs::lastAction{ fClick::NONE };
  opMode Prefs::mode{ opMode::NORMAL };
  volatile uint32_t Prefs::tachoPulseCount{ 0 };
  uint32_t Prefs::tachoPulseActionOn{ 0 };
  bool Prefs::functionSwitchDown{ false };

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
    return lastAction;
  }

  void Prefs::setLastAction( fClick action )
  {
    lastAction = action;
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