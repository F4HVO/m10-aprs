/*
 * M10Configuration.cpp
 *
 *  Created on: 27 Apr 2020
 *      Author: hugo
 */

#include <M10Configuration.h>

#include <msp430.h>


namespace M10
{
    // Configure uc to use external crystal XT2 at 8MHz
    void useXT2clock()
    {
        // DCOx Bit 7,6,5 = frequency select (8 discretes frequencies)
        // MODx Bit 4,3,2,1,0 = modulator selection
        //DCOCTL = 0x0 ;

        // XT2OFF Bit 7 = 0 : XT2 ON
        // XTS Bit 6 LFXT1 = 0 : low freq, 1 high freq
        // DIVAx Bit 5,4 : divider for ACLK (1 2 4 8)
        // XT5V Bit 3 unused
        // RSELx Bit 2,1,0 = resistor select
        BCSCTL1 = BIT2 | BIT1 | BIT0 ;
        // SELMx Bit 7,6 select MCLK = 00 : DCOCLK, 01 DCOCLK, 10 XT2CLK (or LFXT1CLK if no XT2 on chip), 11 LFXT1CLK
        // DIVMx  Bit 5,4 divider for mclk (1 2 4 8)
        // SELS Bit 3 select SMCLK source = 0 DCOCLK, 1 XT2CLK (if XT2 present on chip, LFXT1CLK otherwise)
        // DIVSx Bit 2,1 divider for SMCLK (1 2 4 8)
        // DCOR Bit 0 DCO resistor select = 0 internal, 1 external
        BCSCTL2 = BIT7 | BIT3  ;
        // XT2Sx Bit 7,6 range select
        // LFXT1Sx Bit 5,4 lf clock select
        // XCAPx Bit Bit 3,2 oscillator capacitor section, 1 6 10 12.5pF
        // XT2OF Bit 1 oscillator fault 0 : no fault condition present
        // LFXT1OF Bit 0 lfxt1 oscillator fault
        BCSCTL3 = BIT7 | BIT5 | BIT2;

        do
        {
            // Clear XT2,XT1,DCO fault flags
            // Clear fault flags
            IFG1 &= ~OFIFG;
            // Delay
            __delay_cycles(1000);
        } while ( (IFG1 & OFIFG) == OFIFG ) ; // Test oscillator fault flag
        // DEBUG : WHY fault flat is always on
    }

    // Set watchdog timer, output led, use external clock, power supply
    void setup()
    {
        // stop watchdog timer
        WDTCTL = WDTPW | WDTHOLD ;

        // configure P5.7 (led pin) as output
        P5DIR |= BIT7 ;

        // Use external oscillator
        useXT2clock();

        // Output SMClock
        // P5.5 output
        P5DIR |= BIT5 ;
        P5SEL |= BIT5 ;

        // Configure GPS UART on ports 3.4 3.5
        P3SEL |= BIT5 + BIT4 ;
        // SMCLK
        UCA0CTL1 |= UCSSEL_2 ;

        // GPS serial baudrate = 38400
        UCA0BR0=0xD0 ;
        UCA0BR1=0x00 ;
        UCA0MCTL=0x06 ;

        // Initialize USCI state machine
        UCA0CTL0 = 0;
        UCA0CTL1 &= ~UCSWRST;
        // Enable USCI_A0 RX interrupt
        IE2 |= UCA0RXIE;

        // Configure timer A
        initTimerA() ;

        // Terminate unused ports
        P1DIR = 0xFF;
        P1OUT = 0x00;
        P6DIR = 0xFF;
        P6OUT = 0x00;
    }

    // Turn on main power supply
    void mainPower( bool turnOn )
    {
        // Turn on main power
        // 2.1
        P2DIR |= BIT1 ;

        // 3.0
        P3DIR |= BIT0 ;

        if ( turnOn )
        {
            P2OUT |= BIT1 ;
            P3OUT |= BIT0 ;
        }
        else
        {
            P2OUT &= ~BIT1 ;
            P3OUT &= ~BIT0 ;
        }
    }

    // Turn on GPS power supply
    void gpsPower( bool turnOn )
    {
        // Configure 3.2 as output (GPS power supply switch)
        P3DIR |= BIT2 ;
        if ( turnOn )
        {
            // Turn GPS on
            P3OUT |= BIT2 ;
        }
        else
        {
            // Turn GPS off
            P3OUT &= ~BIT2 ;
        }
    }

    // Turn on synthesizer power
    void synthPower( bool turnOn )
    {
        // Configure 3.1 as output
        P3DIR |= BIT1 ;
        if ( turnOn )
        {
            // Turn on
            P3OUT |= BIT1 ;
        }
        else
        {
            // Turn off
            P3OUT &= ~BIT1 ;
        }
    }

    // Toggle led
    void toggleLed()
    {
        P5OUT ^= BIT7 ;
    }

    // Digital write equivalent
    void digitalWrite( Pin pin, Level level )
    {
        switch ( pin )
        {
            // 4.0
            case ADFTxData :
            {
                if ( level == HIGH )
                {
                    P4OUT |= BIT0 ;
                }
                else
                {
                    P4OUT &= ~BIT0 ;
                }
                break ;
            }
            // 2.3
            case ADFLE :
            {
                if ( level == HIGH )
                {
                    P2OUT |= BIT3 ;
                }
                else
                {
                    P2OUT &= ~BIT3 ;
                }
                break ;
            }
            // 2.5
            case ADFClock :
            {
                if ( level == HIGH )
                {
                    P2OUT |= BIT5 ;
                }
                else
                {
                    P2OUT &= ~BIT5 ;
                }
                break ;
            }
            // 2.4
            case ADFData :
            {
                if ( level == HIGH )
                {
                    P2OUT |= BIT4 ;
                }
                else
                {
                    P2OUT &= ~BIT4 ;
                }
                break ;
            }
            // 2.7
            case PA :
            {
                if ( level == HIGH )
                {
                    P2OUT |= BIT7 ;
                }
                else
                {
                    P2OUT &= ~BIT7 ;
                }
                break ;
            }
            // 5.7
            case LED :
            {
                if ( level == HIGH )
                {
                    P5OUT |= BIT7 ;
                }
                else
                {
                    P5OUT &= ~BIT7 ;
                }
                break ;
            }
        }
    }

    // pinMode equivalent
    void pinMode( Pin pin, PinKind pinKind )
    {
        switch ( pin )
        {
            // 4.0
            case ADFTxData :
            {
                if ( pinKind == OUTPUT )
                {
                    P4DIR |= BIT0 ;
                }
                break ;
            }
            // 2.3
            case ADFLE :
            {
                if ( pinKind == OUTPUT )
                {
                    P2DIR |= BIT3 ;
                }
                break ;
            }
            // 2.5
            case ADFClock :
            {
                if ( pinKind == OUTPUT )
                {
                    P2DIR |= BIT5 ;
                }
                break ;
            }
            // 2.4
            case ADFData :
            {
                if ( pinKind == OUTPUT )
                {
                    P2DIR |= BIT4 ;
                }
                break ;
            }
            // 2.7
            case PA :
            {
                if ( pinKind == OUTPUT )
                {
                    P2DIR |= BIT7 ;
                }
                break ;
            }
        }
    }

    // Delay (in ms)
    void delay( int ms )
    {
        for ( int i = 0 ; i < ms ; ++i )
        {
            __delay_cycles( MASTER_CLOCK_FREQUENCY / 1000 ) ;
        }
    }

    // Delay in microseconds
    void delayMicroseconds( int us )
    {
        //TACCR0 = us ;
        //__bis_SR_register(LPM0_bits + GIE);
        for ( int i = 0 ; i < us ; ++i )
        {
            __delay_cycles( MASTER_CLOCK_FREQUENCY / 1000000 ) ;
        }
    }


    // Timer handling
    void initTimerA()
    {
        //Timer0_A3 Configuration
        TACCR0 = 0; //Initially, Stop the Timer
        TACCTL0 |= CCIE; //Enable interrupt for CCR0.
        TACTL = ID_3 + TASSEL_1 + MC_1; //Select ACLK, ACLK/8, Up Mode
    }

    // Setup power off interrupt
    void setupPowerOff()
    {
        // Power off button : 2.0
        // Use as GPIO
        P2SEL &= ~BIT0 ;
        // As input
        P2DIR &= ~BIT0 ;
        // Enable interrupt
        P2IE |= BIT0 ;
        // On falling edge
        P2IES |= BIT0 ;
    }
}

