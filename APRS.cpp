/*
 * APRS.cpp
 *
 *  Created on: 27 Apr 2020
 *      Author: hugo
 */

#include <APRS.h>


void
APRS::preparePacket( unsigned char buffer[], unsigned char size_array,
                     unsigned char outputData[], unsigned int * outputSize )
{
    // Initialise variables
    *outputSize = 0 ;

    crc_= 0xffff ;
    stuff_ = 0 ;

    //init
    flip_freq_ = 1;

    sendBit( outputData, outputSize );

    //The variable flag is true if you are transmitted flags (7E's) false otherwise.
    flag_ = 1;
    //The variable fcsflag is true if you are transmitting FCS bytes, false otherwise.
    fcsflag_ = 0;

    for ( int i=0 ; i<50 ; i++ )
    {
        //Sends several flag bytes.
        sendByte( 0x7E, outputData, outputSize );
    }

    // done sending flags
    flag_ = 0;
    for( unsigned int i=0 ; i < size_array ; i++ )
    {
        //send the packet bytes
        sendByte( buffer[i], outputData, outputSize ) ;
    }
    //about to send the FCS bytes
    fcsflag_ = 1 ;

    // Send the CRC
    sendByte( (crc_ ^ 0xff), outputData, outputSize );
    crc_ >>= 8;
    sendByte( (crc_ ^ 0xff), outputData, outputSize );

    //done sending FCS
    fcsflag_ = 0;

    //about to send flags
    flag_ = 1;
    for (int i=0;i<20;i++)
    {
        //Sends Several flag bytes.
        sendByte(0x7E, outputData, outputSize);
    }
}

void APRS::flipout()
{
    //since this is a 0, reset the stuff counter
    stuff_ = 0;
    flip_freq_^=1;
}

void APRS::fcsbit(unsigned short tbyte)
{
    crc_ ^= tbyte;
    if (crc_ & 1)
    {
        crc_ = (crc_ >> 1) ^ 0x8408;  // X-modem CRC poly
    }
    else
    {
        crc_ = crc_ >> 1;
    }
}

void APRS::sendBit( unsigned char outputData[], unsigned int * outputSize )
{
    // Set byte to 0 before using it
    if ( bitIndex_ == 0 )
    {
        outputData[(*outputSize)] = 0 ;
    }

    outputData[(*outputSize)] |= flip_freq_ << bitIndex_ ;
    ++bitIndex_ ;
    if ( bitIndex_ == 8 )
    {
        bitIndex_ = 0 ;
        (*outputSize) += 1 ;
    }
}

void APRS::sendByte ( unsigned char inbyte, unsigned char outputData[], unsigned int * outputSize )
{
    unsigned char k, bt;

    for ( k = 0 ; k < 8 ; k++ )
    {
        // Read only the rightmost bit
        bt = inbyte & 0x01;
        if ( (fcsflag_==0) && (flag_==0) )
        {
            // do FCS calc, but only if this is not a flag or fcs byte
            fcsbit(bt) ;
        }

        if ( bt == 0 )
        {
            // if this bit is a zero, flip the output state
            flipout() ;
        }
        else
        {
            //otherwise if it is a 1, do the following:
            //increment the count of consecutive 1's
            stuff_++;

            //stuff an extra 0, if 5 1's in a row
            if ( (flag_==0) && (stuff_==5) )
            {
                //flip the output state to stuff a 0
                sendBit( outputData, outputSize );
                flipout();
            }
        }
        //go to the next bit in the byte
        inbyte = inbyte>>1;
        sendBit( outputData, outputSize );
    }
}

