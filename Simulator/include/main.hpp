#pragma once
#include <Arduino.h>
#include <Ticker.h>

//
// IO Ports definieren
//
constexpr int LED_INTERNAL = 02;
constexpr int TACHO_OUT = 05;
//
// Tacho definitionen
//
constexpr double timerFreq = 5000000.0;  // 200 ns
constexpr double pulsePerRound = 109.0;
constexpr double weelScope = 1.8;
constexpr double speedFactor = ( 60.0 * 60.0 );

//
// simulation
//
constexpr long MAX_SPEED = 200;
constexpr long MIN_SPEED = 10;
//
// Prototypen
//
void setup();
void loop();
uint32_t pulsesForKmh( double speed );
ICACHE_RAM_ATTR void timerIsr();
volatile uint32_t g_counter;
