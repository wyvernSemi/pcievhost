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

#ifndef _MEM_H_
#define _MEM_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef OSVVM
#include "VUser.h"
#endif

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
    uint64_t addr;
    bool     valid;
} PrimaryTbl_t, *pPrimaryTbl_t;

typedef uint16_t  PktData_t;
typedef uint16_t* pPktData_t;

// -------------------------------------------------------------------------
// PROTOTYPES
// -------------------------------------------------------------------------

extern void     InitialiseMem           (int node);

extern void     WriteRamByteBlock       (const uint64_t addr, const PktData_t* const data, const int fbe, const int lbe, const int length, const uint32_t node);
extern int      ReadRamByteBlock        (const uint64_t addr, PktData_t* const data, const int length, const uint32_t node);
                                        
extern void     WriteRamByte            (const uint64_t addr, const uint32_t data, const uint32_t node);
extern void     WriteRamWord            (const uint64_t addr, const uint32_t data, const int little_endian, const uint32_t node);
extern void     WriteRamDWord           (const uint64_t addr, const uint64_t data, const int little_endian, const uint32_t node);
extern uint32_t ReadRamByte             (const uint64_t addr, const uint32_t node);
extern uint32_t ReadRamWord             (const uint64_t addr, const int little_endian, const uint32_t node);
extern uint64_t ReadRamDWord            (const uint64_t addr, const int little_endian, const uint32_t node);
                                        
extern void     WriteConfigSpace        (const uint32_t addr, const uint32_t data, const uint32_t node);
extern uint32_t ReadConfigSpace         (const uint32_t addr, const uint32_t node);
extern void     WriteConfigSpaceBuf     (const uint32_t addr, const PktData_t* const data, const int fbe, const int lbe, const int length, const bool use_mask, const uint32_t node);
extern void     ReadConfigSpaceBuf      (const uint32_t addr, PktData_t* const data, const int length, const uint32_t node);
extern void     WriteConfigSpaceMask    (const uint32_t addr, const uint32_t data, const uint32_t node);
extern uint32_t ReadConfigSpaceMask     (const uint32_t addr, const uint32_t node);
extern void     WriteConfigSpaceMaskBuf (const uint32_t addr, const PktData_t* const data, const int length, const uint32_t node);
extern void     ReadConfigSpaceMaskBuf  (const uint32_t addr, PktData_t* const data, const int length, const uint32_t node);

#endif
