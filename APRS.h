/*
 * APRS.h
 *
 *  Created on: 27 Apr 2020
 *      Author: hugo, adapted from F4GOH DRAPRS : https://github.com/f4goh/DRAPRS
 */

#pragma once

class APRS
{
public:

    void preparePacket( unsigned char buffer[], unsigned char size_array,
                        unsigned char outputData[], unsigned int * outputSize  ) ;


private :

    void flipout() ;
    void fcsbit( unsigned short tbyte ) ;
    void sendBit( unsigned char outputData[], unsigned int * outputSize ) ;
    void sendByte ( unsigned char inbyte, unsigned char outputData[], unsigned int * outputSize ) ;


private :
    bool flip_freq_ = false ;
    unsigned short crc_ = 0xffff ;
    unsigned char flag_ = 0 ;
    unsigned char fcsflag_ = 0 ;
    unsigned char stuff_ = 0 ;

    volatile int shift_;

    int bitIndex_ = 0 ;
};
