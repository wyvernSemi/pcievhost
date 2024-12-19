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
//=============================================================

#ifndef _CODEC_H_
#define _CODEC_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#ifndef OSVVM
#include "VUser.h"
#endif
#include "pcie.h"

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

#define BYTEWIDTH 8
#define SHIFTREGWIDTH 16

// Logic definitions for Decoder
#define Data9        (data & 0x200)
#define Data8        (data & 0x100)
#define Data7        (data & 0x080)
#define Data6        (data & 0x040)
#define Data5        (data & 0x020)
#define Data4        (data & 0x010)
#define Data3        (data & 0x008)
#define Data2        (data & 0x004)
#define Data1        (data & 0x002)
#define Data0        (data & 0x001)
#define Data5xnor4  (((data & 0x030) == 0x030) || ((data & 0x030) == 0x000))
#define Data5and4    ((data & 0x030) == 0x030)
#define Data5nor4    ((data & 0x030) == 0x000)
#define Data1and0    ((data & 0x003) == 0x003)
#define Data1nor0    ((data & 0x003) == 0x000)
#define Data2and1    ((data & 0x006) == 0x006)
#define Data2nor1    ((data & 0x006) == 0x000)
#define Data5to3and  ((data & 0x038) == 0x038)
#define Data5to2nor  ((data & 0x03c) == 0x000)
#define Data5to2and  ((data & 0x03c) == 0x03c)
#define Data7and6    ((data & 0x0c0) == 0x0c0)
#define Data7nor6    ((data & 0x0c0) == 0x000)
#define Data9and8    ((data & 0x300) == 0x300)
#define Data9nor8    ((data & 0x300) == 0x000)
#define Data9xor8   (((data & 0x300) == 0x200) || ((data & 0x300) == 0x100))
#define Data9to7nor  ((data & 0x380) == 0x000)
#define Data9to7and  ((data & 0x380) == 0x380)
#define Data8to6nor  ((data & 0x1c0) == 0x000)

#define DEFAULTLFSRVALUE           0xffff

// CRC definitions
#define TLP_CRC_INITIAL_VALUE      0xffffffff
#define DLLP_CRC_INITIAL_VALUE     0xffff

#define TLPPOLY                    0x04c11db7U
#define TLPCRCSIZE                 32
#define MAX_CRC_TOP_BIT            0x80000000U
#define DLLPPOLY                   0x100b
#define DLLPCRCSIZE                16
#define MAXCRCSIZE                 TLPCRCSIZE

// -------------------------------------------------------------------------
// TYPEDEFS
// -------------------------------------------------------------------------

typedef struct {
    int code;
    int disparity;
} TblType;

typedef struct {
    int      rd [MAX_LINK_WIDTH];                         // Encode running disparity for each lane
    uint32_t elfsr;                                       // Encoder shift regsiter
    uint32_t dlfsr;                                       // Decoder shift register

    bool     ts_active;
    int     last_lane0_sym;
} CodecState_t, *pCodecState_t;

// -------------------------------------------------------------------------
// PROTOTYPES
// -------------------------------------------------------------------------

extern unsigned int Encode    (const int      data, const int no_scramble, const int no_8b10b,  const int lane, const int linkwidth, const int node);
extern unsigned int Decode    (const int      data, const int no_scramble, const int no_8b10b,  const int lane, const int linkwidth, const int node);
extern uint32_t     PciCrc    (const uint32_t Data, const uint32_t CrcIn,  const uint32_t Bits, const uint32_t poly, const uint32_t crcsize);
extern void         InitCodec (const int node);

#endif

