//=============================================================
//
// Copyright (c) 2016-2024 Simon Southwell. All rights reserved.
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

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------
#include <stdint.h>

#include "pcie.h"
#include "pcie_vhost_map.h"
#include "displink.h"

// -------------------------------------------------------------------------
// STATICS
// -------------------------------------------------------------------------

static pPrimaryTbl_t PrimaryTable[VP_MAX_NODES];
static uint8_t       **pCfgSpace       = NULL;
static uint8_t       **pCfgSpaceMask   = NULL;

// -------------------------------------------------------------------------
// InitialiseMem()
//
// Intiliases memory table to a NULL state
//
// -------------------------------------------------------------------------

void InitialiseMem (int node)
{
    if (PrimaryTable[node] != NULL)
    {
        free(PrimaryTable[node]);
        PrimaryTable[node] = NULL;
    }
}

// -------------------------------------------------------------------------
// InitialisePrimaryTable()
//
// Invalidates all entries in the memory primary table
//
// -------------------------------------------------------------------------

static void InitialisePrimaryTable (const pPrimaryTbl_t table)
{
    int i;

    for (i = 0; i < TABLESIZE; i++)
    {
        table[i].valid = false;
    }
}

// -------------------------------------------------------------------------
// InitialiseTable()
//
// Initialise a secondary table
//
// -------------------------------------------------------------------------

static void InitialiseTable (char **table)
{
    int i;

    for (i = 0; i < TABLESIZE; i++)
    {
        table[i] = NULL;
    }
}

// -------------------------------------------------------------------------
// bitrev()
//
// An efficient bit reverse, up to 32 bits.
//
// -------------------------------------------------------------------------

static uint32_t bitrev(const uint32_t Data, const int bits)
{
    unsigned long result = Data;
    int i;

    // Compare each of the bottom bits with reflected top bits
    for (i = 0; i < bits/2; i++)
    {
        // If top and bottom bits different, invert both bits
        if (((result >> (bits-1-i*2)) ^ result) & (1 << i))
        {
            result ^= (1 << i) | (1 << (bits-1-i));
        }
    }

    return result & ((1 << bits) - 1);
}

// -------------------------------------------------------------------------
// GenHash12()
//
// A cheap and cheerful hash
//
// -------------------------------------------------------------------------

static uint32_t GenHash12(const uint64_t addr)
{
    uint64_t munge = 0;

    munge ^= (addr >> 52) & 0xfffULL;
    munge ^= (addr >> 40) & 0xfffULL;
    munge ^= (addr >> 32) & 0xffULL;
    munge ^= (addr >> 24) & 0xffULL;

    return bitrev((uint32_t) (munge & 0xfffULL), 12);
}

// -------------------------------------------------------------------------
// WriteRamByteBlock()
//
// Write a block of data to memory
//
// -------------------------------------------------------------------------

void WriteRamByteBlock(const uint64_t addr, const PktData_t *data, const int fbe, int const lbe, const int length, const uint32_t node)
{
    uint32_t pidx, sidx, offset;
    int idx;

    idx = pidx = GenHash12(addr);
    sidx = (addr >> 12) & TABLEMASK;
    offset = addr & TABLEMASK;

    if ((addr & ~TABLEMASK) != ((addr + length - 1) & ~TABLEMASK))
    {
        VPrint("WriteRamByteBlock: %s***Error --- block write crosses 4K boundary (addr=0x%llx len=0x%x%s\n", FMT_RED, (long long unsigned)addr, length, FMT_NORMAL);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // No primary table, so allocate some space for one and initialise
    if (PrimaryTable[node] == NULL)
    {
        if ((PrimaryTable[node] = malloc(TABLESIZE * sizeof(PrimaryTbl_t))) == NULL)
        {
            VPrint("WriteRamByteBlock: %s***Error --- failed to allocate primary table memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        InitialisePrimaryTable(PrimaryTable[node]);
    }

    // Whilst we have a collision, increment primary offset until an invalid entry, or we matched address
    while (PrimaryTable[node][pidx].valid && PrimaryTable[node][pidx].addr != (addr & 0xffffffffff000000ULL))
    {
        pidx = (pidx+1) % TABLESIZE;

        // If we have searched through the whole table....
        if (pidx == idx)
        {
            VPrint("WriteRamByteBlock: %s***Error --- ran out of primary table space%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }

    // If first time we have written to this block, validate it
    if (!PrimaryTable[node][pidx].valid)
    {
        PrimaryTable[node][pidx].valid = true;
        PrimaryTable[node][pidx].addr = (addr & 0xffffffffff000000ULL);
        PrimaryTable[node][pidx].p = NULL;
    }

    // No secondary table, so allocate some space for one and initialise
    if (PrimaryTable[node][pidx].p == NULL)
    {
        if ((PrimaryTable[node][pidx].p = malloc(TABLESIZE * sizeof(uint32_t *))) == NULL)
        {
            VPrint("WriteRamByteBlock: %s***Error --- failed to allocate secondary table memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        InitialiseTable(PrimaryTable[node][pidx].p);
    }

    // No memory block allocated, so allocate some space
    if ((PrimaryTable[node][pidx].p)[sidx] == NULL)
    {
        if (((PrimaryTable[node][pidx].p)[sidx] = malloc(TABLESIZE)) == NULL)
        {
            VPrint("WriteRamByteBlock: %s***Error --- failed to allocate memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }

    for (idx = 0; idx < length; idx++)
    {
        if ( (idx < 4 && ((1<<idx) & fbe)) ||
             (idx >= (length-4) && ((1<<(4-(length-idx))) & lbe)) ||
             (idx >= 4 && idx < (length-4)))
        {
            ((char *)((PrimaryTable[node][pidx].p)[sidx]))[idx+offset] = data[idx];
        }
    }
}

// -------------------------------------------------------------------------
// ReadRamByteBlock()
//
// Read a block of data from memory.
//
// -------------------------------------------------------------------------

int ReadRamByteBlock(const uint64_t addr, PktData_t *data, const int length, const uint32_t node)
{
    uint32_t pidx, sidx, offset;
    int idx, len;

    idx = pidx = GenHash12(addr);
    sidx = (addr >> 12) & TABLEMASK;
    offset = addr & TABLEMASK;

    if ((addr & ~TABLEMASK) != ((addr + length) & ~TABLEMASK))
    {
        VPrint("ReadRamByteBlock: %s***Error --- block read crosses 4K boundary%s\n", FMT_RED, FMT_NORMAL);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (PrimaryTable[node] == NULL)
    {
        VPrint("ReadRamByteBlock: %s***Error --- reading from uninitialised primary table%s\n", FMT_RED, FMT_NORMAL);
        return MEM_BAD_STATUS;
    }

    // Whilst we have detected a collision, increment primary offset until an invalid entry or we matched address
    while (PrimaryTable[node][pidx].valid && PrimaryTable[node][pidx].addr != (addr & 0xffffffffff000000ULL))
    {
        pidx = (pidx+1) % TABLESIZE;

        // If we searched the whole table...
        if (pidx == idx)
        {
            VPrint("ReadRamByteBlock: %s***Error --- address does not exist in primary table%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }

    // No secondary table, so flag an error
    if (PrimaryTable[node][pidx].p == NULL)
    {
        VPrint("ReadRamByteBlock: %s***Error --- reading from uninitialised secondary table%s\n", FMT_RED, FMT_NORMAL);
        return MEM_BAD_STATUS;
    }

    // No memory block allocated, so flag an error
    if ((PrimaryTable[node][pidx].p)[sidx] == NULL)
    {
        VPrint("ReadRamByteBlock: %s***Error --- reading from uninitialised memory block%s\n", FMT_RED, FMT_NORMAL);
        return MEM_BAD_STATUS;
    }

    for (idx = 0; idx < length; idx++)
    {
        data[idx] = ((char *)(PrimaryTable[node][pidx].p)[sidx])[idx+offset] & 0xff;
    }

    return MEM_GOOD_STATUS;
}

// -------------------------------------------------------------------------
// WriteRamByte()
//
// Write a data byte to memory.
//
// -------------------------------------------------------------------------

void WriteRamByte(const uint64_t inaddr, const uint32_t data, const uint32_t node)
{
    uint64_t addr;
    int addr_lo, fbe;
    PktData_t buf[4];

    addr = inaddr & ~3ULL;
    addr_lo = (int)(inaddr & 3ULL);
    fbe  = 0x1 << addr_lo;
    buf[addr_lo] = data & 0xff;

    WriteRamByteBlock (addr, buf, fbe, 0, 4, node);
}

// -------------------------------------------------------------------------
// WriteRamHWord()
//
// Write a data half word to memory.
//
// -------------------------------------------------------------------------

void WriteRamHWord (const uint64_t addr, const uint32_t data, const int le, const uint32_t node)
{
    uint32_t data_out;
    int addr_lo, fbe;
    PktData_t buf[4];
    int i;

    addr_lo  =  (int)(addr & 2ULL);
    fbe      = le ? (0x3 << addr_lo) : (0xc >> addr_lo) ;
    data_out = (addr_lo) ? (data << 16) : data;

    for (i = 0; i < 4; i++)
    {
        buf[i] = (le ? (data_out >> (i*8))  & 0xffffUL: (data_out >> ((3-i)*8))) & 0xff;
    }

    WriteRamByteBlock (addr & ~3ULL, buf, fbe, 0x0, 4, node);
}

// -------------------------------------------------------------------------
// WriteRamWord()
//
// Write a data word to memory.
//
// -------------------------------------------------------------------------

void WriteRamWord (const uint64_t addr, const uint32_t data, const int le, const uint32_t node)
{
    PktData_t buf[4];
    int i;

    for (i = 0; i < 4; i++)
    {
        buf[i] = (le ? (data >> (i*8)) : (data >> ((3-i)*8))) & 0xff;
    }

    WriteRamByteBlock (addr & ~3ULL, buf, 0xf, 0x0, 4, node);
}

// -------------------------------------------------------------------------
// WriteRamDWord()
//
// Write a double data word (64 bits) to memory.
//
// -------------------------------------------------------------------------

void WriteRamDWord (const uint64_t addr, const uint64_t data, const int le, const uint32_t node)
{
    PktData_t buf[8];
    int i;

    for (i = 0; i < 8; i++)
    {
        buf[i] = (PktData_t) (le ? (data >> (i*8)) : (data >> ((7-i)*8))) & 0xffULL;
    }

    WriteRamByteBlock (addr & ~7ULL, buf, 0xf, 0xf, 8, node);
}

// -------------------------------------------------------------------------
// ReadRamByte()
//
// Read a byte from memory
//
// -------------------------------------------------------------------------

uint32_t ReadRamByte (const uint64_t addr, const uint32_t node)
{
    PktData_t buf[4];
    int i;

    // If ReadRamByteBlock fails, return 0
    if (ReadRamByteBlock (addr & ~3ULL, buf, 4, node))
    {
        return 0;
    }

    i = (int)(addr & 3ULL);

    return buf[i];
}

// -------------------------------------------------------------------------
// ReadRamHWord()
//
// Read a half word from memory
//
// -------------------------------------------------------------------------

uint32_t ReadRamHWord (const uint64_t addr, const int le, const uint32_t node)
{
    PktData_t buf[4];
    uint32_t data = 0;
    int addr_lo = addr & 0x2;
    int i;

    // If ReadRamByteBlock fails, return 0
    if (ReadRamByteBlock (addr & ~3ULL, buf, 4, node))
    {
        return 0;
    }

    for (i = 0; i < 4; i++)
    {
        data |= (buf[i] & 0xff) << (le ? (i*8) : ((3-i)*8));
    }

    return ((addr_lo) ? (data >> 16) : data) & 0xffff;
}

// -------------------------------------------------------------------------
// ReadRamWord()
//
// Read a word from memory
//
// -------------------------------------------------------------------------

uint32_t ReadRamWord (const uint64_t addr, const int le, const uint32_t node)
{
    PktData_t buf[4];
    uint32_t data = 0;
    int i;

    // If ReadRamByteBlock fails, return 0
    if (ReadRamByteBlock (addr & ~3ULL, buf, 4, node))
    {
        return 0;
    }

    for (i = 0; i < 4; i++)
    {
        data |= (buf[i] & 0xff) << (le ? (i*8) : ((3-i)*8));
    }

    return data;
}

// -------------------------------------------------------------------------
// ReadRamDWord()
//
// Read a double word (64 bits) from memory
//
// -------------------------------------------------------------------------

uint64_t ReadRamDWord (const uint64_t addr, const int le, const uint32_t node)

{
    PktData_t buf[8];
    uint64_t data = 0;
    int i;

    // If ReadRamByteBlock fails, return 0
    if (ReadRamByteBlock (addr & ~7ULL, buf, 8, node))
    {
        return 0ULL;
    }

    for (i = 0; i < 8; i++)
    {
        data |= ((uint64_t)(buf[i] & 0xff)) << (le ? (i*8) : ((7-i)*8));
    }

    return data;
}

// -------------------------------------------------------------------------
// WriteConfigSpace()
//
// Write word to the 4K confg space page (separate from memory)
//
// -------------------------------------------------------------------------

void WriteConfigSpace (const uint32_t addr, const uint32_t data, const uint32_t node)
{
    PktData_t buff[4];

    buff[3] = (data >> 24) & 0xff;
    buff[2] = (data >> 16) & 0xff;
    buff[1] = (data >>  8) & 0xff;
    buff[0] = (data >>  0) & 0xff;

    WriteConfigSpaceBuf(addr, buff, 0xf, 0xf, 4, false, node);

}

void WriteConfigSpaceBuf(const uint32_t addr, const PktData_t *data, const int fbe, const int lbe, const int length, bool use_mask, const uint32_t node)
{
    int  idx;
    char mask;

    if (pCfgSpace == NULL)
    {
        DebugVPrint("WriteConfigSpaceBuf:Allocate space for CfgSpace table\n");

        if ((pCfgSpace = malloc(sizeof(char*) * VP_MAX_NODES)) == NULL)
        {
            VPrint("WriteConfigSpaceBuf: %s***Error --- failed to allocate config space memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        for (idx = 0; idx < VP_MAX_NODES; idx++)
        {
            pCfgSpace[node] = NULL;
        }
    }

    // No config space, so allocate some space for one and initialise
    if (pCfgSpace[node] == NULL)
    {
        DebugVPrint("WriteConfigSpaceBuf:Allocate mem for CfgSpace[%d]\n", node);
        if ((pCfgSpace[node] = calloc(TABLESIZE, 1)) == NULL)
        {
            VPrint("WriteConfigSpace: %s***Error --- failed to allocate config space memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }

    for (idx = 0; idx < length; idx++)
    {
        // Generate mask if any specified, else make all bits writable
        if (use_mask && pCfgSpaceMask != NULL)
            if (pCfgSpaceMask[node] != NULL)
                mask = ~pCfgSpaceMask[node][addr + idx];
            else
                mask = 0xff;
        else
            mask = 0xff;

        if ( (idx < 4 && ((1<<idx) & fbe)) ||
             (idx >= (length-4) && ((1<<(4-(length-idx))) & lbe)) ||
             (idx >= 4 && idx < (length-4)))
        {
            pCfgSpace[node][addr + idx] = (pCfgSpace[node][addr + idx] & ~mask) | ((data[idx] & 0xff) & mask);
            DebugVPrint("*****WriteConfigSpaceBuf: %02x\n", pCfgSpace[node][addr + idx]);
        }
    }
}

// -------------------------------------------------------------------------
// ReadConfigSpace()
//
// Read word from the 4K config space page (separate from memory)
//
// -------------------------------------------------------------------------

uint32_t ReadConfigSpace(const uint32_t addr, const uint32_t node)
{
    PktData_t buff[4];
    uint32_t word = 0;

    ReadConfigSpaceBuf(addr & 0xffc, buff, 4, node);

    word = ((buff[3] & 0xff) << 24) |
           ((buff[2] & 0xff) << 16) |
           ((buff[1] & 0xff) <<  8) |
           ((buff[0] & 0xff) <<  0) ;

   return word;
}

// -------------------------------------------------------------------------
// ReadConfigSpaceBuf()
//
// Read a number of words from the configuration space into a data buffer
// with checking for unconfigured configuration space.
//
// -------------------------------------------------------------------------

void ReadConfigSpaceBuf(const uint32_t addr, PktData_t * const data, const int len, const uint32_t node)
{
    bool valid_config_space = ReadConfigSpaceBufChk(addr, data, len, true, node);
}

// -------------------------------------------------------------------------
// ReadConfigSpaceBufChk()
//
// Read a number of words from the configuration space into a data buffer
// with configurable checking for unconfigured configuration space.
//
// -------------------------------------------------------------------------

bool ReadConfigSpaceBufChk(const uint32_t addr, PktData_t * const data, const int len, const bool check, const uint32_t node)
{
    int  idx;
    bool valid_config_space = true;

    if (pCfgSpace == NULL)
        valid_config_space = false;
    else if (pCfgSpace[node] == NULL)
        valid_config_space = false;

    if (!valid_config_space)
    {
        if (check)
        {
            VPrint("ReadConfigSpaceBufChk: %s***Error --- reading from uninitialised config space%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }
    else
    {
        for (idx = 0; idx < len; idx++)
        {
            data[idx] = pCfgSpace[node][addr + idx];

            DebugVPrint("*****ReadConfigSpace : %02x\n", data[idx] );
        }
    }

    return valid_config_space;
}

// -------------------------------------------------------------------------
// WriteConfigSpaceMask()
//
// Write word to the 4K confg space page mask
//
// -------------------------------------------------------------------------

void WriteConfigSpaceMask (const uint32_t addr, const uint32_t data, const uint32_t node)
{
    PktData_t buff[4];

    buff[3] = (data >> 24) & 0xff;
    buff[2] = (data >> 16) & 0xff;
    buff[1] = (data >>  8) & 0xff;
    buff[0] = (data >>  0) & 0xff;

    WriteConfigSpaceMaskBuf(addr, buff, 4, node);
}

// -------------------------------------------------------------------------
// WriteConfigSpaceMaskBuf()
// -------------------------------------------------------------------------

void WriteConfigSpaceMaskBuf(const uint32_t addr, const PktData_t *data, const int length, const uint32_t node)
{
    int idx;

    if (pCfgSpaceMask == NULL)
    {
        DebugVPrint("WriteConfigSpaceMask:Allocate space for CfgSpace table\n");

        if ((pCfgSpaceMask = malloc(sizeof(char*) * VP_MAX_NODES)) == NULL)
        {
            VPrint("WriteConfigSpaceMaskBuf:%s ***Error --- failed to allocate config space memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        for (idx = 0; idx < VP_MAX_NODES; idx++)
        {
            pCfgSpaceMask[node] = NULL;
        }
    }

    // No config space, so allocate some space for one and initialise
    if (pCfgSpaceMask[node] == NULL)
    {
        DebugVPrint("WriteConfigSpaceMask:Allocate mem for CfgSpaceMask[%d]\n", node);
        if ((pCfgSpaceMask[node] = calloc(TABLESIZE, 1)) == NULL)
        {
            VPrint("WriteConfigSpaceMaskBuf: %s***Error --- failed to allocate config space memory%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }

    for (idx = 0; idx < length; idx++)
    {
        pCfgSpaceMask[node][addr + idx] = (data[idx] & 0xff);
        DebugVPrint("*****WriteConfigSpaceMaskBuf : %02x\n", pCfgSpaceMask[node][addr + idx]);
    }
}

// -------------------------------------------------------------------------
// ReadConfigSpaceMask()
//
// Read word from the 4K config space mask page
//
// -------------------------------------------------------------------------

uint32_t ReadConfigSpaceMask(const uint32_t addr, const uint32_t node)
{
    PktData_t buff[4];
    uint32_t word = 0;

    ReadConfigSpaceMaskBuf(addr & 0xffc, buff, 4, node);

    word = ((buff[3] & 0xff) << 24) |
           ((buff[2] & 0xff) << 16) |
           ((buff[1] & 0xff) <<  8) |
           ((buff[0] & 0xff) <<  0) ;

   return word;
}

// -------------------------------------------------------------------------
// ReadConfigSpaceMaskBuf()
//
// Read a number of words from the configuration space mask buffer into a
// data buffer with checking for unconfigured configuration space.
//
// -------------------------------------------------------------------------

void ReadConfigSpaceMaskBuf(const uint32_t addr, PktData_t * const data, const int len, const uint32_t node)
{
    bool valid_config_space = ReadConfigSpaceMaskBufChk(addr, data, len, true, node);
}

// -------------------------------------------------------------------------
// ReadConfigSpaceMaskBufChk()
//
// Read a number of words from the configuration space mask buffer into a
// data buffer with configurable checking for unconfigured configuration
// space.
//
// -------------------------------------------------------------------------

bool ReadConfigSpaceMaskBufChk(const uint32_t addr, PktData_t * const data, const int len, const bool check, const uint32_t node)
{
    int  idx;
    bool valid_config_space = true;

    if (pCfgSpaceMask == NULL)
        valid_config_space = false;
    else if (pCfgSpaceMask[node] == NULL)
        valid_config_space = false;

    if (!valid_config_space)
    {
        if (check)
        {
            VPrint("ReadConfigSpaceMaskBufChk: %s***Error --- reading from uninitialised config space%s\n", FMT_RED, FMT_NORMAL);
            VWrite(PVH_FATAL, 0, 0, node);
        }
    }
    else
    {
        for (idx = 0; idx < len; idx++)
        {
            data[idx] = pCfgSpaceMask[node][addr + idx];

            DebugVPrint("*****ReadConfigSpaceMaskBuf: %02x\n", data[idx] );
        }
    }

    return valid_config_space;
}
