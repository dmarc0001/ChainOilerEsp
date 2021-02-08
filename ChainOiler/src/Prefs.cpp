#include "Prefs.hpp"

namespace Preferences
{
  //! Voreinstellungen beim Start
  const char *Prefs::serialStr = "20210208-201936-build-0435";
  String Prefs::WLANSSID{ defaultSSID };
  String Prefs::WLANPassword{ defaultPassword };
  double Prefs::pulsePerWeelRound{ defaultPulsePerWeelRound };
  double Prefs::weelCircumFerence{ defaultWeelCircumFerence };
  double Prefs::normalOilInterval{ defaultNormalOilInterval };
  double Prefs::rainOilIntervalFactor{ defaultRainOilIntervalFactor };
  double Prefs::crossOilIntervalFactor{ defaultCrossOilIntervalFactor };
  double Prefs::speedProgressionFactor{ defaultSpeedProgressionFactor };
  int Prefs::threshodRainSensor{ defaultPumpLedLightingTime };
  uint32_t Prefs::pumpLedLightingTime{ defaultPumpLedLightingTime };
  volatile bool Prefs::isTachoAction{ false };
  fClick Prefs::lastAction{ fClick::NONE };
  uint32_t Prefs::lastActionDownTime{ 0L };
  uint32_t Prefs::lastActionUpTime{ 0L };
  opMode Prefs::mode{ opMode::NORMAL };
  volatile uint32_t Prefs::tachoPulseCount{ 0 };
  volatile uint32_t Prefs::tachoPulseForSpeedCount{ 0 };
  uint32_t Prefs::tachoPulseActionOnCount{ 0 };
  uint32_t Prefs::pulsesPerMeasuredRoute{
      ( static_cast< uint32_t >( floor( MEASURE_ROUTE_METERS / weelCircumFerence ) * defaultPulsePerWeelRound ) ) };
  uint32_t Prefs::measuresMsPerRouteMeters{ 0 };
  bool Prefs::functionSwitchDown{ false };

  bool Prefs::initPrefs()
  {
    SPI.begin();
    bool initOk = false;
    initOk = LittleFS.begin();
    if ( !( initOk ) )  // Format FS, of not formatted. - Try 1
    {
      Serial.println( "LittleFS filesystem format..." );
      LittleFS.format();
      initOk = LittleFS.begin();
    }
    if ( !( initOk ) )  // Format LittleFS. - Try 2
    {
      LittleFS.format();
      initOk = LittleFS.begin();
    }
    if ( initOk )
    {
      Serial.println( "LittleFs is OK" );
    }
    else
    {
      Serial.println( "LittleFs is NOT OK" );
    }
#ifdef DEBUG
    printPrefs();
#endif

    return initOk;

    // TODO: Preferenzen aus Festspeicher lesen oder defaults setzten
    // TODO: Nichtflüchtigen Speicher init, auslesen oder neu beschreiben
    //
  }

#ifdef DEBUG
  void Prefs::printPrefs()
  {
    Serial.print( "Prefs::serialStr: " );
    Serial.println( Prefs::serialStr );
    //
    Serial.print( "Prefs::WLANSSID: " );
    Serial.println( Prefs::WLANSSID );
    //
    Serial.print( "Prefs::pulsePerWeelRound: " );
    Serial.println( Prefs::pulsePerWeelRound );
    //
    Serial.print( "Prefs::weelCircumFerence: " );
    Serial.println( Prefs::weelCircumFerence );
    //
    Serial.print( "Prefs::normalOilInterval: " );
    Serial.println( Prefs::normalOilInterval );
    //
    Serial.print( "Prefs::rainOilIntervalFactor: " );
    Serial.println( Prefs::rainOilIntervalFactor );
    //
    Serial.print( "Prefs::crossOilIntervalFactor: " );
    Serial.println( Prefs::crossOilIntervalFactor );
    //
    Serial.print( "Prefs::speedProgressionFactor: " );
    Serial.println( Prefs::speedProgressionFactor );
    //
    Serial.print( "Prefs::threshodRainSensor: " );
    Serial.println( Prefs::threshodRainSensor );
    //
    Serial.print( "Prefs::pumpLedLightingTime: " );
    Serial.println( Prefs::pumpLedLightingTime );
    //
    Serial.print( "Prefs::mode (NORMAL): " );
    Serial.println( Prefs::mode );
    //
    Serial.print( "Prefs::tachoPulseActionOnCount: " );
    Serial.println( Prefs::tachoPulseActionOnCount );
    //
    Serial.print( "Prefs::pulsesPerMeasuredRoute: " );
    Serial.println( Prefs::pulsesPerMeasuredRoute );
    //
    delay( 1000 );
  }
#endif

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

  void Prefs::computeTachoActionCountValue( double pFactor )
  {
    double factor = pFactor;

    switch ( mode )
    {
      case opMode::AWAKE:
      case opMode::NORMAL:
        // wird vorgegeben
        // factor = 1.0;
        break;
      case opMode::RAIN:
        factor = rainOilIntervalFactor;
        break;
      case opMode::CROSS:
        factor = crossOilIntervalFactor;
        break;
      case opMode::APMODE:
        factor = 1.0;
        break;
      case opMode::TEST:
        factor = 1.0;
        break;
    };
    //
    // errechne den korrekten Wert mit Faktor
    //
    // Strecke in Metern * impulse per Umdrehung / Rdumfang in Metern, das ganze mal Faktor
    // ergibt die Strecke, bei der eine Aktion ausgelöst wird
    //
    tachoPulseActionOnCount =
        static_cast< uint32_t >( floor( ( ( normalOilInterval * pulsePerWeelRound ) / weelCircumFerence ) / factor ) );
    Serial.print( "oil interval: " );
    Serial.print( normalOilInterval );
    Serial.print( " m, pulses per round: " );
    Serial.print( pulsePerWeelRound );
    Serial.print( ", wheel_c: " );
    Serial.print( weelCircumFerence );
    Serial.print( " m, factor: " );
    Serial.print( factor );
    Serial.print( " result: " );
    Serial.println( tachoPulseActionOnCount );
  }

  /**
   * Wie lange soll die LED leuchten
   */
  uint32_t Prefs::getTimeForPumpLedFlash()
  {
    return pumpLedLightingTime;
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

  int Prefs::getThreshodRainSensor()
  {
    return threshodRainSensor;
  }

  uint32_t Prefs::getTachoPulseCount()
  {
    return tachoPulseCount;
  }

  uint32_t Prefs::getTachoPulseActionOnCount()
  {
    return tachoPulseActionOnCount;
  }

  uint32_t Prefs::computeSpeed()
  {
    // Berechne Geschwindigkeit in m/s
    uint32_t km = Preferences::MEASURE_ROUTE_METERS * 1000UL;
    if ( km > measuresMsPerRouteMeters && measuresMsPerRouteMeters > 0 )
    {
      uint32_t value = static_cast< uint32_t >( km / measuresMsPerRouteMeters );
      return value;
    }
    return 0;
  }

  double Prefs::getSpeedProgressionFactor()
  {
    return speedProgressionFactor;
  }

  void Prefs::makeDefaults()
  {
    using namespace Preferences;
    tachoPulseCount = 0;
    //
    // Aktion nach normalOilInterval Metern bei Radumfang weelCircumFerence und pulsePerWeelRound Pulsen per Umdrehung
    //
    computeTachoActionCountValue( 1.0 );
    // SPEED_PROGRESSION_FACTOR
  }

}  // namespace Preferences