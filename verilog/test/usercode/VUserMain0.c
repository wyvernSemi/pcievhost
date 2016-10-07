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
// $Id: VUserMain0.c,v 1.2 2016-10-07 08:38:00 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/test/usercode/VUserMain0.c,v $
//
//=============================================================

//=============================================================
// VUserMain8.c
//=============================================================

#include <stdio.h>
#include <stdlib.h>
#include "pcie.h"

#define RST_DEASSERT_INT 4

static int node = 0;

static unsigned int Interrupt = 0;

//-------------------------------------------------------------
// ResetDeasserted()
//
// ISR for reset de-assertion. Clears interrupts state.
//
//-------------------------------------------------------------

int ResetDeasserted(void)
{
    Interrupt |= RST_DEASSERT_INT;
}

//-------------------------------------------------------------
// VUserInput_0()
//
// Consumes the unhandled input Packets
//-------------------------------------------------------------

void VUserInput_0(pPkt_t pkt, int status, void* usrptr) 
{
    int idx;

    if (pkt->seq == DLLP_SEQ_ID) 
    {
        VPrint("---> VUserInput_0 received DLLP\n");
        free(pkt->data);
        free(pkt);
    }
    else
    {
        VPrint("---> VUserInput_0 received TLP sequence %d of %d bytes at %d\n", pkt->seq, GET_TLP_LENGTH(pkt->data), pkt->TimeStamp);

	// If a complation, extract the TPL payload data and display
        if (pkt->ByteCount)
        {
	    // Get a pointer to the start of the payload data
            pPktData_t payload = GET_TLP_PAYLOAD_PTR(pkt->data);

	    // Display the data
            VPrint("---> ");
            for (idx = 0; idx < pkt->ByteCount; idx++)
            {
                VPrint("%02x ", payload[idx]);
                if ((idx % 16) == 15)
                {
                    VPrint("\n---> ");
                }
            }

            if ((idx % 16) != 0)
            {
                VPrint("\n");
            }
        }

	// Once packet is finished with, the allocated space *must* be freed.
        // All input packets have their own memory space to avoid overwrites
	// which shared buffers.
        DISCARD_PACKET(pkt);
    }
}

//-------------------------------------------------------------
// VUserMain0()
//
// Test program to generate all sorts of TL, DL and PL
// patterns. Where appropriate, node 1 pcieVHost will
// generate an auto-response. It is not meant to be
// a valid sequence of transactions.
//
//-------------------------------------------------------------

void VUserMain0() 
{
    int idx;
    PktData_t buff[4096];
    PktData_t *pkt_p, *data_p;
    sPkt_t packet;
    int seq = 0, rid = 1, tag = 0;
    int tmp, i;

    uint64 addr;

    InitialisePcie(VUserInput_0, NULL, node);
    VWrite(LINK_STATE, 0, 0, node);
    ConfigurePcie(CONFIG_ENABLE_SKIPS, 20000, node);

    VPrint("VUserMain: in node %d\n", node);

    VRegInterrupt(RST_DEASSERT_INT, ResetDeasserted, node);

    do
    {
        SendOs(SKP, node);
    }
    while (!Interrupt);

    Interrupt &= ~RST_DEASSERT_INT;

    // Send at least one SKIP ordered set after reset deasserted
    // to ensure scrambler is synchronised
    SendOs(SKP, node);
    SendOs(IDL, node);
    SendOs(FTS, node);

    for (i = 0; i < 10; i++)
    {
        // Send out flow controls types that won't be auto-generated in this test,
        // one at a time
        switch(i)
        {
        case 0: SendFC(DL_INITFC1_P,    0,  0,  0, SEND, node); break;
        case 1: SendFC(DL_INITFC1_NP,   0, 16, 16, SEND, node); break;
        case 2: SendFC(DL_INITFC1_CPL,  0,  1,  1, SEND, node); break;
        case 3: SendFC(DL_INITFC2_P,    0,  0,  0, SEND, node); break;
        case 4: SendFC(DL_INITFC2_NP,   0, 16, 16, SEND, node); break;
        case 5: SendFC(DL_INITFC2_CPL,  0,  1,  1, SEND, node); break;
        case 6: SendFC(DL_UPDATEFC_CPL, 0,  1,  1, SEND, node); break;
        }
    
        // These are *expected* to generate warnings by node 1 pcieVHost, but
        // will be displayed by PcieDispLink
        SendPM(DL_PM_ENTER_L1,  SEND, node);
        SendPM(DL_PM_ENTER_L23, SEND, node);
        SendPM(DL_PM_REQ_L0S,   SEND, node);
        SendPM(DL_PM_REQ_L1,    SEND, node);
        SendPM(DL_PM_REQ_ACK,   SEND, node);
        SendVendor(SEND, node);
    
        //---------------------------------------------
    
        buff[0] = 0x76;
        buff[1] = 0xa5;
        buff[2] = 0x70;
        MemWrite (0x12345679, buff, 3, 0, rid, SEND, node);
    
        VPrint("VUserMain: sent mem write (next seq = %d) from node %d\n", seq, node);
    
        MemRead (0x12345679, 1, tag++, rid, SEND, node);
    
        VPrint("VUserMain: sent mem read (next seq = %d) from node %d\n", seq, node);
    
        //---------------------------------------------
    
        for (idx = 0; idx < 256; idx++)
        {            
            buff[idx] = rand() & 0xff;
        }
        MemWrite (0xa0000001ULL, buff, 256, 0, rid, SEND, node);
    
        buff[0] = 0x55;
        buff[1] = 0xaa;
        buff[2] = 0xf0;
        buff[3] = 0x00;
        CfgWrite (0x30, buff, 4, tag++, rid, SEND, node);
    
        CfgRead (0x31, 1, tag++, rid, SEND, node);
    
        buff[0] = 0x69;
        IoWrite (0x92658659, buff, 1, tag++, rid, SEND, node);
        IoRead  (0x92658659, 2, tag++, rid, SEND, node);

        Message (MSG_ASSERT_INTA,   NULL, 0, 0, rid, SEND, node);
        Message (MSG_ASSERT_INTB,   NULL, 0, 0, rid, SEND, node);
        Message (MSG_ASSERT_INTC,   NULL, 0, 0, rid, SEND, node);
        Message (MSG_ASSERT_INTD,   NULL, 0, 0, rid, SEND, node);
        Message (MSG_DEASSERT_INTA, NULL, 0, 0, rid, SEND, node);
        Message (MSG_DEASSERT_INTB, NULL, 0, 0, rid, SEND, node);
        Message (MSG_DEASSERT_INTC, NULL, 0, 0, rid, SEND, node);
        Message (MSG_DEASSERT_INTD, NULL, 0, 0, rid, SEND, node);
        Message (MSG_PM_ACTIVE_NAK, NULL, 0, 0, rid, SEND, node);
        Message (MSG_PM_PME,        NULL, 0, 0, rid, SEND, node);
        Message (MSG_PME_OFF,       NULL, 0, 0, rid, SEND, node);
        Message (MSG_PME_TO_ACK,    NULL, 0, 0, rid, SEND, node);
        Message (MSG_ERR_COR,       NULL, 0, 0, rid, SEND, node);
        Message (MSG_ERR_NONFATAL,  NULL, 0, 0, rid, SEND, node);
        Message (MSG_ERR_FATAL,     NULL, 0, 0, rid, SEND, node);
        Message (MSG_UNLOCK,        NULL, 0, 0, rid, SEND, node);
    
        buff[0] = 0x71;
        buff[1] = 0x07;
        buff[2] = 0x73;
        buff[3] = 0x45;
        Message (MSG_SET_PWR_LIMIT, buff, 4, tag++, rid, SEND, node);
    
    
        SendTs(TS1_ID, PAD, PAD, 255, 0, node, false);
        SendTs(TS2_ID, 0, 0, 255, 0xf, node, true); 
    }

    SendIdle(100, node);
    
    VWrite(PVH_FINISH, 0, 0, node);
    
}
    
