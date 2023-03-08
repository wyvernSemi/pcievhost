//=====================================================================
//
// pcie_dpi.c                                         Date: 2023/03/06
//
// Copyright (c) 2004-2023 Simon Southwell.
//
// This file is part of pcievhost.
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=====================================================================
//
// Top level VP user thread routines. Sets up connection to
// queue and calls relevant user function for node number.
//
//=====================================================================

#include <stdint.h>
#include <errno.h>
#include "pcie_dpi.h"
#include "dpi_header.h"

// Pointers to state for each node (up to VP_MAX_NODES)
pSchedState_t ns[VP_MAX_NODES];

// References to the two node's user programs
extern void VUserMain0(void);
extern void VUserMain1(void);

/////////////////////////////////////////////////////////////
// SystemVerilog exported DPI routines

extern void PcieUpdate0(int addr, int datain, int* dataout, int rnw, int ticks);
extern void PcieUpdate1(int addr, int datain, int* dataout, int rnw, int ticks);

/////////////////////////////////////////////////////////////
// Main routine called whenever PcieInit DPI task invoked from
// initial block of module.
//
int PcieInit (int node)
{
    // Ensure that the Aldec tools can intercept the stdout stream
    setvbuf(stdout, 0, _IONBF, 0);

    VPrint("PcieInit(%d)\n", node);

    // Range check node number
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("***Error: PcieInit() got out of range node number (%d)\n", node);
        exit(1);
    }

    DebugVPrint("PcieInit(): node = %d\n", node);

    // Allocate some space for the node state and update pointer
    ns[node] = (pSchedState_t) malloc(sizeof(SchedState_t));

    // Range check node number
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("***Error: VInit() got out of range node number (%d)\n", node);
        exit(1);
    }

    switch(node)
    {
    case 0: VUserMain0(); break;
    case 1: VUserMain1(); break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////
// Message exchange routine. Handles all messages to and from
// simulation process (apart from initialisation. Each sent
// message has a reply. Interrupt messages require that
// the original IO message reply is waited for again.
//
static void VExch (psend_buf_t psbuf, prcv_buf_t prbuf, uint32_t node)
{
    switch(node)
    {
    case 0:
        DebugVPrint("VExch: Calling PcieUpdate0\n");
        PcieUpdate0(psbuf->addr,
                    psbuf->data_out,
                    &prbuf->data_in,
                    psbuf->rw == V_WRITE ? 0 : 1,
                    psbuf->ticks);
        DebugVPrint("VExch: PcieUpdate0 returned\n");
        break;
     case 1:
        DebugVPrint("VExch: Calling PcieUpdate1\n");
        PcieUpdate1(psbuf->addr,
                    psbuf->data_out,
                    &prbuf->data_in,
                    psbuf->rw == V_WRITE ? 0 : 1,
                    psbuf->ticks);
        DebugVPrint("VExch: PcieUpdate1 returned\n");
        break;
    }
}

/////////////////////////////////////////////////////////////
// Invokes a write message exchange
//
int VWrite (unsigned int Addr, unsigned int Data, int Delta, uint32_t node)
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

/////////////////////////////////////////////////////////////
// Invokes a read message exchange
//
int VRead (unsigned int Addr, unsigned int *rdata, int Delta, uint32_t node)
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

/////////////////////////////////////////////////////////////
// Invokes a tick message exchange
//
int VTick (unsigned int ticks, uint32_t node)
{
    rcv_buf_t rbuf;
    send_buf_t sbuf;

    sbuf.addr     = 0;
    sbuf.data_out = 0;
    sbuf.rw       = V_IDLE;
    sbuf.ticks    = ticks;

    VExch(&sbuf, &rbuf, node);

    return 0;
}


