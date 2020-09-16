/*
 * GTopGPS.h
 *
 *  Created on: 29 mai 2019
 *      Author: hugo
 */

#include <stdint.h>

#pragma once

//M10 GTOP
#define stdFLEN        0x3F  // 63 bytes
#define pos_GPSlat     0x05  // 4 byte
#define pos_GPSlon     0x09  // 4 byte
#define pos_GPSalt     0x0D  // 3 byte
#define pos_GPSvE      0x10  // 2 byte
#define pos_GPSvN      0x12  // 2 byte
#define pos_GPSvU      0x14  // 2 byte
#define pos_GPStime    0x16  // 3 byte
#define pos_GPSdate    0x19  // 3 byte
#define pos_Check      0x3D  // 2 byte



uint32_t get_4bytes(uint8_t * packet, unsigned int pos) ;

uint32_t get_3bytes(uint8_t * packet, unsigned int pos) ;

short get_2bytes(uint8_t * packet, unsigned int pos) ;

struct Position {
    // Degrees multiplied by 1e6
    int32_t Lat;
    // Degrees multiplied by 1e6
    int32_t Lon;
    int32_t Alt;
};

struct Speed {
    int vE;
    int vN;
    int vU;
    float vH;
    int Cap;
    float Vz;
};

struct Datation {
    uint32_t Date;
    // Decimal 100226 stands for 10h02m26s
    uint32_t Time;
};

class GTopGPS
{
public :
    GTopGPS() ;

    bool encode( uint8_t character ) ;

    static bool checkM10(uint8_t *msg, int len) ;

    Position getPosition() ;

    Speed getSpeed() ;

    Datation getTime() ;

    void clear() ;

    // Format time as hhmmss
    void getTime( unsigned char string[] ) ;

    // Format position as ddmm.hhN/dddmm.hhE
    // Return trus if position is OK
    bool getPosition( unsigned char string[] ) ;


private :

    unsigned int index_ ;
    uint8_t packet_[stdFLEN] ;
};

