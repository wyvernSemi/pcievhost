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
// $Id: VUserMain1.c,v 1.2 2016/10/07 08:39:55 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/test/usercode/VUserMain1.c,v $
//
//=============================================================

//=============================================================
// VUserMain9.c
//=============================================================

#include <stdio.h>
#include <stdlib.h>
#include "pcie.h"

#define RST_DEASSERT_INT 4

static int node = 1;

static unsigned int Interrupt = 0;

int ResetDeasserted_1(void)
{
    Interrupt |= RST_DEASSERT_INT;
}

void VUserInput_1(pPkt_t pkt, int status, void* usrptr) 
{
    if (pkt->seq == DLLP_SEQ_ID)
    {
        VPrint("---> VUserInput_1 received DLLP\n");
        free(pkt->data);
        free(pkt);
    }
    else
    {
        VPrint("---> VUserInput_1 received TLP sequence %d\n", pkt->seq);
        free(pkt->data);
        free(pkt);
    }
}

void VUserMain1() 
{
    int idx;
    PktData_t buff[4096];
    PktData_t *pkt_p, *data_p;
    sPkt_t packet;
    int seq = 0, rid = 1, tag = 0;
    int tmp, i;

    uint64 addr;

    InitialisePcie(VUserInput_1, NULL, node);
    VWrite(LINK_STATE, 0, 0, node);

    VPrint("VUserMain: in node %d\n", node);

    VRegInterrupt(RST_DEASSERT_INT, ResetDeasserted_1, node);

    do
    {
        SendOs(SKP, node);
    } while (!Interrupt);

    Interrupt &= ~RST_DEASSERT_INT;

    // Send at least one SKIP ordered set after reset deasserted
    // to ensure scrambler is synchronised
    SendOs(SKP, node);
   
    for (i = 0; i < 0x7fffffff; i++)
    {
        SendIdle(1, node);
    }
}
