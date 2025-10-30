//=============================================================
//
// Copyright (c) 2024 Simon Southwell. All rights reserved.
//
// Date: 18th Dec 2024
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
// VUserMain0.c
//=============================================================

#include <stdio.h>
#include <stdlib.h>

#include "pcieModelClass.h"

#define RST_DEASSERT_INT 4

static unsigned int Interrupt = 0;

//-------------------------------------------------------------
// ResetDeasserted()
//
// ISR for reset de-assertion. Clears interrupts state.
//
//-------------------------------------------------------------

static int ResetDeasserted(int irq)
{
    Interrupt |= irq & RST_DEASSERT_INT;

    return 0;
}

//-------------------------------------------------------------
// VUserInput_0()
//
// Consumes the unhandled input Packets
//-------------------------------------------------------------

static void VUserInput_0(pPkt_t pkt, int status, void* usrptr)
{
    int idx;

    if (pkt->seq == DLLP_SEQ_ID)
    {
        DebugVPrint("---> VUserInput_0 received DLLP\n");
        free(pkt->data);
        free(pkt);
    }
    else
    {
        DebugVPrint("---> VUserInput_0 received TLP sequence %d of %d bytes at %d\n", pkt->seq, GET_TLP_LENGTH(pkt->data), pkt->TimeStamp);

        // If a completion, extract the TPL payload data and display
        if (pkt->ByteCount)
        {
            // Get a pointer to the start of the payload data
            pPktData_t payload = GET_TLP_PAYLOAD_PTR(pkt->data);

            // Display the data
            DebugVPrint("---> ");
            for (idx = 0; idx < pkt->ByteCount; idx++)
            {
                DebugVPrint("%02x ", payload[idx]);
                if ((idx % 16) == 15)
                {
                    DebugVPrint("\n---> ");
                }
            }

            if ((idx % 16) != 0)
            {
                DebugVPrint("\n");
            }
        }

        // Once packet is finished with, the allocated space *must* be freed.
        // All input packets have their own memory space to avoid overwrites
        // with shared buffers.
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

extern "C" void VUserMain0(int node)
{
    int idx;
    PktData_t buff[4096];
    char      sbuf[128];
    int       rid = node+1, tag = 0;
    int       i;
    uint64_t  addr;
    bool gen_ecrc = DIGEST;

    // Create an API object for this node
    pcieModelClass* pcie = new pcieModelClass(node);

    // Initialise PCIe VHost, with input callback function and no user pointer.
    pcie->initialisePcie(VUserInput_0, NULL);

    pcie->getPcieVersionStr(sbuf, 128);
    VPrint("  %s\n", sbuf);

    // Make sure the link is out of electrical idle
    VWrite(LINK_STATE, 0, 0, node);

    pcie->configurePcie(CONFIG_ENABLE_SKIPS, 20000);

    DebugVPrint("VUserMain: in node %d\n", node);

    VRegIrq(ResetDeasserted, node);

    // Use node number as seed
    pcie->pcieSeed(node);

    // Send out idles until we recieve an interrupt
    do
    {
        pcie->sendOs(IDL);
    }
    while (!Interrupt);

    Interrupt &= ~RST_DEASSERT_INT;

    // Initialise the link for 16 lanes
    pcie->initLink(16);

    // Initialise flow control
    pcie->initFc();

    // Initialise BAR0
    buff[0] = 0x78;
    buff[1] = 0x56;
    buff[2] = 0x34;
    buff[3] = 0x12;
    pcie->cfgWrite (CFG_BAR_HDR_OFFSET, buff, 4, tag++, rid, SEND, gen_ecrc);

    // Initialise BAR1
    buff[0] = 0x00;
    buff[1] = 0x00;
    buff[2] = 0x00;
    buff[3] = 0xa0;
    pcie->cfgWrite (CFG_BAR_HDR_OFFSET + 4, buff, 4, tag++, rid, SEND, gen_ecrc);

    // Send out various example transactions for a bit
    for (i = 0; i < 10; i++)
    {
        // These are *expected* to generate warnings by node 1 pcieVHost, but
        // will be displayed by PcieDispLink
        pcie->sendPM(DL_PM_ENTER_L1,  SEND);
        pcie->sendPM(DL_PM_ENTER_L23, SEND);
        pcie->sendPM(DL_PM_REQ_L0S,   SEND);
        pcie->sendPM(DL_PM_REQ_L1,    SEND);
        pcie->sendPM(DL_PM_REQ_ACK,   SEND);
        pcie->sendVendor(SEND);

        //---------------------------------------------

        buff[0] = 0x76;
        buff[1] = 0xa5;
        buff[2] = 0x70;
        pcie->memWrite (0x12345679, buff, 3, 0, rid, SEND, gen_ecrc);

        DebugVPrint("VUserMain0: sent mem write from node %d\n", node);

        pcie->memRead (0x12345679, 1, tag++, rid, SEND, gen_ecrc);

        DebugVPrint("VUserMain0: sent mem read from node %d\n", node);

        //---------------------------------------------

        for (idx = 0; idx < 256; idx++)
        {
            buff[idx] = rand() & 0xff;
        }
        pcie->memWrite (0xa0000001ULL, buff, 256, 0, rid, SEND, gen_ecrc);

        pcie->memRead  (0xa0000083ULL, 128, tag++, rid, SEND, gen_ecrc);

        //---------------------------------------------

        buff[0] = 0x00;
        buff[1] = 0xf0;
        buff[2] = 0xaa;
        buff[3] = 0x55;
        pcie->cfgWrite (0x30, buff, 4, tag++, rid, SEND, gen_ecrc);

        pcie->cfgRead (0x31, 1, tag++, rid, SEND, gen_ecrc);

        //---------------------------------------------

        buff[0] = 0x69;
        pcie->ioWrite (0x92658659, buff, 1, tag++, rid, SEND, gen_ecrc);
        pcie->ioRead  (0x92658659, 2, tag++, rid, SEND, gen_ecrc);

        //---------------------------------------------

        pcie->message (MSG_ASSERT_INTA,   NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_ASSERT_INTB,   NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_ASSERT_INTC,   NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_ASSERT_INTD,   NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_DEASSERT_INTA, NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_DEASSERT_INTB, NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_DEASSERT_INTC, NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_DEASSERT_INTD, NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_PM_ACTIVE_NAK, NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_PM_PME,        NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_PME_OFF,       NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_PME_TO_ACK,    NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_ERR_COR,       NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_ERR_NONFATAL,  NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_ERR_FATAL,     NULL, 0, 0, rid, SEND, gen_ecrc);
        pcie->message (MSG_UNLOCK,        NULL, 0, 0, rid, SEND, gen_ecrc);

        buff[0] = 0x71;
        buff[1] = 0x07;
        buff[2] = 0x73;
        buff[3] = 0x45;
        pcie->message (MSG_SET_PWR_LIMIT, buff, 4, tag++, rid, SEND, gen_ecrc);
        
        
        pcie->memRead (0x12345679, 1, tag++, rid, SEND, gen_ecrc, true);
        
    }

    // Go quiet for a while, before finishing
    pcie->sendIdle(100);

    // Halt the simulation
    VWrite(PVH_FINISH, 0, 0, node);

}

