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
// $Id: mem.h,v 1.4 2016/10/10 13:08:54 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/mem.h,v $
//
//=============================================================

#ifndef _MEM_H_
#define _MEM_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include "VUser.h"

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

#define TABLESIZE      (4096UL)
#define TABLEMASK      (TABLESIZE-1)

#define MEM_BAD_STATUS  1
#define MEM_GOOD_STATUS 0

// -------------------------------------------------------------------------
// TYPEDEFS
// -------------------------------------------------------------------------

typedef struct {
    char** p;
    uint64 addr;
    bool   valid;
} PrimaryTbl_t, *pPrimaryTbl_t;

typedef uint16  PktData_t;
typedef uint16* pPktData_t;

// -------------------------------------------------------------------------
// PROTOTYPES
// -------------------------------------------------------------------------

extern void   InitialiseMem       (int node);

extern void   WriteRamByteBlock   (const uint64 addr, const PktData_t* const data, const int fbe, const int lbe, const int length, const uint32 node);
extern int    ReadRamByteBlock    (const uint64 addr, PktData_t* const data, const int length, const uint32 node);
extern void   WriteConfigSpace    (const uint32 addr, const uint32 data, const uint32 node);
extern uint32 ReadConfigSpace     (const uint32 addr, const uint32 node);
extern void   WriteConfigSpaceBuf (const uint32 addr, const PktData_t* const data, const int fbe, const int lbe, const int length, const uint32 node);
extern void   ReadConfigSpaceBuf  (const uint32 addr, PktData_t* const data, const int length, const uint32 node);

extern void   WriteRamByte        (const uint64 addr, const uint32 data, const uint32 node);
extern void   WriteRamWord        (const uint64 addr, const uint32 data, const int little_endian, const uint32 node);
extern void   WriteRamDWord       (const uint64 addr, const uint64 data, const int little_endian, const uint32 node);
extern uint32 ReadRamByte         (const uint64 addr, const uint32 node);
extern uint32 ReadRamWord         (const uint64 addr, const int little_endian, const uint32 node);
extern uint64 ReadRamDWord        (const uint64 addr, const int little_endian, const uint32 node);
#endif
