//=============================================================
// 
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Sep 2023
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

//=============================================================
// VUserMain1.c
//=============================================================

#include <stdio.h>
#include <stdlib.h>
#include "pcie.h"
#include "dpi_header.h"

static int node = 1;

//-------------------------------------------------------------
// VUserInput_1()
//
// Consumes the unhandled input Packets
//-------------------------------------------------------------

static void VUserInput_1(pPkt_t pkt, int status, void* usrptr) 
{
    int idx;

    if (pkt->seq == DLLP_SEQ_ID) 
    {
        DebugVPrint("---> VUserInput_1 received DLLP\n");
    }
    else
    {
        DebugVPrint("---> VUserInput_1 received TLP sequence %d of %d bytes at %d\n", pkt->seq, GET_TLP_LENGTH(pkt->data), pkt->TimeStamp);
    }

    // Once packet is finished with, the allocated space *must* be freed.
    // All input packets have their own memory space to avoid overwrites
    // which shared buffers.
    DISCARD_PACKET(pkt);
}

//-------------------------------------------------------------
// VUserMain1()
//
// Endpoint complement to VUserMain0. Initialises link and FC
// before sending idles indefinitely.
//
//-------------------------------------------------------------

void VUserMain1() 
{
    VPrint("Entered VUserMain1\n");
    
    int notReset = 0;

    // Initialise PCIe VHost, with input callback function and no user pointer.
    InitialisePcie(VUserInput_1, NULL, node);

    // Make sure the link is out of electrical idle
    VWrite(LINK_STATE, 0, 0, node);

    DebugVPrint("VUserMain: in node %d\n", node);

    // Use node number as seed
    PcieSeed(node, node);

    // Send out idles until reset removed
    do
    {
        SendOs(IDL, node);
        PcieGetReset(&notReset);
    }
    while (!notReset);

    // Initialise the link for 16 lanes
    InitLink(16, node);

    // Initialise flow control
    InitFc(node);

    // Send out idles forever
    while (true)
    {
        SendIdle(100, node);
    }
}
    
