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

#include "ltssm.h"
#include "pcieModelClass.h"

//-------------------------------------------------------------
// DEFINES and MACROS
//-------------------------------------------------------------

#define RST_DEASSERT_INT 4

// Macro to idle until ip_byte_count is non-zero and then clear it.
#define WaitForInput()  {while (!ip_byte_count) pcie->sendIdle(1); ip_byte_count = 0;}

// Macro to put a 32-bit word in an PktData_t output buffer
#define WordToBuf(_word, _byteoff) {buff[_byteoff + 0] =  (_word        & 0xff); \
                                    buff[_byteoff + 1] = ((_word >>  8) & 0xff); \
                                    buff[_byteoff + 2] = ((_word >> 16) & 0xff); \
                                    buff[_byteoff + 3] = ((_word >> 24) & 0xff);}

// Increment tag modulo 32
#define GetNextTag(_tag){_tag = (_tag + 1)%32;}

//-------------------------------------------------------------
// STATICS
//-------------------------------------------------------------

static unsigned int Interrupt = 0;

// Input buffer and counter. Updated from input callback.
// Main program can wait for ip_byte_count to be non-zero,
// (sending idles), use the buffer data and then clear the
// count.
static uint8_t      ipbuf[4096];
static int          ip_byte_count = 0;

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
                ipbuf[idx] = payload[idx];
            }
            ip_byte_count = pkt->ByteCount;

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
// Test program to read the endpoint configuration space and
// program the valid BARs.
//
// Where appropriate, node 1 pcieVHost will generate an
// auto-response.
//
//-------------------------------------------------------------

extern "C" void VUserMain0(int node)
{
    int idx;
    PktData_t buff[4096];
    char      sbuf[128];
    int       rid = node+1;
    int       tag = 0;
    uint32_t  baraddr[6];
    int       errors = 0;
    uint64_t  bar64;
   

    // Create an API object for this node
    pcieModelClass* pcie = new pcieModelClass(node);

    // Initialise PCIe VHost, with input callback function and no user pointer.
    pcie->initialisePcie(VUserInput_0, NULL);

    pcie->getPcieVersionStr(sbuf, 128);
    VPrint("  %s\n", sbuf);

    // Make sure the link is out of electrical idle
    VWrite(LINK_STATE, 0, 0, node);

    pcie->configurePcie(CONFIG_ENABLE_SKIPS, 20000);
    
    // Configure for a pipe
    pcie->configurePcie(CONFIG_DISABLE_SCRAMBLING);
    pcie->configurePcie(CONFIG_DISABLE_8B10B);
    
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
    InitLink(16, node);

    // Initialise flow control
    pcie->initFc();

    // ---------------------------------------------------------
    // Inspect Common space and configure BARs
    // ---------------------------------------------------------

    // Read vendor ID
    pcie->cfgRead (CFG_VENDOR_ID_OFFSET, 2, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
    VPrint("===> Vendor ID read back 0x%04x\n", ((uint16_t*)ipbuf)[0]);

    // Read device ID
    pcie->cfgRead (CFG_DEVICE_ID_OFFSET, 2, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
    VPrint("===> Device ID read back 0x%04x\n", ((uint16_t*)ipbuf)[1]);

    // Read revision ID and class code
    pcie->cfgRead (CFG_REVISION_ID_OFFSET, 1, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
    VPrint("===> Revision ID read back 0x%02x\n", ((uint8_t*)ipbuf)[0]);
    VPrint("===> Class code read back class=0x%02x subclass=0x%02x progif=0x%02x\n",
             ((uint8_t*)ipbuf)[3], ((uint8_t*)ipbuf)[2], ((uint8_t*)ipbuf)[1]);

    // Read header type
    pcie->cfgRead (CFG_HEADER_TYPE_OFFSET, 1, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
    VPrint("===> Revision ID read back 0x%02x\n", ((uint8_t*)ipbuf)[2]);

    // Read status
    pcie->cfgRead (CFG_STATUS_OFFSET, 2, tag, rid, SEND);  GetNextTag(tag);
    WaitForInput();
    VPrint("===> Status read back 0x%04x\n", ((uint16_t*)ipbuf)[1]);

    // Read header type
    pcie->cfgRead (CFG_HEADER_TYPE_OFFSET, 2, tag, rid, SEND);  GetNextTag(tag);
    WaitForInput();
    VPrint("===> Header type read back 0x%02x\n", ((uint8_t*)ipbuf)[2]);

    // Inspect BARs
    WordToBuf(0xffffffff, 0);
    for (int bidx = 0; bidx < 6; bidx++)
    {
        VPrint("===> BAR%d writing 0x%08x\n",bidx,  0xffffffff);
        pcie->cfgWrite (CFG_BAR_HDR_OFFSET + bidx*4, buff, 4, tag, rid, SEND);  GetNextTag(tag);
        pcie->cfgRead  (CFG_BAR_HDR_OFFSET + bidx*4, 4, tag, rid, SEND);  GetNextTag(tag);

        WaitForInput();
        baraddr[bidx] = ((uint32_t*)ipbuf)[0];
        
        // If a 64-bit address write.  and read back next BAR
        if (((baraddr[bidx] & CFG_BAR_LOCATABLE_MASK) >> CFG_BAR_LOCATABLE_BIT_POS) == CFG_BAR_LOCATABLE_64_BIT)
        {
            bidx++;
            VPrint("===> BAR%d writing 0x%08x\n",bidx,  0xffffffff);
            pcie->cfgWrite (CFG_BAR_HDR_OFFSET + bidx*4, buff, 4, tag, rid, SEND);  GetNextTag(tag);
            pcie->cfgRead  (CFG_BAR_HDR_OFFSET + bidx*4, 4, tag, rid, SEND);  GetNextTag(tag);
            
            WaitForInput();
            baraddr[bidx] = ((uint32_t*)ipbuf)[0];
            
            bar64 = (uint64_t)baraddr[bidx-1] | ((uint64_t)baraddr[bidx] << 32);
            VPrint("===> BAR%d read back 0x%016lx = %llu Mbytes\n", bidx-1, bar64, (~(bar64 & ~0xfULL) + 1) >> 20);
        }
        else
        {
            VPrint("===> BAR%d read back 0x%08x = %d bytes\n", bidx, baraddr[bidx], ~(baraddr[bidx] & ~0xfU) + 1);
        }

    }

    // Set address for BAR0 to 0x2A784000
    baraddr[0] = 0x2a784000;
    WordToBuf(baraddr[0], 0);
    VPrint("===> BAR0 writing 0x%08x\n",  baraddr[0]);
    pcie->cfgWrite (CFG_BAR_HDR_OFFSET, buff, 4, tag, rid, SEND); GetNextTag(tag);

    // Set address for BAR1 to 0X7E09AC00
    baraddr[1] = 0x7e09ac00;
    WordToBuf(baraddr[1], 0);
    VPrint("===> BAR1 writing 0x%08x\n",  baraddr[1]);
    pcie->cfgWrite (CFG_BAR_HDR_OFFSET+4, buff, 4, tag, rid, SEND); GetNextTag(tag);
    
    // Set address for BAR2/3 to 0X7FFF8A600000000
    bar64 = 0x7fff8a600000000ULL;
    baraddr[2] = (uint32_t)(bar64 & 0xffffffff);
    baraddr[3] = (uint32_t)((bar64 >> 32) & 0xffffffff);
    
    WordToBuf(baraddr[2], 0);
    VPrint("===> BAR2 writing 0x%08x\n",  baraddr[2]);
    pcie->cfgWrite (CFG_BAR_HDR_OFFSET+8, buff, 4, tag, rid, SEND); GetNextTag(tag);
    WordToBuf(baraddr[3], 0);
    VPrint("===> BAR3 writing 0x%08x\n",  baraddr[3]);
    pcie->cfgWrite (CFG_BAR_HDR_OFFSET+12, buff, 4, tag, rid, SEND); GetNextTag(tag);

    // Check BAR0
    pcie->cfgRead  (CFG_BAR_HDR_OFFSET, 4, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();

    if ((((uint32_t*)ipbuf)[0] & ~0xfU) == baraddr[0])
    {
        VPrint("===> BAR0 read back 0x%08x\n", ((uint32_t*)ipbuf)[0]);
    }
    else
    {
        VPrint("**** ERROR: unexpected BAR0 value. Got 0x%08x, expected 0x%08x\n", (((uint32_t*)ipbuf)[0] & ~0xfU), baraddr[0]);
        errors++;
    }

    // Check BAR1
    pcie->cfgRead  (CFG_BAR_HDR_OFFSET+4, 4, tag, rid, SEND); GetNextTag(tag);

    WaitForInput();

    if ((((uint32_t*)ipbuf)[0] & ~0xfU) == baraddr[1])
    {
        VPrint("===> BAR1 read back 0x%08x\n", ((uint32_t*)ipbuf)[0]);
    }
    else
    {
        VPrint("**** ERROR: unexpected BAR1 value. Got 0x%08x, expected 0x%08x\n", (((uint32_t*)ipbuf)[0] & ~0xfU), baraddr[1]);
        errors++;
    }
    
    // Check BAR2/3
    
    pcie->cfgRead  (CFG_BAR_HDR_OFFSET+8, 4, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
 
    if ((((uint32_t*)ipbuf)[0] & ~0xfU) == baraddr[2])
    {
        VPrint("===> BAR2 read back 0x%08x\n", ((uint32_t*)ipbuf)[0]);
    }
    else
    {
        VPrint("**** ERROR: unexpected BAR2 value. Got 0x%08x, expected 0x%08x\n", (((uint32_t*)ipbuf)[0] & ~0xfU), baraddr[2]);
        errors++;
    }
    
    pcie->cfgRead  (CFG_BAR_HDR_OFFSET+12, 4, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
 
    if (((uint32_t*)ipbuf)[0] == baraddr[3])
    {
        VPrint("===> BAR3 read back 0x%08x\n", ((uint32_t*)ipbuf)[0]);
    }
    else
    {
        VPrint("**** ERROR: unexpected BAR3 value. Got 0x%08x, expected 0x%08x\n", ((uint32_t*)ipbuf)[0], baraddr[1]);
        errors++;
    }

    // ---------------------------------------------------------
    // Walk through the capabilities
    // ---------------------------------------------------------

    // Initial next capability pointer at TYPE0 config ptr offset
    pcie->cfgRead (CFG_CAPABILITIES_PTR_OFFSET, 4, tag, rid, SEND); GetNextTag(tag);
    WaitForInput();
    uint32_t next_cap_ptr = ((uint8_t*)ipbuf)[0];

    do
    {
        // If not at the end of the list, process next capability
        if (next_cap_ptr)
        {
            uint32_t this_cap = next_cap_ptr;

            // Read capability's ID
            pcie->cfgRead (this_cap + 0, 4, tag, rid, SEND); GetNextTag(tag);
            WaitForInput();
            uint32_t capId = ((uint8_t*)ipbuf)[0];
            next_cap_ptr   = ((uint8_t*)ipbuf)[1];

            switch(capId)
            {
            case PWRMGMTCAPTYPE:
                VPrint("===> Found Power Mangement capability\n");
                VPrint("    0x%08x\n", ((uint32_t*)ipbuf)[0]);

                for (int idx = 4; idx < CFG_PWR_MGMT_CAPS_SIZE_BYTES; idx += 4)
                {
                    pcie->cfgRead (this_cap + idx, 4, tag, rid, SEND); GetNextTag(tag);
                    WaitForInput();
                    VPrint("    0x%08x\n", ((uint32_t*)ipbuf)[0]);
                }
                break;
            case MSICAPTYPE:
                VPrint("===> Found MSI capability\n");
                VPrint("    0x%08x\n", ((uint32_t*)ipbuf)[0]);

                for (int idx = 4; idx < CFG_MSI_CAPS_SIZE_BYTES; idx += 4)
                {
                    pcie->cfgRead (this_cap + idx, 4, tag, rid, SEND); GetNextTag(tag);
                    WaitForInput();
                    VPrint("    0x%08x\n", ((uint32_t*)ipbuf)[0]);
                }
                break;
            case PCIECAPTYPE:
                VPrint("===> Found PCIe capability\n");
                VPrint("    0x%08x\n", ((uint32_t*)ipbuf)[0]);

                for (int idx = 4; idx < CFG_PCIE_CAPS_SIZE_BYTES; idx += 4)
                {
                    pcie->cfgRead (this_cap + idx, 4, tag, rid, SEND); GetNextTag(tag);
                    WaitForInput();
                    VPrint("    0x%08x\n", ((uint32_t*)ipbuf)[0]);
                }
                break;
            default:
                VPrint("****ERROR: Unknown/unsupported capability\n");
                errors++;
                break;
            }
        }
    }
    while (next_cap_ptr);

    // Print results
    if (errors)
    {
        VPrint("\n****ERROR: Finished with %d errors\n\n", errors);
    }
    else
    {
        VPrint("\n===> Finished with no errors\n\n");
    }

    // Go quiet for a while, before finishing
    pcie->sendIdle(100);

    // Halt the simulation
    VWrite(PVH_FINISH, 0, 0, node);

}

