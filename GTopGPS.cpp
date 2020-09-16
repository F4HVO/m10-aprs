/*
 * GTopGPS.cpp
 *
 *  Created on: 29 mai 2019
 *      Author: hugo
 */

#include "GTopGPS.h"
#include <math.h>

uint32_t get_4bytes(uint8_t * packet, unsigned int pos) {
  int i;
  uint32_t val = 0 ;
  for (i = 0; i < 4; i++) {
    val |= uint32_t(packet[pos + i]) << uint32_t(8 * (3 - i));
  }
  return val;
}

uint32_t get_3bytes(uint8_t * packet, unsigned int pos) {
  int i;
  uint32_t val = 0 ;
  for (i = 0; i < 3; i++) {
    val |= uint32_t(packet[pos + i]) << uint32_t(8 * (2 - i));
  }
  return val;
}

short get_2bytes(uint8_t * packet,unsigned int pos) {
  return  packet[pos] << 8 | packet[pos + 1];
}



GTopGPS::GTopGPS( )
: index_(0)
{
}


bool GTopGPS::encode( uint8_t character ) {
    // Skip first character which seems random
    if ( index_ == 0 )
    {
        ++index_ ;
        return false ;
    }
    packet_[index_-1] = character ;
    ++index_;
    if ( index_ == stdFLEN+1 )
    {
        index_ = 0 ;
        return checkM10( packet_, stdFLEN ) ;
    }
    return false ;
}


Position
GTopGPS::getPosition() {
    Position p ;
    p.Lat = get_4bytes(packet_,pos_GPSlat) ;
    p.Lon = get_4bytes(packet_,pos_GPSlon) ;
    p.Alt = get_3bytes(packet_,pos_GPSalt) ;
    return p ;
}

// Not tested
Speed
GTopGPS::getSpeed() {
    Speed s ;
    short vel16;
    float vx, vy, vz, dir;
    vel16 = get_2bytes(packet_, pos_GPSvE);// east
    s.vE = vel16;
    vx = vel16 / 1e2; // east
    vel16 = get_2bytes(packet_, pos_GPSvN);
    s.vN = vel16;
    vy = vel16 / 1e2; // north
    vel16 = get_2bytes(packet_, pos_GPSvU);//up
    s.vU = vel16;
    vz = vel16 / 1e2;
    s.vH = sqrt(vx * vx + vy * vy);
    dir = atan2(vx, vy) * 180 / M_PI;
    if (dir < 0) dir += 360;
    s.Cap = dir;
    s.Vz = vz;
    return s ;
}

Datation
GTopGPS::getTime() {
    Datation d ;
    d.Time = get_3bytes(packet_, pos_GPStime) ;
    d.Date = get_3bytes(packet_, pos_GPSdate) ;
    return d ;
}

void
GTopGPS::clear() {
    index_ = 1 ;
}

// Format time as hhmmss
void GTopGPS::getTime( unsigned char string[] )
{
    uint32_t date = getTime().Time ;

    uint32_t hour = date / 10000 ;
    uint32_t minute = (date % 10000) / 100 ;
    uint32_t seconde = date % 100 ;

    string[0] = '0' + hour / 10 ;
    string[1] = '0' + hour % 10 ;
    string[2] = '0' + minute / 10 ;
    string[3] = '0' + minute % 10 ;
    string[4] = '0' + seconde / 10 ;
    string[5] = '0' + seconde % 10 ;
}


// Deg is in deg * 1e6
void
formatPos( uint32_t deg, bool isLat, unsigned char string[], char direction )
{
    unsigned char i = 0 ;
    if ( isLat )
    {
        // Lat deg
        string[i] = '0' + deg / 10000000 ;
    }
    else
    {
        // Lon deg
        string[i] = '0' + deg / 100000000 ;
        string[++i] = '0' + ( deg % 100000000 ) / 10000000 ;
    }

    string[++i] = '0' + ( deg % 10000000 ) / 1000000 ;

    // Minute
    int minute = ( deg % 1000000 ) * 6 / 100000 ;
    string[++i] = '0' + minute / 10 ;
    string[++i] = '0' + minute % 10 ;

    string[++i] = '.' ;

    // hundredth of minute
    int hundredth = ( deg % 10000 ) * 6 / 1000 ;
    string[++i] = '0' + hundredth / 10 ;
    string[++i] = '0' + hundredth % 10 ;

    string[++i] = direction ;
}

// Format position as ddmm.hhN/dddmm.hhE (hh are hundredth of a minute)
bool GTopGPS::getPosition( unsigned char string[] )
{
    Position p = getPosition() ;

    // Check that position is valid
    if ( p.Lat == 89999999 && p.Lon == 0 )
    {
        const char noPos[] = "0   .  N/0    .  E" ;
        for ( unsigned char  i = 0 ; i < sizeof(noPos) - 1 ; ++i )
        {
            string[i] = noPos[i] ;
        }
        return false ;
    }

    // Handle coordinate signs
    if ( p.Lat >= 0 )
    {
        formatPos( p.Lat, true, string, 'N' ) ;
    }
    else
    {
        formatPos( -p.Lat, true, string, 'S' ) ;
    }
    if ( p.Lon >= 0 )
    {
        formatPos( p.Lon, false, string + 9, 'E' ) ;
    }
    else
    {
        formatPos( -p.Lon, false, string + 9, 'W' ) ;
    }

    return true ;
}

// Checksum used to validate GPS packet
unsigned int CRC16_2(unsigned char *buf, int len)
{
    unsigned int crc = 0xFFFF;
    for (unsigned int pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned int)buf[pos];    // XOR byte into least sig. byte of crc

        // Loop over each bit
        for (int i = 8; i != 0; i--)
        {
            // If the LSB is set
            if ((crc & 0x0001) != 0)
            {
                // Shift right and XOR 0xA001
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                // Else LSB is not set
                // Just shift right
                crc >>= 1;
            }
        }
    }

    return crc;
}

// Check that GPS packet is valid
bool GTopGPS::checkM10(uint8_t *msg, int len) {
    int cs = CRC16_2( msg, len-2) ;
    cs = cs & 0xFFFF;
    int cs1 = (msg[pos_Check] << 8) | msg[pos_Check + 1];
    return (cs1 == cs);
}
