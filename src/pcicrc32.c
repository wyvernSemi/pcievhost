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
//=============================================================
//
// VPI functions for verilog task $pcicrc32 and $pcicrc16
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
#include <vpi_user.h>

#include "VSched_pli.h"

#define CRC32ARGS char* userdata
#define CRC16ARGS char* userdata

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

#define POLY            0x04c11db7U
#define CRCSIZE         32
#define BIT31           0x80000000U

#define ARGS_ARRAY_SIZE 10

int PciCrc32(CRC32ARGS)
{
    int i;
    unsigned Crc;

#if !defined(VPROC_SV)

    unsigned Data;
    int Bits;
    int            args[ARGS_ARRAY_SIZE];

    vpiHandle      taskHdl;

    // Obtain a handle to the argument list
    taskHdl            = vpi_handle(vpiSysTfCall, NULL);

    getArgs(taskHdl, &args[1]);

    Data = args[1];
    Crc  = args[2];
    Bits = args[3];

#else
    Crc  = *Crc32;
#endif

    for (i = 0; i < Bits; i++)
        Crc = (Crc << 1UL) ^ ((((Crc & BIT31) ? 1 : 0) ^ ((Data >> i) & 1)) ? POLY : 0);

#ifndef VPROC_SV

    args[2] = Crc;
    updateArgs(taskHdl, &args[1]);

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
    unsigned Data;
    int            args[ARGS_ARRAY_SIZE];

    vpiHandle      taskHdl;

    // Obtain a handle to the argument list
    taskHdl            = vpi_handle(vpiSysTfCall, NULL);

    getArgs(taskHdl, &args[1]);

    Data = args[1];
    Crc  = args[2];

#else
    Crc  = *Crc16;
#endif

    for (i = 0; i < 32; i++)
        Crc = (Crc << 1UL) ^ ((((Crc & BIT16) ? 1 : 0) ^ ((Data >> i) & 1)) ? POLY16 : 0);

#ifndef VPROC_SV

    args[2] = Crc;
    updateArgs(taskHdl, &args[1]);

#else
    *Crc16  = Crc;
#endif

    return 0;
}
