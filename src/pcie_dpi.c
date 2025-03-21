//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Mar 2023
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
//
// Top level VP DPI routines. Calls relevant user function
// for given node number.
//
//=============================================================

#include <stdint.h>
#include <errno.h>

#include "svdpi.h"
#include "pcie_dpi.h"

// References to the two possible nodes' user programs
EXTERN void VUserMain0(void);
EXTERN void VUserMain1(void);

// -------------------------------------------------------------------------
// Main routine called whenever PcieInit imported DPI task invoked from
// initial block of module.
// -------------------------------------------------------------------------

EXTERN void PcieInit (int node)
{
    // Ensure that the Aldec tools can intercept the stdout stream
    setvbuf(stdout, 0, _IONBF, 0);

    VPrint("PcieInit(%d)\n", node);

    // Call appropriate user code for given node
    switch(node)
    {
    case 0: VUserMain0(); break;
    case 1: VUserMain1(); break;
    default:
        VPrint("***Error: PcieInit() got out of range node number (%d)\n", node);
        exit(1);
        break;
    }
}

// -------------------------------------------------------------------------
// Message exchange routine. Handles all messages to and from exported
// DPI tasks (apart from initialisation). Each sent
// message has a reply.
// -------------------------------------------------------------------------

static void VExch (psend_buf_t psbuf, prcv_buf_t prbuf, uint32_t node)
{
    switch(node)
    {
    case 0:
        svSetScope(svGetScopeFromName("test.host"));
        PcieUpdate0(psbuf->addr,
                    psbuf->data_out,
                    (int*)&prbuf->data_in,
                    psbuf->rw == V_WRITE ? 0 : 1,
                    psbuf->ticks);
        break;
     case 1:
        svSetScope(svGetScopeFromName("test.ep"));
        PcieUpdate1(psbuf->addr,
                    psbuf->data_out,
                    (int *)&prbuf->data_in,
                    psbuf->rw == V_WRITE ? 0 : 1,
                    psbuf->ticks);
        break;
    }
}

// -------------------------------------------------------------------------
// Invokes a write message exchange
// -------------------------------------------------------------------------

EXTERN int VWrite (unsigned int Addr, unsigned int Data, int Delta, uint32_t node)
{
    rcv_buf_t rbuf;
    send_buf_t sbuf;

    sbuf.addr     = Addr;
    sbuf.data_out = Data;
    sbuf.rw       = V_WRITE;
    sbuf.ticks    = Delta ? DELTA_CYCLE : 1;

    VExch(&sbuf, &rbuf, node);

    return rbuf.data_in ;
}

// -------------------------------------------------------------------------
// Invokes a read message exchange
// -------------------------------------------------------------------------

EXTERN int VRead (unsigned int Addr, unsigned int *rdata, int Delta, uint32_t node)
{
    rcv_buf_t rbuf;
    send_buf_t sbuf;

    sbuf.addr     = Addr;
    sbuf.data_out = 0;
    sbuf.rw       = V_READ;
    sbuf.ticks    = Delta ? DELTA_CYCLE : 1;

    VExch(&sbuf, &rbuf, node);

    *rdata = rbuf.data_in;

    return 0;
}


