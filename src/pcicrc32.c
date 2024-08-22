//=============================================================
// 
// Copyright (c) 2016 Simon Southwell. All rights reserved.
//
// Date: 20th Sep 2016
//
// This file is part of the pcieVHost package.
//
// pcieVHost is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pcieVHost is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pcieVHost. If not, see <http://www.gnu.org/licenses/>.
//
// $Id: pcicrc32.c,v 1.2 2016/10/07 08:33:39 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/pcicrc32.c,v $
//
//=============================================================
//
// PLI functions for verilog task $pcicrc32 and $pcicrc16
//
// Calculates CRC for up to 32 bit data.
//
// Arg 1 Data
//     2 Current CRC/ Output CRC
//     3 Number of data bits
//
//=============================================================

#include <stdio.h>

#ifndef VPROC_SV
#include "veriuser.h"

#define CRC32ARGS void
#define CRC16ARGS void
#else

#define CRC32ARGS const unsigned Data, unsigned *Crc32, const int Bits
#define CRC16ARGS const unsigned Data, unsigned *Crc16  

#endif

// -------------------------------------------------------------------------
// PciCrc32()
// 
// PLI 32 bit CRC code for $pcicrc32 task
//
// -------------------------------------------------------------------------

#define POLY    0x04c11db7U
#define CRCSIZE 32
#define BIT31   0x80000000U

int PciCrc32(CRC32ARGS)
{
    int i;
    unsigned Crc;
    
#ifndef VPROC_SV
    unsigned Data;
    int Bits;
    
    Data = tf_getp(1);
    Crc  = tf_getp(2);
    Bits = tf_getp(3);
#else
    Crc  = *Crc32;
#endif

    for (i = 0; i < Bits; i++) 
        Crc = (Crc << 1UL) ^ ((((Crc & BIT31) ? 1 : 0) ^ ((Data >> i) & 1)) ? POLY : 0);

#ifndef VPROC_SV
    tf_putp(2, (unsigned int)(Crc));
#else
    *Crc32  = Crc;
#endif

    return 0;
}

// -------------------------------------------------------------------------
// PciCrc16()
//
// PLI 16 bit CRC code for $pcicrc16 task
//
// -------------------------------------------------------------------------

#define POLY16    0x100b
#define CRCSIZE16 16
#define BIT16     0x8000U

int PciCrc16(CRC16ARGS)
{
    int i;
    unsigned Crc;
    
#ifndef VPROC_SV
    unsigned int Data;
    
    Data = tf_getp(1);
    Crc  = tf_getp(2);
#else
    Crc  = *Crc16;
#endif

    for (i = 0; i < 32; i++) 
        Crc = (Crc << 1UL) ^ ((((Crc & BIT16) ? 1 : 0) ^ ((Data >> i) & 1)) ? POLY16 : 0);

#ifndef VPROC_SV
    tf_putp(2, (unsigned int)(Crc & 0xffff));
#else
    *Crc16 = Crc;
#endif

    return 0;
}
