/*
 * M10Configuration.h
 *
 *  Created on: 27 Apr 2020
 *      Author: hugo
 */

#pragma once

namespace M10 {

    typedef enum {
        ADFTxData,
        ADFLE,
        ADFClock,
        ADFData,
        PA,
        LED
    } Pin ;

    typedef enum {
        LOW,
        HIGH
    } Level ;

    typedef enum {
        INPUT,
        OUTPUT
    } PinKind ;

    // Main clock frequency 8MHz
    const long MASTER_CLOCK_FREQUENCY = 8000000 ;

    // Configure uc to use external crystal XT2 at 8MHz
    void useXT2clock() ;

    // Set watchdog timer, output led, use external clock, power supply
    void setup() ;

    // Turn on main power supply
    void mainPower( bool turnOn ) ;

    // Turn on GPS power supply
    void gpsPower( bool turnOn  ) ;

    // Turn on synthesizer power
    void synthPower( bool turnOn ) ;

    // Toggle led
    void toggleLed() ;

    // Digital write equivalent
    void digitalWrite( Pin pin, Level level ) ;

    // pinMode equivalent
    void pinMode( Pin pin, PinKind pinKind ) ;

    // Delay (in ms)
    void delay( int ms ) ;

    // Delay in microseconds
    void delayMicroseconds( int us ) ;

    // Configure Timer A
    void initTimerA() ;

    // Setup power off interrupt
    void setupPowerOff() ;
}
