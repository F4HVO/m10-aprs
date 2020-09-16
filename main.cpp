/*
 * main.cpp
 *
 *  Created on: 27 Apr 2020
 *      Author: hugo
 */

#include <msp430.h>
#include "APRS.h"
#include "M10Configuration.h"
#include "RadioAdf7012.h"
#include "GTopGPS.h"

#include <string.h>


// Gps frame decoder
GTopGPS gps ;

volatile bool newGpsPosition = false ;


unsigned  char     aprsPacket[72]= {
  'F'<<1, '4'<<1, 'H'<<1, 'V'<<1, 'O'<<1, ' '<<1, 0x60,               // APTT4 7 bytes (0-6)
  'F'<<1, '4'<<1, 'H'<<1, 'V'<<1, 'O'<<1, ' '<<1, ('0' + 10) << 1,    // F4HVO-11 7 bytes (7-13)
  'W'<<1, 'I'<<1, 'D'<<1, 'E'<<1, '1'<<1, ' '<<1, ('0' + 1) << 1,     // WIDE1-1 7 bytes (14-20)
  'W'<<1, 'I'<<1, 'D'<<1, 'E'<<1, '2'<<1, ' '<<1, ('0' + 1) << 1 | 1, // WIDE2-1 end ssid lsb =1 7 octets (21-27)
  0x03, 0xf0, // ctrl, pid 2 bytes (28-29)
  '/', '0', '0', '0', '0', '0', '0', 'h', //heure 8 (30-37)
  '0', '0', '0', '0', '.', '0', '0', 'N', '/', '0', '1', '2', '3', '4', '.', '6', '7', 'E', //lat, long 18 octets (38-55)
  '>', '7', '3', ' ',  'd', 'e', ' ', 'H', 'u', 'g', 'o', ' ', '!', ' ', ' ', ' '
};            // Comments 15 bytes (56-71)

// APRS packet format handling
APRS aprs ;

// Primitive machine state
// Nominal mode : start in SLEEPING state
// Test mode : start in TEST_MODE
enum { SENDING_POS,
       SLEEPING,
       TEST_MODE } beaconState = SLEEPING ;

// We will send several positions every time the beacon enters SENDING_POS state
unsigned char positionCounter = 0 ;

// We will sleep several "cycles"
unsigned char sleepCycles = 1 ;

// Binary packet to send over the air (packet prepared by APRS class)
unsigned char rawPacket[200] ;
unsigned int rawPacketSize = 0 ;

// Handle radio configuration and operation
RadioAdf7012 radio ;

int main()
{
    // Setup board
    M10::setup() ;

    // Keep board powered when user releases power button
    M10::mainPower( true ) ;

    // Turn on LED
    M10::digitalWrite( M10::LED, M10::LOW ) ;

    // Blink led to indicate startup
    for ( unsigned char i = 12 ; i > 0 ; --i )
    {
        M10::toggleLed() ;
        M10::delay(100) ;
    }

    // Setup button interrupt for power off
    M10::setupPowerOff() ;

    // Main loop
    while ( true )
    {
        switch ( beaconState )
        {
            // In this state, we wait for good GPS data, GPS UART keeps waking us up
            case SENDING_POS:
            {
                // Turn transmitter on
                M10::synthPower( true ) ;

                // Configure ADF 7012
                radio.setup() ;

                // If new GPS available, TX updated position
                if ( ! newGpsPosition )
                {
                    break ;
                }

                // A packet has been decoded, flash led
                newGpsPosition = false ;

                // Update packet with GPS data, continue if position is valid
                if ( ! gps.getPosition( aprsPacket + 38 ) )
                {
                    break ;
                }

                // Turn on led when TXing
                M10::digitalWrite( M10::LED, M10::HIGH ) ;

                gps.getTime( aprsPacket + 31 ) ;

                // Prepare raw APRS packet
                aprs.preparePacket( aprsPacket, 72, rawPacket, &rawPacketSize ) ;

                // Send packet over the air
                // Avoid GPS UART interrupt while sending data
                // We bit bang bits to transmitter therefore timing is the name of the game
                __disable_interrupt() ;
                radio.send_data( rawPacket, rawPacketSize ) ;
                __enable_interrupt() ;

                // TX has ended, turn off led
                M10::digitalWrite( M10::LED, M10::LOW ) ;

                // If we have sent enough positions, we can switch mode back to sleeping
                if ( --positionCounter == 0 )
                {
                    beaconState = SLEEPING ;

                    // Turn off transmitter
                    M10::synthPower( false ) ;

                    // Turn off GPS
                    M10::gpsPower( false ) ;

                    // We will sleep N cycles
                    sleepCycles = 1 ; // 8 ;

                    // Start sleep timer
                    TACCR0 = 65535 ;
                }

                break ;
            }

            // In this state, GPS and transmitter are off, timer wakes us up sleepCycles times
            case SLEEPING:
            {
                if ( --sleepCycles > 0 )
                {
                    // Start sleep timer again
                    TACCR0 = 65535 ;
                    break ;
                }

                // We have sleeped enough
                // Turn GPS ON
                M10::gpsPower( true ) ;

                // We now are in active state
                beaconState = SENDING_POS ;

                // We will send N positions when they are available
                positionCounter = 5 ;

                break ;
            }

            // Send continuously
            case TEST_MODE:
            {
                // Turn transmitter on
                M10::synthPower( true ) ;

                // Configure ADF 7012
                radio.setup() ;

                // Turn on led when TXing
                M10::digitalWrite( M10::LED, M10::HIGH ) ;

                // Prepare raw APRS packet
                aprs.preparePacket( aprsPacket, 72, rawPacket, &rawPacketSize ) ;

                // Send packet over the air
                radio.send_data( rawPacket, rawPacketSize ) ;

                M10::digitalWrite( M10::LED, M10::LOW ) ;

                TACCR0 = 7000 ;
            }
        }

        // Start sleeping
        // Enter LPM3, interrupts enabled
        __bis_SR_register( LPM3_bits + GIE );
    }
}

// Interrupt code for GPS RX
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{

  char c = UCA0RXBUF ;
  if ( gps.encode( c ) )
  {
      newGpsPosition = true ;

      // Reset parsing state (value stays unchanged)
      gps.clear() ;

      // Clear LPM3 bits from 0(SR)
      __bic_SR_register_on_exit(LPM3_bits);
  }
}

// Flag to handle power off sequence
bool isShutingDown = false ;

// Timer ISR
#pragma vector = TIMER0_A0_VECTOR
__interrupt void Timer_A_CCR0_ISR(void)
{
    // If the user is still pushing the switch
    if ( isShutingDown && ! ( P2IN & BIT0 ) )
    {
        M10::digitalWrite( M10::LED, M10::HIGH ) ;

        // Flash led to indicate shutdown
        for ( unsigned char i = 6 ; i > 0 ; --i )
        {
            M10::toggleLed() ;
            M10::delay(100) ;
        }

        // These violent delights have violent ends
        // Machine commits power off
        M10::mainPower( false ) ;

        // It only has power until human finger releases switch
        // This will happen any time soon
        // Good bye
    }
    else
    {
        isShutingDown = false ;
    }

    // Clear LPM3 bits from 0(SR)
    __bic_SR_register_on_exit(LPM3_bits);
}

// Port 2 interrupt service routine (power button)
#pragma vector=PORT2_VECTOR
__interrupt void Port_2(void)
{
    // Start end of life timer
    TACCR0 = 5000 ;

    // P2.0 IFG clear
    P2IFG &= (~BIT0) ;

    isShutingDown = true ;
}
