//=============================================================
//
// Copyright (c) 2016-2025 Simon Southwell. All rights reserved.
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
//
// Support functions for PCIe Host model
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include "pcie.h"
#include "pcie_utils.h"
#include "displink.h"
#include "codec.h"

// -------------------------------------------------------------------------
// CalcNewRxDataCredits()
//
// Calculate new absolute received credit count.
//
// -------------------------------------------------------------------------

static uint32_t CalcNewRxCredits(const uint32_t rxfc, const uint32_t currfc, const uint32_t maxcredits, const int node, const int hdrcheck)
{
    uint32_t newfc = currfc;

    // If the bottom bits of current credits are greater than received credits
    // then we've wrapped and must add a whole (maxcredits+1) block, to
    // keep an absolute count.
    if ((newfc & maxcredits) > (rxfc & maxcredits))
    {
        newfc += maxcredits + 1;
    }

    newfc = (newfc & ~maxcredits) | (rxfc & maxcredits);

    if (hdrcheck && newfc >= currfc + 127) /* Checks for 2.6.1 PCIE 1.1 pg 110 */
    {
        VPrint("CalcNewRxCredits(): %s***Error --- Received Hdr FC advertising more than 127 credits NewFC %d OldFC %d\%sn", fmterrstr, newfc, currfc, fmtnormstr);
        VWrite(PVH_FATAL, 0, 0, node);
    }
    else if (newfc >= currfc + 2047)
    {
        VPrint("CalcNewRxCredits(): %s***Error --- Received Data FC advertising more than 2047 credits NewFC %d OldFC %d%s\n", fmterrstr, newfc, currfc, fmtnormstr);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    return newfc;
}

// -------------------------------------------------------------------------
// ProcessRxFlowControl()
//
// Counts received packet credits, and sends out FC
// updates when required.
//
// -------------------------------------------------------------------------

static void ProcessRxFlowControl(const pFlowControl_t const flw, const int intype, const int payload_length, const int node)
{
    int type = intype;

    if (((type & DL_ROUTE_MASK) == TL_MSG) || ((type & DL_ROUTE_MASK) == TL_MSGD))
    {
       type &= DL_ROUTE_MASK;
    }

    switch (type)
    {
    case TL_MWR32:
    case TL_MWR64:
    case TL_MSGD:
        if (flw->ConsumedHdrCredits[0][FC_POST])
        {
            flw->RxHdrCredits[0][FC_POST] += 1;
            if (flw->RxHdrCredits[0][FC_POST] > flw->ConsumedHdrCredits[0][FC_POST])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Posted header Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        if (flw->ConsumedDataCredits[0][FC_POST])
        {
            flw->RxDataCredits[0][FC_POST] += payload_length/4 + ((payload_length%4) ? 1 : 0);
            if (flw->RxDataCredits[0][FC_POST] > flw->ConsumedDataCredits[0][FC_POST])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Posted Data Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }

        break;
    case TL_MSG:
        if (flw->ConsumedHdrCredits[0][FC_POST])
        {
            flw->RxHdrCredits[0][FC_POST]  += 1;

            if (flw->RxHdrCredits[0][FC_POST] > flw->ConsumedHdrCredits[0][FC_POST])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Posted header Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        break;
    case TL_CPL:
    case TL_CPLLK:
        if (flw->ConsumedHdrCredits[0][FC_CMPL])
        {
            flw->RxHdrCredits[0][FC_CMPL]  += 1;
            if (flw->RxHdrCredits[0][FC_CMPL] > flw->ConsumedHdrCredits[0][FC_CMPL])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Completion header Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        break;
    case TL_CPLD:
    case TL_CPLDLK:
        if (flw->ConsumedHdrCredits[0][FC_CMPL])
        {
            flw->RxHdrCredits[0][FC_CMPL]  += 1;
            if (flw->RxHdrCredits[0][FC_CMPL] > flw->ConsumedHdrCredits[0][FC_CMPL])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Completion header Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        if (flw->ConsumedDataCredits[0][FC_CMPL])
        {
            flw->RxDataCredits[0][FC_CMPL] += payload_length/4 + ((payload_length%4) ? 1 : 0);
            if (flw->RxDataCredits[0][FC_CMPL] > flw->ConsumedDataCredits[0][FC_CMPL])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Completion data Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        break;
    case TL_MRD32:
    case TL_MRD64:
    case TL_MRDLCK32:
    case TL_MRDLCK64:
    case TL_IORD:
    case TL_CFGRD0:
    case TL_CFGRD1:
        if (flw->ConsumedHdrCredits[0][FC_NONPOST])
        {
            flw->RxHdrCredits[0][FC_NONPOST]  += 1;
            if (flw->RxHdrCredits[0][FC_NONPOST] > flw->ConsumedHdrCredits[0][FC_NONPOST])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Non-posted header Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        DebugVPrint ("flw->ConsumedHdrCredits[0][FC_NONPOST] %d, flw->RxHdrCredits[0][FC_NONPOST] %d\n", flw->ConsumedHdrCredits[0][FC_NONPOST], flw->RxHdrCredits[0][FC_NONPOST]);
        break;
    default:
        if (flw->ConsumedHdrCredits[0][FC_NONPOST])
        {
            flw->RxHdrCredits[0][FC_NONPOST] += 1;
            if (flw->RxHdrCredits[0][FC_NONPOST] > flw->ConsumedHdrCredits[0][FC_NONPOST])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Non-posted header Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        if (flw->ConsumedDataCredits[0][FC_NONPOST])
        {
            flw->RxDataCredits[0][FC_NONPOST] += payload_length/4 + ((payload_length%4) ? 1 : 0);
            if (flw->RxDataCredits[0][FC_NONPOST] > flw->ConsumedDataCredits[0][FC_NONPOST])
            {
                VPrint("ProcessRxFlowControl(): %s***Error --- Overflow on Non-posted data Credits%s", fmterrstr, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, node);
            }
        }
        break;
    }
}

// -------------------------------------------------------------------------
// UpdateConsumedFC()
//
// Called every cycle to increment the flow control counts
// at a steady 'rate'.
//
// -------------------------------------------------------------------------

static void UpdateConsumedFC(const pPcieModelState_t const state)
{
    uint32_t current_cycle, i;
    bool no_change = true;
    pFlowControl_t flw = &(state->flwcntl);
    int fc_timeout[FC_NUMTYPES];

    if (state->usrconf.DisableFc)
    {
        return;
    }

    current_cycle = GetCycleCount(state->thisnode);

    DebugVPrint ("CONSUMED --- NPOSTED: hdr %x data %x, POSTED: hdr %x data %x, CPL: hdr %x data %x\n",
                 flw->ConsumedHdrCredits[0][FC_NONPOST], flw->ConsumedDataCredits[0][FC_NONPOST], flw->ConsumedHdrCredits[0][FC_POST], flw->ConsumedDataCredits[0][FC_POST], flw->ConsumedHdrCredits[0][FC_CMPL], flw->ConsumedDataCredits[0][FC_CMPL]);
    DebugVPrint ("ADVERT   --- NPOSTED: hdr %x data %x, POSTED: hdr %x data %x, CPL: hdr %x data %x\n",
                 flw->AdvertisedHdrCredits[0][FC_NONPOST], flw->AdvertisedDataCredits[0][FC_NONPOST], flw->AdvertisedHdrCredits[0][FC_POST], flw->AdvertisedDataCredits[0][FC_POST], flw->AdvertisedHdrCredits[0][FC_CMPL], flw->AdvertisedDataCredits[0][FC_CMPL]);
    DebugVPrint ("CREDITS  --- NPOSTED: hdr %x data %x, POSTED: hdr %x data %x, CPL: hdr %x data %x\n",
                 flw->RxHdrCredits[0][FC_NONPOST], flw->RxDataCredits[0][FC_NONPOST], flw->RxHdrCredits[0][FC_POST], flw->RxDataCredits[0][FC_POST], flw->RxHdrCredits[0][FC_CMPL], flw->RxDataCredits[0][FC_CMPL]);


    // Consume headers at defined rate until none left
    if (current_cycle%(state->usrconf.HdrConsumptionRate) == 0)
    {
        for (i = 0; i < FC_NUMTYPES; i++)
        {
            if (flw->ConsumedHdrCredits[0][i] && ((flw->ConsumedHdrCredits[0][i] - flw->RxHdrCredits[0][i]) < state->usrconf.InitFcHdrCr[0][i]))
            {
                flw->ConsumedHdrCredits[0][i] += 1;
                flw->ConsumedHdrUpdated[0][i] = 1;
                no_change = false;
            }
        }
    }

    // Consume received data at defined rate until none left
    if (current_cycle%(state->usrconf.DataConsumptionRate) == 0)
    {
        for (i = 0; i < FC_NUMTYPES; i++)
        {
            if (flw->ConsumedDataCredits[0][i] && ((flw->ConsumedDataCredits[0][i] - flw->RxDataCredits[0][i]) < state->usrconf.InitFcDataCr[0][i]))
            {
                flw->ConsumedDataCredits[0][i] += 1;
                flw->ConsumedDataUpdated[0][i] = 1;
                no_change = false;
            }
        }
    }

    // Flag if a flow control timeout for any of the types
    for (i = 0; i < FC_NUMTYPES; i++)
    {
        // If a timeout, pretend there's been a change to force sending of flow control
        if (fc_timeout[i] = ((GetCycleCount(state->thisnode) - flw->LastSentFcTime[0][i]) > DEFAULT_FC_TIME))
        {
            no_change = false;
        }
    }

    // We need to send an update if not both header and data infinite, if there's been
    // a change in consumed credits and if no header space left or data space
    // less than max payload size (or equal 0 for non-posted).

    // Non-posted
    if (flw->ConsumedHdrCredits[0][FC_NONPOST] || flw->ConsumedDataCredits[0][FC_NONPOST])
    {
        if ((flw->ConsumedHdrUpdated[0][FC_NONPOST]  && ((flw->AdvertisedHdrCredits[0][FC_NONPOST]  - flw->RxHdrCredits[0][FC_NONPOST]) == 0)) ||
            (flw->ConsumedDataUpdated[0][FC_NONPOST] && ((flw->AdvertisedDataCredits[0][FC_NONPOST] - flw->RxDataCredits[0][FC_NONPOST]) == 0)) ||
            ((flw->ConsumedHdrUpdated[0][FC_NONPOST] || flw->ConsumedDataUpdated[0][FC_NONPOST]) && (state->send_p == NULL)) ||
            fc_timeout[FC_NONPOST])
        {
            SendFC (DL_UPDATEFC_NP, 0, flw->ConsumedHdrCredits[0][FC_NONPOST]%(DL_MAX_HDRFC+1), flw->ConsumedDataCredits[0][FC_NONPOST]%(DL_MAX_DATAFC+1), state->draining_queue, state->thisnode);
            flw->AdvertisedHdrCredits[0][FC_NONPOST] = flw->ConsumedHdrCredits[0][FC_NONPOST];
            flw->AdvertisedDataCredits[0][FC_NONPOST] = flw->ConsumedDataCredits[0][FC_NONPOST];
            flw->LastSentFcTime[0][FC_NONPOST] = GetCycleCount(state->thisnode);
            flw->ConsumedHdrUpdated[0][FC_NONPOST]  = 0;
            flw->ConsumedDataUpdated[0][FC_NONPOST] = 0;
        }
    }

    // Completions
    if (flw->ConsumedHdrCredits[0][FC_CMPL] || flw->ConsumedDataCredits[0][FC_CMPL])
    {
        if ((flw->ConsumedHdrUpdated[0][FC_CMPL]  && ((flw->AdvertisedHdrCredits[0][FC_CMPL]  - flw->RxHdrCredits[0][FC_CMPL]) == 0)) ||
            (flw->ConsumedDataUpdated[0][FC_CMPL] && ((flw->AdvertisedDataCredits[0][FC_CMPL] - flw->RxDataCredits[0][FC_CMPL]) < DEFAULT_MAX_PAYLOAD_SIZE)) ||
            ((flw->ConsumedHdrUpdated[0][FC_CMPL] || flw->ConsumedDataUpdated[0][FC_CMPL]) && (state->send_p == NULL)) ||
            fc_timeout[FC_CMPL])
        {
            SendFC (DL_UPDATEFC_CPL, 0, flw->ConsumedHdrCredits[0][FC_CMPL]%(DL_MAX_HDRFC+1), flw->ConsumedDataCredits[0][FC_CMPL]%(DL_MAX_DATAFC+1), state->draining_queue, state->thisnode);
            flw->AdvertisedHdrCredits[0][FC_CMPL] = flw->ConsumedHdrCredits[0][FC_CMPL];
            flw->AdvertisedDataCredits[0][FC_CMPL] = flw->ConsumedDataCredits[0][FC_CMPL];
            flw->LastSentFcTime[0][FC_CMPL] = GetCycleCount(state->thisnode);
            flw->ConsumedHdrUpdated[0][FC_CMPL]  = 0;
            flw->ConsumedDataUpdated[0][FC_CMPL] = 0;
        }
    }

    // Posted
    if (flw->ConsumedHdrCredits[0][FC_POST] || flw->ConsumedDataCredits[0][FC_POST])
    {
        if ((flw->ConsumedHdrUpdated[0][FC_POST]  && ((flw->AdvertisedHdrCredits[0][FC_POST]  - flw->RxHdrCredits[0][FC_POST]) == 0)) ||
            (flw->ConsumedDataUpdated[0][FC_POST] && ((flw->AdvertisedDataCredits[0][FC_POST] - flw->RxDataCredits[0][FC_POST]) < DEFAULT_MAX_PAYLOAD_SIZE)) ||
            ((flw->ConsumedHdrUpdated[0][FC_POST] || flw->ConsumedDataUpdated[0][FC_POST]) && (state->send_p == NULL)) ||
            fc_timeout[FC_POST])
        {

            SendFC (DL_UPDATEFC_P, 0, flw->ConsumedHdrCredits[0][FC_POST]%(DL_MAX_HDRFC+1), flw->ConsumedDataCredits[0][FC_POST]%(DL_MAX_DATAFC+1), state->draining_queue, state->thisnode);
            flw->AdvertisedHdrCredits[0][FC_POST] = flw->ConsumedHdrCredits[0][FC_POST];
            flw->AdvertisedDataCredits[0][FC_POST] = flw->ConsumedDataCredits[0][FC_POST];
            flw->LastSentFcTime[0][FC_POST] = GetCycleCount(state->thisnode);
            flw->ConsumedHdrUpdated[0][FC_POST]  = 0;
            flw->ConsumedDataUpdated[0][FC_POST] = 0;
        }
    }
}

// -------------------------------------------------------------------------
// CheckSkips()
//
// Checks time since last skip OS and flags a requirement
// if necessary. (OS sequences not queued, so cannot simply
// call SendOS(), but must flag to SendPacket() which can
// insert it at an appropriate time.)
//
// -------------------------------------------------------------------------

static void CheckSkips(const pPcieModelState_t const state)
{
    if ((state->TicksSinceReset - state->LastTxSkipTime) > state->usrconf.SkipInterval)
    {
        state->SkipScheduled += 1;
        state->LastTxSkipTime = state->TicksSinceReset;
    }
}

// -------------------------------------------------------------------------
// AckPkt()
//
// Acknowledging packets consists of updating curr_ack to
// acknowledge sequence, only if acknowledge sequence is
// higher *and* there is no higher outstanding NAK.
//
// -------------------------------------------------------------------------

static void AckPkt(const pPcieModelState_t const state, const int sequence)
{
    if (sequence < state->curr_nak && (sequence > state->curr_ack || state->curr_ack == NULLACK))
    {
        state->curr_ack = sequence;
    }
}

// -------------------------------------------------------------------------
// NakPkt()
//
// Update curr_nak with nak sequence, but only if lower
// than outstanding Nak (i.e. we've Nak'd earlier, are not
// yet retrying, and a later packet also gets Nak'd).
//
// -------------------------------------------------------------------------

static void NakPkt(const pPcieModelState_t const state, const int sequence)
{
    if (sequence < state->curr_nak)
    {
        state->curr_nak = sequence;
    }
}

// -------------------------------------------------------------------------
// CheckDelayQueue()
//
// -------------------------------------------------------------------------

static void CheckDelayQueue (const pPcieModelState_t const state)
{
    pPkt_t tmp;

    while (state->cpl_head_p != NULL && state->cpl_head_p->TimeStamp <= state->TicksSinceReset)
    {
        tmp = state->cpl_head_p->NextPkt;
        AddPktToQueue(state, state->cpl_head_p);
        state->cpl_head_p = tmp;
    }
}

// -------------------------------------------------------------------------
// checkBars()
//
// Check address against BARs. Returns true if within a configured
// BARs region. Also returns true if this node's configuration space
// or configuration space mask aren't configured, when all address
// space is made available.
//
// -------------------------------------------------------------------------

static bool checkBars(const uint64_t addr, const uint32_t bytelen, const unsigned node)
{
    uint64_t BAR;
    uint64_t BARMASK;

    bool ok_to_access = false;
    unsigned locatable;

    PktData_t buf[4];

    // Go through all six  BARs
    for (unsigned idx = 0; idx < 6; idx++)
    {
       // Read the first word of the BAR. Returns true if a config space configured.
       if (ReadConfigSpaceBufChk(CFG_BAR_HDR_OFFSET + 4*idx, buf, 4, false, node))
       {
           locatable = (buf[0] >> 1) & 0x3;

           // Set BAR lower address bits and mask lower type/locatable and prefetchable bits
           BAR = ((uint64_t)buf[0] | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[2] << 16) | ((uint64_t)buf[3] << 24)) & 0xfffffffffffffff0ULL;

           if (locatable == CFG_BAR_LOCATABLE_64_BIT)
           {
               // Read the upper BAR bits and OR into BAR value
               ReadConfigSpaceBufChk(CFG_BAR_HDR_OFFSET + 4 + 4*idx, buf, 4, false, node);
               BAR = ((uint64_t)buf[0] <<  32) | ((uint64_t)buf[1] << 40) | ((uint64_t)buf[2] << 48) | ((uint64_t)buf[3] << 56);
           }
       }
       // If no config space configured, always allow access
       else
       {
           ok_to_access = true;
           break;
       }

       // Read the first word of the BAR mask. Returns true if a config space mask configured.
       if (ReadConfigSpaceMaskBufChk(CFG_BAR_HDR_OFFSET + 4*idx, buf, 4, false, node))
       {
           // Set BAR lower mask bits
           BARMASK = (uint64_t)buf[0] | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[2] << 16) | ((uint64_t)buf[3] << 24);

           if (locatable == CFG_BAR_LOCATABLE_64_BIT)
           {
               // Read the upper BAR mask bits and OR into BAR mask value
               ReadConfigSpaceMaskBufChk(CFG_BAR_HDR_OFFSET + 4 + 4*idx, buf, 4, false, node);
               BARMASK = ((uint64_t)buf[0] <<  32) | ((uint64_t)buf[1] << 40) | ((uint64_t)buf[2] << 48) | ((uint64_t)buf[3] << 56);

               // Skip over upper bits of 64-bit BAR
               idx++;
           }
       }
       else
       {
           // If no config space masks configured, always allow access
           ok_to_access = true;
           break;
       }

        // Calculate the length from the mask bits
        uint64_t length = (BARMASK + 1) & ((locatable == CFG_BAR_LOCATABLE_64_BIT) ? 0xffffffffffffffffULL : 0x00000000ffffffffULL);

        // Calculate end address of burst
        uint64_t endaddr = addr + bytelen;

        // Check if all addresses are within the BAR's range
        if (addr >= BAR && addr < (BAR + length) && endaddr >= BAR && endaddr < (BAR + length))
        {
            // OK to access if it is.
            ok_to_access = true;
            break;
        }
    }

    return ok_to_access;
}

// -------------------------------------------------------------------------
// ProcessInput()
//
// Gets called for every DLLP or TLP packet seen at the
// input link. Processes Acks/Naks and flow control DLLPs,
// and manages local memory reads and writes, generating
// completions for reads (by calling Completion() ).
// Returned completions get passed up to user registered
// callback function.
//
// -------------------------------------------------------------------------

static void ProcessInput (const pPcieModelState_t const state, const pPkt_t const pkt, const int Edb)
{
    PktData_t crc[4], ecrc[4];
    uint32_t type, lcrc_offset, ecrc_offset, payload_length;
    uint64_t addr;
    uint32_t length, rid, cid, tag, fbe, lbe, byte_count;
    PktData_t buff[MAX_BYTE_BLOCK], *pdata;
    int status;
    pFlowControl_t flw = &(state->flwcntl);
    uint32_t got_lcrc, exp_lcrc;
    uint32_t got_ecrc, exp_ecrc;

    // Set an optimistic packet status
    status = PKT_STATUS_GOOD;

    // DLLP
    if (pkt->seq == DLLP_SEQ_ID)
    {
        // Check if CRC good
        crc[0] = pkt->data[5];
        crc[1] = pkt->data[6];
        CalcDllpCrc(pkt->data);

        PktData_t gotcrc = (crc[0] << 8)| crc[1];
        PktData_t expcrc = (pkt->data[5] << 8) | pkt->data[6];

        type = pkt->data[1];
        type &= ((type & 0x30) == 0x20) ? 0xff : 0xf8; // Mask VC bits

        DispDll(state, pkt, true);

        // If good CRC ...
        if (expcrc == gotcrc)
        {
            type = pkt->data[1];
            type &= ((type & 0x30) == 0x20) ? 0xff : 0xf8; // Mask VC bits
            switch (type)
            {
            case DL_ACK:
                if (state->usrconf.DisableAck)
                {
                    if (state->vuser_cb != NULL)
                    {
                        (state->vuser_cb)(pkt, status, state->usrptr);
                    }
                }
                else
                {
                    AckPkt(state, ((pkt->data[3] & 0xf)<<8) | (pkt->data[4] &0xff));
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;
            case DL_NAK:
                if (state->usrconf.DisableAck)
                {
                    if (state->vuser_cb != NULL)
                    {
                        (state->vuser_cb)(pkt, status, state->usrptr);
                    }
                }
                else
                {
                    NakPkt(state, ((pkt->data[3] & 0xf)<<8) | (pkt->data[4] &0xff));
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;
            case DL_UPDATEFC_P:
                if (state->usrconf.DisableFc)
                {
                    if (state->vuser_cb != NULL)
                    {
                        (state->vuser_cb)(pkt, status, state->usrptr);
                    }
                }
                else
                {
                    flw->FlowCntlDataCredits[pkt->data[1] & 0x7][FC_POST] = CalcNewRxCredits(GET_DATA_FC(pkt->data), flw->FlowCntlDataCredits[pkt->data[1] & 0x7][FC_POST], DL_MAX_DATAFC, state->thisnode, FC_DATA_CHK);
                    flw->FlowCntlHdrCredits[pkt->data[1] & 0x7][FC_POST]  = CalcNewRxCredits(GET_HDR_FC(pkt->data) , flw->FlowCntlHdrCredits[pkt->data[1] & 0x7][FC_POST] , DL_MAX_HDRFC, state->thisnode, FC_HDR_CHK);
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;
            case DL_UPDATEFC_NP:
                if (state->usrconf.DisableFc)
                {
                    if (state->vuser_cb != NULL)
                    {
                        (state->vuser_cb)(pkt, status, state->usrptr);
                    }
                }
                else
                {
                    flw->FlowCntlDataCredits[pkt->data[1] & 0x7][FC_NONPOST] = CalcNewRxCredits(GET_DATA_FC(pkt->data), flw->FlowCntlDataCredits[pkt->data[1] & 0x7][FC_NONPOST], DL_MAX_DATAFC, state->thisnode, FC_DATA_CHK);
                    flw->FlowCntlHdrCredits[pkt->data[1] & 0x7][FC_NONPOST]  = CalcNewRxCredits(GET_HDR_FC(pkt->data) , flw->FlowCntlHdrCredits[pkt->data[1] & 0x7][FC_NONPOST] , DL_MAX_HDRFC, state->thisnode, FC_HDR_CHK);
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;
            case DL_UPDATEFC_CPL:
                if (state->usrconf.DisableFc)
                {
                    if (state->vuser_cb != NULL)
                    {
                        (state->vuser_cb)(pkt, status, state->usrptr);
                    }
                }
                else
                {
                    flw->FlowCntlDataCredits[pkt->data[1] & 0x7][FC_CMPL] = CalcNewRxCredits(GET_DATA_FC(pkt->data), flw->FlowCntlDataCredits[pkt->data[1] & 0x7][FC_CMPL], DL_MAX_DATAFC, state->thisnode, FC_DATA_CHK);
                    flw->FlowCntlHdrCredits[pkt->data[1] & 0x7][FC_CMPL]  = CalcNewRxCredits(GET_HDR_FC(pkt->data) , flw->FlowCntlHdrCredits[pkt->data[1] & 0x7][FC_CMPL] , DL_MAX_HDRFC, state->thisnode, FC_HDR_CHK);
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;
            case DL_INITFC1_P:
            case DL_INITFC2_P:
            case DL_INITFC1_NP:
            case DL_INITFC2_NP:
            case DL_INITFC1_CPL:
            case DL_INITFC2_CPL:
                if (state->usrconf.DisableFc)
                {
                    if (state->vuser_cb != NULL)
                    {
                        (state->vuser_cb)(pkt, status, state->usrptr);
                    }
                }
                else
                {
                    RxFcInit(flw, type, GET_HDR_FC(pkt->data), GET_DATA_FC(pkt->data));
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;

            default:
                DebugVPrint( "ProcessInput: Warning --- unsupported DLLP received (type = %x) on node %d\n", type, state->thisnode);
                status = PKT_STATUS_UNSUPPORTED;

                // Return unsupported packet to user process, if one registered. Otherwise discard.
                if (state->vuser_cb != NULL)
                {
                    (state->vuser_cb)(pkt, status, state->usrptr);
                }
                else
                {
                    CheckFree(pkt->data);
                    CheckFree(pkt);
                }
                break;
            }
        // Bad CRC
        }
        else
        {
            DebugVPrint( "ProcessInput: Warning --- received bad DLLP on node %d\n", state->thisnode);
            status = PKT_STATUS_BAD_DLLP_CRC;

            // Return bad packet to user process, if one registered. Otherwise discard.
            if (state->vuser_cb != NULL)
            {
                (state->vuser_cb)(pkt, status, state->usrptr);
            }
        }

    // TLP
    }
    else
    {
        type               = pkt->data[TLP_TYPE_BYTE_OFFSET];
        payload_length     = GET_TLP_LENGTH_ADJ(pkt->data);

        bool has_data      = type & TL_TYPE_WRITE;
        bool ecrc_present  = TLP_HAS_DIGEST(pkt->data);
        bool gen_cmpl_ecrc = ecrc_present && !state->usrconf.DisableEcrcCmpl;

        // Check LCRC
        lcrc_offset = 15 + 4 * ((has_data ? GET_TLP_LENGTH(pkt->data) : 0) + (ecrc_present ? 1 : 0) + TLP_HDR_4DW(pkt->data));
        crc[0] = pkt->data[lcrc_offset+0];
        crc[1] = pkt->data[lcrc_offset+1];
        crc[2] = pkt->data[lcrc_offset+2];
        crc[3] = pkt->data[lcrc_offset+3];
        CalcLcrc(pkt->data);

        exp_lcrc = ((uint32_t)pkt->data[lcrc_offset+0] << 24) | ((uint32_t)pkt->data[lcrc_offset+1] << 16) | ((uint32_t)pkt->data[lcrc_offset+2] << 8) | ((uint32_t)pkt->data[lcrc_offset+3]);
        got_lcrc = ((uint32_t)crc[0] << 24) | ((uint32_t)crc[1] << 16) | ((uint32_t)crc[2] << 8) | ((uint32_t)crc[3]);

        if (ecrc_present)
        {
            ecrc_offset = lcrc_offset - 4;
            ecrc[0] = pkt->data[ecrc_offset+0];
            ecrc[1] = pkt->data[ecrc_offset+1];
            ecrc[2] = pkt->data[ecrc_offset+2];
            ecrc[3] = pkt->data[ecrc_offset+3];
            CalcEcrc(pkt->data);
            exp_ecrc = ((uint32_t)pkt->data[ecrc_offset+0] << 24) | ((uint32_t)pkt->data[ecrc_offset+1] << 16) | ((uint32_t)pkt->data[ecrc_offset+2] << 8) | ((uint32_t)pkt->data[ecrc_offset+3]);
            got_ecrc = ((uint32_t)ecrc[0] << 24) | ((uint32_t)ecrc[1] << 16) | ((uint32_t)ecrc[2] << 8) | ((uint32_t)ecrc[3]);
        }

        DispTl(state, pkt, true);

        // Bad CRC
        if (crc[0] != pkt->data[lcrc_offset+0] || crc[1] != pkt->data[lcrc_offset+1] ||
            crc[2] != pkt->data[lcrc_offset+2] || crc[3] != pkt->data[lcrc_offset+3] )
            {

            // Check to see if it isn't a discarded TLP, and NAK if not.
            if (Edb && ((~crc[0] & 0xff) == pkt->data[lcrc_offset+0] &&
                        (~crc[1] & 0xff) == pkt->data[lcrc_offset+1] &&
                        (~crc[2] & 0xff) == pkt->data[lcrc_offset+2] &&
                        (~crc[3] & 0xff) == pkt->data[lcrc_offset+3]))
            {
                status |= PKT_STATUS_NULLIFIED;
            }
            else
            {
                VPrint("ProcessInput: Info --- %sTlp LCRC failure%s. Sending NAK from node %d\n", fmterrstr, fmtnormstr, state->thisnode);
                if (!state->usrconf.DisableAck)
                {
                    SendNak (pkt->seq, state->thisnode);
                }
                status |= PKT_STATUS_BAD_LCRC;
            }

            // Return bad packet data to user process, if registered. Otherwise discard
            if (state->vuser_cb != NULL)
            {
                (state->vuser_cb)(pkt, status, state->usrptr);
            }
            else
            {
                CheckFree(pkt->data);
                CheckFree(pkt);
            }
            return;
        }

        // Check ECRC, if present
        if (ecrc_present && (ecrc[0] != pkt->data[ecrc_offset+0] || ecrc[1] != pkt->data[ecrc_offset+1] ||
                             ecrc[2] != pkt->data[ecrc_offset+2] || ecrc[3] != pkt->data[ecrc_offset+3] ))
        {
            VPrint("ProcessInput: Info --- %sTlp ECRC failure at node %d%s\n", fmterrstr, state->thisnode, fmtnormstr);
            status |= PKT_STATUS_BAD_ECRC;
            if (state->vuser_cb != NULL)
            {
                (state->vuser_cb)(pkt, status, state->usrptr);
            }
            else
            {
                CheckFree(pkt->data);
                CheckFree(pkt);
            }
            return;
        }

        // Good CRC, so send an Ack
        if (!state->usrconf.DisableAck)
        {
            SendAck (pkt->seq, state->thisnode);
        }

        // If mem write ...
        if (!state->usrconf.DisableMem && (type == TL_MWR32 || type == TL_MWR64))
        {
            // Update memory
            if (type == TL_MWR32)
            {
                addr   = ((uint64_t)pkt->data[TLP_ADDR_OFFSET]   << 24) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+1] << 16) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+2] << 8)  | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+3] << 0) ;
            }
            else
            {
                addr   = ((uint64_t)pkt->data[TLP_ADDR_OFFSET]   << 56) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+1] << 48) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+2] << 40) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+3] << 32) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+4] << 24) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+5] << 16) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+6] << 8)  | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+7] << 0) ;
            }

            length     = GET_TLP_LENGTH(pkt->data);

            // Check if address is ok to to use
            if (checkBars(addr, length*4, state->thisnode))
            {
                pdata  = (type == TL_MWR32) ? &(pkt->data[TLP_DATA_OFFSET32]) : &(pkt->data[TLP_DATA_OFFSET64]);
                fbe    = GET_TLP_FBE(pkt->data);
                lbe    = GET_TLP_LBE(pkt->data);
                WriteRamByteBlock(addr, pdata, fbe, lbe, length*4, state->thisnode);
            }

            CheckFree(pkt->data);
            CheckFree(pkt);

        // If mem read ...
        }
        else if (!state->usrconf.DisableMem && (type == TL_MRD32 || type == TL_MRD64))
        {
            // Construct completion and add to queue
            if (type == TL_MRD32)
            {
                addr   = ((uint64_t)pkt->data[TLP_ADDR_OFFSET]   << 24) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+1] << 16) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+2] << 8)  | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+3] << 0) ;
            }
            else
            {
                addr   = ((uint64_t)pkt->data[TLP_ADDR_OFFSET]   << 56) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+1] << 48) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+2] << 40) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+3] << 32) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+4] << 24) | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+5] << 16) |
                         ((uint64_t)pkt->data[TLP_ADDR_OFFSET+6] << 8)  | ((uint64_t)pkt->data[TLP_ADDR_OFFSET+7] << 0) ;
            }

            length     = GET_TLP_LENGTH(pkt->data);
            pdata      = (type == TL_MRD32) ? &(pkt->data[TLP_DATA_OFFSET32]) : &(pkt->data[TLP_DATA_OFFSET64]);
            fbe        = GET_TLP_FBE(pkt->data);
            lbe        = GET_TLP_LBE(pkt->data);
            rid        = GET_TLP_RID(pkt->data);
            cid        = state->CplId;
            tag        = GET_TLP_TAG(pkt->data);
                
            // Check address is good for an access
            if (checkBars(addr, length*4, state->thisnode))
            {
                if (ReadRamByteBlock (addr, buff, length*4, state->thisnode))
                {
                    VPrint("ProcessInput: %s***Error --- ReadRamByteBlock for address %llx returned bad status at node %d%s\n", fmterrstr, (long long unsigned)addr, state->thisnode, fmtnormstr);
                    VWrite(PVH_FATAL, 0, 0, state->thisnode);
                }

                int rlen = (length ? length : MAX_PAYLOAD_BYTES/4);
                PartCompletionDelay(addr, buff, CPL_SUCCESS, fbe, lbe, rlen, rlen, tag, cid, rid, gen_cmpl_ecrc, state->usrconf.CompletionRate, true, state->thisnode);
            }
            else
            {
                PartCompletionDelay(0, NULL, CPL_UNSUPPORTED, 0x0, 0x0, 0, 0, tag, cid, rid, gen_cmpl_ecrc, state->usrconf.CompletionRate, true, state->thisnode);
            }

            CheckFree(pkt->data);
            CheckFree(pkt);

        // If Completion TLP...
        }
        else if (type == TL_CPLD || type == TL_CPL || type == TL_CPLLK || type == TL_CPLDLK)
        {
            byte_count = GET_CPL_BYTECOUNT(pkt->data);
            length     = GET_TLP_LENGTH(pkt->data);

            // Return read data to user process, if registered. Otherwise discard
            if (state->vuser_cb != NULL)
            {
                (state->vuser_cb)(pkt, status, state->usrptr);
            }
            else
            {
                CheckFree(pkt->data);
                CheckFree(pkt);
            }
            // If a last completion increment the completion event counter
            if ((type == TL_CPL) || (type == TL_CPLLK) || (length*4) >= byte_count)
            {
                state->CompletionEvent++;
            }

        // Config reads and writes (for Endpoints only)
        }
        else if (!state->usrconf.DisableMem && state->Endpoint && (type == TL_CFGWR0 || type == TL_CFGRD0))
        {
            addr   = (uint64_t)(pkt->data[TLP_ADDR_OFFSET]   << 24) | (uint64_t)(pkt->data[TLP_ADDR_OFFSET+1] << 16) |
                     (uint64_t)(pkt->data[TLP_ADDR_OFFSET+2] << 8)  | (uint64_t)(pkt->data[TLP_ADDR_OFFSET+3] << 0) ;

            pdata             = &(pkt->data[TLP_DATA_OFFSET32]);
            fbe               = GET_TLP_FBE(pkt->data);
            rid               = GET_TLP_RID(pkt->data);
            cid               = state->CplId;
            tag               = GET_TLP_TAG(pkt->data);

            if (type == TL_CFGRD0)
            {
                // Read a word (4 bytes) from the config space and put in buffer
                ReadConfigSpaceBuf((uint32_t)(addr & 0xfff), buff, 4, state->thisnode);

                PartCompletionDelay(0, buff, CPL_SUCCESS, 0xf, 0x0, 1, 1, tag, cid, rid, gen_cmpl_ecrc, state->usrconf.CompletionRate, true, state->thisnode);
            }
            else
            {
                // The device completer ID is always updated on config writes
                state->CplId = GET_CFG_CID(pkt->data);
                WriteConfigSpaceBuf((uint32_t)(addr & 0xfff), pdata, fbe, 0, 4, true, state->thisnode);

                PartCompletionDelay(0, buff, CPL_SUCCESS, 0xf, 0x0, 0, 0, tag, cid, rid, gen_cmpl_ecrc, state->usrconf.CompletionRate, true, state->thisnode);
            }

            CheckFree(pkt->data);
            CheckFree(pkt);

        // Messages, config accesses (not endpoint), memory accesses (if internal mem disabled),
        // and IO accesses
        }
        else
        {
            DebugVPrint("ProcessInput: Warning --- received unsupported TLP (type 0x%02x, tag %d) at node %d\n", type, GET_TLP_TAG(pkt->data), state->thisnode);

            rid = GET_TLP_RID(pkt->data);
            cid = state->CplId;
            tag = GET_TLP_TAG(pkt->data);

            // Accesses, other than messages and memory write accesses, require a completion of some sort,
            // unless specifically disabled or internal memory disabled where all packets get sent to
            // any registered user callback function
            if (!state->usrconf.DisableUrCpl && !state->usrconf.DisableMem &&
               (type & DL_ROUTE_MASK) != TL_MSG && (type & DL_ROUTE_MASK) != TL_MSGD &&
                type != TL_MWR32 && type != TL_MWR64 && type != TL_MRD32 && type != TL_MRD64)
            {
                PartCompletionDelay(0, NULL, CPL_UNSUPPORTED, 0x0, 0x0, 0, 0, tag, cid, rid, gen_cmpl_ecrc, state->usrconf.CompletionRate, true, state->thisnode);
            }

            // Return unsupported packet to user process, if one registered. Otherwise discard.
            if (state->vuser_cb != NULL)
            {
                (state->vuser_cb)(pkt, status, state->usrptr);
            }
            else
            {
                CheckFree(pkt->data);
                CheckFree(pkt);
            }
        }

        // Update Rx flow control counts for received data
        if (!state->usrconf.DisableFc)
        {
            ProcessRxFlowControl(&(state->flwcntl), type, payload_length, state->thisnode);
        }
    }
}

// -------------------------------------------------------------------------
// ProcessOS()
//
// Maintains state on received orders sets and training
// sequences. As yet, nothing uses this data.
//
// -------------------------------------------------------------------------

static void ProcessOS(const pPcieModelState_t const state,
                      const pLinkEventCount_t const linkevent, const int lane, const int type, const pTS_t const ts_data,
                      const os_callback_t cb, void* usrptr, const int node)
{
    if (cb != NULL)
    {
        cb(type, lane, ts_data, usrptr);
    }
    else
    {
        if (type == 0)
        {
            DebugVPrint("ProcessOS: OS/TS sequence broken on lane %d at node %d\n", lane, node);
            linkevent->FlaggedIdle[lane]++;
        }
        else
        {
            if (type == IDL)
            {
                DebugVPrint("ProcessOS: Seen IDLE OS on lane %d at node %d\n", lane, node);
                linkevent->IdleCount[lane]++;
            }
            else if (type == SKP)
            {
                DebugVPrint("ProcessOS: Seen SKIP OS on lane %d at node %d\n", lane, node);
                linkevent->SkipCount[lane]++;
            }
            else if (type == FTS)
            {
                DebugVPrint("ProcessOS: Seen FTS OS on lane %d at node %d\n", lane, node);
                linkevent->FtsCount[lane]++;
            }
            else if (type == TS1_ID)
            {
                DebugVPrint("ProcessOS: Seen TS1 on lane %d at node %d\n", lane, node);
                DebugVPrint("           linknum=0x%02x lanenum=0x%02x n_fts=%d datarate=%d control=%d\n", ts_data->linknum, ts_data->lanenum, ts_data->n_fts, ts_data->datarate, ts_data->control);
            
                //DispOS(state, type, ts_data, lane, true, node);
            
                linkevent->Ts1Count[lane]++;
                linkevent->LastTS[lane] = *ts_data;
            }
            else if (type == TS2_ID)
            {
                DebugVPrint("ProcessOS: Seen TS2 on lane %d at node %d\n", lane, node);
                DebugVPrint("           linknum=0x%02x lanenum=0x%02x n_fts=%d datarate=%d control=%d\n", ts_data->linknum, ts_data->lanenum, ts_data->n_fts, ts_data->datarate, ts_data->control);
            
                //DispOS(state, type, ts_data, lane, true, node);
            
                linkevent->Ts2Count[lane]++;
                linkevent->LastTS[lane] = *ts_data;
            }
            else
            {
                DebugVPrint("ProcessOS: Warning --- Seen unrecognised OS event on lane %d at node %d\n", lane, node);
            }

            DispOS(state, type, ts_data, lane, true, node);
        }
    }
}

// ----------------------- Exported functions -------------------------

// -------------------------------------------------------------------------
// CheckFree()
//
// Free memory pointed to by ptr, with check it isn't NULL.
//
// -------------------------------------------------------------------------

void CheckFree (void*  ptr)
{
    if (! ptr)
    {
        VPrint ("CheckFree(): %s***Error --- received null ptr to free%s\n", fmterrstr, fmtnormstr);
        exit (EXIT_FAILURE);
    }
    else
    {
        DebugVPrint ("CheckFree %p\n", ptr);
        free (ptr);
    }
}

// -------------------------------------------------------------------------
// CalcNewRand()
//
// Platform independant PRN generation function.
//
// -------------------------------------------------------------------------

static const unsigned int checktab[32]    = PRN_CHECKTAB_INIT;
static const unsigned int ByteParity[256] = PARITY_ARRAY_INIT;

uint32_t CalcNewRand(const unsigned int Seed)
{
     register uint32_t NewNum = 0;
     register int i;

     for (i = 0; i < 32; i++)
     {
         NewNum |= PcieOddParity(Seed & checktab[i]) << i;
     }

     return NewNum;
}

// -------------------------------------------------------------------------
// PcieCompare()
//
// Routine used by qsort for reording completion requests.
//
// -------------------------------------------------------------------------

static int PcieCompare (const void *p1, const void *p2)
{
    pPkt_t pkt1, pkt2;

    pkt1 = (pPkt_t)p1;
    pkt2 = (pPkt_t)p2;

    if (pkt1->TimeStamp > pkt2->TimeStamp)
    {
        return (1);
    }
    else if (pkt1->TimeStamp < pkt2->TimeStamp)
    {
        return (-1);
    }

    return (0);
}

// -------------------------------------------------------------------------
// PcieSortQueue()
//
// Reorders the completion delay queue by timestamp,
// modified by AddPktToQueueDelay() to give a processing
// time rather than a creation time.
//
// -------------------------------------------------------------------------

static pPkt_t PcieSortQueue (const pPkt_t const head_p, pPkt_t* end, const int node)
{
    pPkt_t array[MAX_HDR_CREDITS+1], tmp_p, newhead_p;
    int idx = 0, len;

    tmp_p = head_p;

    // Troll through linked list, placing pointers into an array
    while(tmp_p != NULL && idx < (MAX_HDR_CREDITS+1))
    {
        array[idx++] = tmp_p;
        tmp_p = tmp_p->NextPkt;
    }

    // Check we haven't overflowed
    if (idx == (MAX_HDR_CREDITS+1) && tmp_p != NULL)
    {
        VPrint("PcieSortQueue(): %s***Error --- completion queue overflow%s\n", fmterrstr, fmtnormstr);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // If less than 2 elements, no need to sort
    if (idx <= 1)
    {
        return head_p;
    }

    len = idx;

    // Perform sorting
    qsort ((void *)array, len, sizeof(pPkt_t), PcieCompare);

    // Reconstruct list
    newhead_p = array[0];
    for (idx = 0; idx < len; idx++)
    {
        if (idx < (len-1))
        {
            array[idx]->NextPkt = array[idx+1];
        }
        else
        {
            array[idx]->NextPkt = NULL;
            *end = array[idx];
        }
    }

    return newhead_p;
}

// -------------------------------------------------------------------------
// CalcByteCount()
//
// TLP completion header byte count calculator.
//
// -------------------------------------------------------------------------

int CalcByteCount (const int len, const int fbe, const int lbe)
{
    if (len == 0 && fbe == 0xf && lbe == 0xf)
    {
        VPrint( "CalcByteCount: %s***Error --- Len = 0, with valid byte enables%s\n", fmterrstr, fmtnormstr);
        exit (EXIT_FAILURE);
    }

    if (lbe == 0)
    {
        return (((fbe & 0x9) == 0x9)                          ? 4 :
               (((fbe & 0xd) == 0x5) || ((fbe & 0xb) == 0xa)) ? 3 :
                 (fbe == 0x3 || fbe == 0x6 || fbe == 0xc)     ? 2 :
                                                                1);
    }
    else
    {
        return (len*4) - ((fbe == 0xe) ? 1 : (fbe == 0xc) ? 2 : (fbe == 0x8) ? 3 : 0) -
                         ((lbe == 0x7) ? 1 : (lbe == 0x3) ? 2 : (lbe == 0x1) ? 3 : 0);
    }
}

// -------------------------------------------------------------------------
// CalcLoAddr()
//
// Returns two lowest address bits based on the first
// byte enable field of a TLP header.
//
// -------------------------------------------------------------------------

int CalcLoAddr (const int fbe)
{
    return ((fbe == 0) || (fbe & 0x1)) ? 0 : (fbe & 0x2) ? 1 : (fbe & 0x4) ? 2 : 3;
}

// -------------------------------------------------------------------------
// CalcBe()
//
// Returns the 8 bit LBE/FBE TLP header field, based on
// address and byte length.
//
// -------------------------------------------------------------------------

int CalcBe (const int inaddr, const int byte_len)
{
    int val, endpos;
    int addr = inaddr & 0x3;

    endpos = addr + byte_len;

    // First BE
    val = (0xf << (addr & 0x3)) & 0xf;

    // Only one double word---combine start and end BEs
    if (endpos <= 4)
    {
        val &= 0xf >> (4-endpos);
    // More than one DW
    }
    else
    {
        endpos %= 4;
        endpos += endpos ? 0 : 4;
        val |= (0xf0 >> (4-endpos)) & 0xf0;
    }
    return val;
}

// -------------------------------------------------------------------------
// CalcDllpCrc()
//
// 16 bit CRC calculator, based on a polynomial of 0x100b
//
// -------------------------------------------------------------------------

void CalcDllpCrc(PktData_t *dllp)
{
    uint32_t Crc=DLLP_CRC_INITIAL_VALUE;
    uint32_t Data;
    int i;

    for (i = 1; i < 5; i++)
    {
        Data = (uint32_t)dllp[i];
        Crc = PciCrc(Data, Crc, 8, DLLPPOLY, DLLPCRCSIZE);
    }

    dllp[DLLP_CRC_OFFSET]   = (PktData_t)Bitrev8[(Crc >> 8) & BYTE_MASK] ^ BYTE_MASK;
    dllp[DLLP_CRC_OFFSET+1] = (PktData_t)Bitrev8[(Crc >> 0) & BYTE_MASK] ^ BYTE_MASK;
}

// -------------------------------------------------------------------------
// CalcEcrc()
//
// 32 bit CRC calculator, masking variant fields for ECRC.
// Input is a pointer to a data packet (with blank ECRC).
//
// -------------------------------------------------------------------------

void CalcEcrc(PktData_t *pkt)
{
    int i = TLP_TYPE_BYTE_OFFSET;
    uint32_t Crc = TLP_CRC_INITIAL_VALUE;
    uint32_t InvariantMask;

    if ((pkt[TLP_TD_BYTE_OFFSET] & TLP_DIGEST_MASK) == 0)
    {
        DebugVPrint( "CalcEcrc(): Warning --- Digest bit not set. Not adding Ecrc.\n");
        return;
    }

    // Terminate CRC generation on the byte before the ECRC position
    while (pkt[i + ECRC_TERMINATION_LOOKAHEAD] != PKT_TERMINATION)
    {
        InvariantMask = (i == TLP_TYPE_BYTE_OFFSET) ? TLP_TYPE_VARIANT_BIT : (i == TLP_EP_BYTE_OFFSET) ? TLP_EP_VARIANT_BIT : 0;
        Crc = PciCrc ((uint32_t)pkt[i] | InvariantMask, Crc, 8, TLPPOLY, TLPCRCSIZE);
        i++;
    }

    // Add CRC to packet
    pkt[i++] = (PktData_t)Bitrev8[(Crc >> 24) & BYTE_MASK] ^ BYTE_MASK;
    pkt[i++] = (PktData_t)Bitrev8[(Crc >> 16) & BYTE_MASK] ^ BYTE_MASK;
    pkt[i++] = (PktData_t)Bitrev8[(Crc >>  8) & BYTE_MASK] ^ BYTE_MASK;
    pkt[i++] = (PktData_t)Bitrev8[(Crc >>  0) & BYTE_MASK] ^ BYTE_MASK;
}

// -------------------------------------------------------------------------
// CalcLcrc()
//
// 32 bit LCRC generator. Inputs is a pointer to a packet,
// with pre-calculated ECRC (if applicable).
//
// -------------------------------------------------------------------------

void CalcLcrc(PktData_t *pkt)
{
    int i = DLLP_SEQ_OFFSET;
    uint32_t Crc = TLP_CRC_INITIAL_VALUE;

    // Termintate CRC calculation at byte before LCRC position
    while (pkt[i + LCRC_TERMINATION_LOOKAHEAD] != PKT_TERMINATION)
    {
        Crc = PciCrc ((uint32_t)pkt[i], Crc, 8, TLPPOLY, TLPCRCSIZE);
        i++;
    }

    // Add CRC to packet
    pkt[i++] = (PktData_t)Bitrev8[(Crc >> 24) & BYTE_MASK] ^ BYTE_MASK;
    pkt[i++] = (PktData_t)Bitrev8[(Crc >> 16) & BYTE_MASK] ^ BYTE_MASK;
    pkt[i++] = (PktData_t)Bitrev8[(Crc >>  8) & BYTE_MASK] ^ BYTE_MASK;
    pkt[i++] = (PktData_t)Bitrev8[(Crc >>  0) & BYTE_MASK] ^ BYTE_MASK;
}

// -------------------------------------------------------------------------
// CreateTlpTemplate()
//
// Creates a TLP data packet, based on requested type,
// filling in various fields with defaults, which may
// subsequently be overwritten. Memory is dynamically
// allocated for the data packet, and must be freed when
// the packet is no longer required. I.e. after it has
// been successfully acknowledged.
//
// -------------------------------------------------------------------------

PktData_t * CreateTlpTemplate (const int Type, const uint64_t addr, const int bytelen, const int digest_present, PktData_t **payload_start)
{
    int type = Type;
    int payload_length, header_length, tail_length; // length units are bytes
    int total_length, actual_length, idx;
    int endpos;
    uint32_t addr32;
    int i;

    PktData_t *pmem;

    if (bytelen < 0 || bytelen > MAX_PAYLOAD_BYTES)
    {
        VPrint( "CreateTlpTemplate(): %s***Error --- bytelen out of range (%d)%s\n", fmterrstr, bytelen, fmtnormstr);
        return NULL;
    }

    // If a message, mask routing type for switch statement
    if ((type & MSG_IDENTIFIER_BITS) == MSG_IDENTIFIER_VALUE)
    {
        type &= MSG_ROUTE_BIT_MASK;
    }

    // Calculate the common numbers
    endpos = ((uint32_t)(addr & ADDR_DW_OFFSET_MASK) + bytelen) & ADDR_DW_OFFSET_MASK;
    payload_length = bytelen ? (bytelen + (addr & ADDR_DW_OFFSET_MASK) + (endpos ? (4 - endpos)%4 : 0)) : 0;
    if (payload_length == 0) /* We have to deal with special zero byte reads and write having non-zero payloads */
    {
        if (Type == TL_MRD64 || Type == TL_MRD32 || Type == TL_MWR64 || Type == TL_MWR32)
        {
            payload_length = 4;
        }
    }

    tail_length    = digest_present ? TAILDWDIGEST : TAILDWNODIGEST;


    switch (type)
    {
    case TL_MRD32:
    case TL_MRD64:
    case TL_MRDLCK32:
    case TL_MRDLCK64:
    case TL_MWR32:
    case TL_MWR64:
    case TL_IOWR:
    case TL_IORD:

        // If upper 32 bits of 64 bit address are clear, PCIE requires
        // the packet to be a 32 bit address TLP type
        if (!(addr & ADDR_HI_BIT_MASK))
        {
            type &= TL_ADDR64_MASK;
        }

        // 3 or 4 DW header?
        header_length  = (type & ~TL_ADDR64_MASK) ? HDR4DW : HDR3DW;

        // Actual payload length may be different
        actual_length  = (type == TL_MWR32 || type == TL_MWR64 || type == TL_IOWR) ? payload_length : 0;

        total_length = header_length + actual_length + tail_length + FIXED_OVERHEAD_LENGTH;

        if (total_length % 4)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- total_length not DWORD aligned%s\n", fmterrstr, fmtnormstr);
            VPrint( "total_length=%d header_length=%d tail_length=%d payload_length=%d actual_length=%d bytelen=%d endpos=%d (addr & 0x3)=%x\n",
                             total_length, header_length, tail_length, payload_length, actual_length, bytelen, endpos, ((uint32_t)addr & 0x3));
            return NULL;
        }

        if ((pmem = (PktData_t *)calloc ((total_length+1) * sizeof(PktData_t), 1)) == NULL)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- malloc call failed%s\n", fmterrstr, fmtnormstr);
            return NULL;
        }

        // Return pointer to the first enabled byte (un-enabled start/end bytes have undefined values)
        *payload_start = &pmem[FIXED_OVERHEAD_START + header_length + (addr & ADDR_DW_OFFSET_MASK)];

        idx = 0;

        // Preamble
        pmem[idx++] = STP;
        pmem[idx++] = 0;
        pmem[idx++] = 0;                                                                // seq number defaults to 0

        // Header 1
        pmem[idx++] = type & TLP_TYPE_MASK;
        pmem[idx++] = 0;                                                                // default TC of 0
        pmem[idx++] = ((digest_present & 1) << 7) | (((payload_length/4) >> 8) & 0x3);  // defaults attr = 0 and EP = 0
        pmem[idx++] = (payload_length/4) & BYTE_MASK;

        // Header 2
        pmem[idx++] = 0;
        pmem[idx++] = 0;                                                                // req id defaults to 0
        pmem[idx++] = 0;                                                                // tag defaults to 0;
        pmem[idx++] = CalcBe(addr, bytelen);

        // 64 bit header
        if (type & ~TL_ADDR64_MASK)
        {
            addr32 = (uint32_t)(addr >> 32ULL);
            pmem[idx++] = (addr32 >> 24) & BYTE_MASK;
            pmem[idx++] = (addr32 >> 16) & BYTE_MASK;
            pmem[idx++] = (addr32 >> 8)  & BYTE_MASK;
            pmem[idx++] = (addr32 >> 0)  & BYTE_MASK;
        }

        addr32 = (uint32_t)(addr & ADDR_LO_BIT_MASK);
        pmem[idx++] = (addr32 >> 24) & BYTE_MASK;
        pmem[idx++] = (addr32 >> 16) & BYTE_MASK;
        pmem[idx++] = (addr32 >> 8)  & BYTE_MASK;
        pmem[idx++] = (addr32 >> 0)  & ADDR_LO_BYTE_MASK;

        // Jump over payload and tail
        memset (&(pmem[idx]), 0, (actual_length + tail_length) * sizeof(PktData_t));
        idx += actual_length + tail_length;
        pmem[idx++] = END;
        pmem[idx++] = PKT_TERMINATION;

        return pmem;

        break;

    case TL_CPL:
    case TL_CPLD:
    case TL_CPLLK:
    case TL_CPLDLK:
        header_length  = HDR3DW;
        total_length = header_length + payload_length + tail_length + FIXED_OVERHEAD_LENGTH;

        if (total_length % 4)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- total_length not DWORD aligned%s\n", fmterrstr, fmtnormstr);
            VPrint( "total_length=%d header_length=%d tail_length=%d payload_length=%d bytelen=%d endpos=%d (addr & 0x3)=%x\n",
                             total_length, header_length, tail_length, payload_length, bytelen, endpos, ((uint32_t)addr & 0x3));
            return NULL;
        }

        if ((pmem = (PktData_t *)calloc (total_length * sizeof(PktData_t) + sizeof(PktData_t), 1)) == NULL)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- malloc call failed%s\n", fmterrstr, fmtnormstr);
            return NULL;
        }

        // Return pointer to the first enabled byte (un-enabled start/end bytes have undefined values)
        *payload_start = &pmem[FIXED_OVERHEAD_START + header_length + (addr & ADDR_DW_OFFSET_MASK)];

        idx = 0;

        // Preamble
        pmem[idx++] = STP;
        pmem[idx++] = 0;
        pmem[idx++] = 0;

        // Header 1
        pmem[idx++] = type & TLP_TYPE_MASK;
        pmem[idx++] = 0;                                                                // default TC of 0
        pmem[idx++] = ((digest_present & 1) << 7) | (((payload_length/4) >> 8) & 0x3);  // defaults attr = 0 and EP = 0
        pmem[idx++] = (payload_length/4) & BYTE_MASK;

        // Header 2
        pmem[idx++] = 0;
        pmem[idx++] = 0;                                                                // Cmpl ID default of 0
        pmem[idx++] = (bytelen >> 8) & 0xf;                                             // status = success, BCM = 0
        pmem[idx++] = (bytelen & BYTE_MASK);

        // Header 3
        pmem[idx++] = 0;
        pmem[idx++] = 0;                                                                // req id defaults to 0
        pmem[idx++] = 0;                                                                // tag defaults to 0;
        pmem[idx++] = (PktData_t)(addr & TLP_CPL_LO_ADDR_MASK);                         // lower addr bits

        // Jump over payload and tail
        memset (&(pmem[idx]), 0, (payload_length + tail_length) * sizeof(PktData_t));
        idx += payload_length + tail_length;
        pmem[idx++] = END;
        pmem[idx++] = PKT_TERMINATION;

        return pmem;

        break;

    case TL_CFGRD0:
    case TL_CFGWR0:
    case TL_CFGRD1:
    case TL_CFGWR1:

        header_length  = HDR3DW;
        actual_length  = (type == TL_CFGWR0 || type == TL_CFGWR1) ? payload_length : 0;
        total_length = header_length + actual_length + tail_length + FIXED_OVERHEAD_LENGTH;

        if (total_length % 4)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- total_length not DWORD aligned%s\n", fmterrstr, fmtnormstr);
            VPrint( "total_length=%d header_length=%d tail_length=%d payload_length=%d bytelen=%d endpos=%d (addr & 0x3)=%x\n",
                             total_length, header_length, tail_length, payload_length, bytelen, endpos, ((uint32_t)addr & 0x3));
            return NULL;
        }

        if ((pmem = (PktData_t *)calloc (total_length * sizeof(PktData_t) + sizeof(PktData_t), 1)) == NULL)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- malloc call failed%s\n", fmterrstr, fmtnormstr);
            return NULL;
        }

        // Return pointer to the first enabled byte (un-enabled start/end bytes have undefined values)
        *payload_start = &pmem[FIXED_OVERHEAD_START + header_length + (addr & ADDR_DW_OFFSET_MASK)];

        idx = 0;

        // Preamble
        pmem[idx++] = STP;
        pmem[idx++] = 0;
        pmem[idx++] = 0;

        // Header 1
        pmem[idx++] = type & TLP_TYPE_MASK;
        pmem[idx++] = 0;                                                                // default TC of 0
        pmem[idx++] = ((digest_present & 1) << 7) | (((payload_length/4) >> 8) & 0x3);  // defaults attr = 0 and EP = 0
        pmem[idx++] = (payload_length/4) & BYTE_MASK;

        // Header 2
        pmem[idx++] = 0;
        pmem[idx++] = 0;                                                                // req id defaults to 0
        pmem[idx++] = 0;                                                                // tag defaults to 0;
        pmem[idx++] = CalcBe(addr, bytelen);

        // Header 3
        pmem[idx++] = 0;
        pmem[idx++] = 0;
        pmem[idx++] = (addr >> 8);
        pmem[idx++] = (addr & ADDR_LO_BYTE_MASK);

        // Jump over payload and tail
        memset (&(pmem[idx]), 0, (actual_length + tail_length) * sizeof(PktData_t));
        idx += actual_length + tail_length;

        pmem[idx++] = END;
        pmem[idx++] = PKT_TERMINATION;

        return pmem;

        break;

    case TL_MSG:
    case TL_MSGD:
        header_length  = HDR4DW;
        total_length = header_length + payload_length + tail_length + FIXED_OVERHEAD_LENGTH;

        if (total_length % 4)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- total_length not DWORD aligned%s\n", fmterrstr, fmtnormstr);
            VPrint( "total_length=%d header_length=%d tail_length=%d payload_length=%d bytelen=%d endpos=%d (addr & 0x3)=%x\n",
                             total_length, header_length, tail_length, payload_length, bytelen, endpos, ((uint32_t)addr & 0x3));
            return NULL;
        }

        if ((pmem = (PktData_t *)calloc (total_length * sizeof(PktData_t) + sizeof(PktData_t), 1)) == NULL)
        {
            VPrint( "CreateTlpTemplate(): %s***Error --- malloc call failed%s\n", fmterrstr, fmtnormstr);
            return NULL;
        }

        // Return pointer to the first enabled byte (un-enabled start/end bytes have undefined values)
        *payload_start = &pmem[FIXED_OVERHEAD_START + header_length + (addr & ADDR_DW_OFFSET_MASK)];

        idx = 0;

        // Preamble
        pmem[idx++] = STP;
        pmem[idx++] = 0;
        pmem[idx++] = 0;

        // Header 1
        pmem[idx++] = Type & TLP_TYPE_MASK;
        pmem[idx++] = 0;                                                                // default TC of 0
        pmem[idx++] = ((digest_present & 1) << 7) | (((payload_length/4) >> 8) & 0x3);  // defaults attr = 0 and EP = 0
        pmem[idx++] = (payload_length/4) & BYTE_MASK;

        // Header 2
        pmem[idx++] = 0;
        pmem[idx++] = 0;                                                                // req id defaults to 0
        pmem[idx++] = 0;                                                                // tag defaults to 0;
        pmem[idx++] = 0;                                                                // message code defaults to 0 (unlock)

        // Header 3
        pmem[idx++] = 0;
        pmem[idx++] = 0;
        pmem[idx++] = 0;
        pmem[idx++] = 0;

        // Header 4
        pmem[idx++] = 0;
        pmem[idx++] = 0;
        pmem[idx++] = 0;
        pmem[idx++] = 0;

        // Jump over payload and tail
        memset (&(pmem[idx]), 0, (payload_length + tail_length) * sizeof(PktData_t));
        idx += payload_length + tail_length;

        pmem[idx++] = END;
        pmem[idx++] = PKT_TERMINATION;

        return pmem;

        break;

    default:
        VPrint( "CreateTlpTemplate(): %s***Error --- Unrecognised type%s\n", fmterrstr, fmtnormstr);
        return NULL;
        break;
    }

}

// -------------------------------------------------------------------------
// CreateDllpTemplate()
//
// Create a template data packet for a DLLP of given Type,
// and fill in with some defaults which can be overridden
// at a later date. Memory for the data packet is
// dynamically allocated, and must be freed when the packet
// is no longer required. I.e. when sent over the link.
//
// -------------------------------------------------------------------------

PktData_t * CreateDllpTemplate (const int Type, PktData_t **payload_start)
{
    PktData_t *pmem;
    int SwitchType = Type;

    if ((pmem = (PktData_t *)calloc (9, sizeof(PktData_t))) == NULL)
    {
        VPrint( "CreateDllpTemplate(): %s***Error --- malloc call failed%s\n", fmterrstr, fmtnormstr);
        return NULL;
    }

    // For selecting the requested type, the virtual channel bits
    // ([2:0]) are masked, except DL_PM_ENTER_L1 where they have
    // a different meaning.
    if ((Type & DL_VC_MASK) != DL_PM_ENTER_L1)
    {
        SwitchType &= DL_VC_MASK;
    }

    switch(SwitchType){
    case DL_ACK:
    case DL_NAK:
    case DL_INITFC1_P:
    case DL_INITFC2_P:
    case DL_INITFC1_NP:
    case DL_INITFC2_NP:
    case DL_INITFC1_CPL:
    case DL_INITFC2_CPL:
    case DL_UPDATEFC_P:
    case DL_UPDATEFC_NP:
    case DL_UPDATEFC_CPL:
    case DL_PM_ENTER_L1:
    case DL_PM_ENTER_L23:
    case DL_PM_REQ_L0S:
    case DL_PM_REQ_L1:
    case DL_PM_REQ_ACK:
    case DL_VENDOR:
        pmem[0] = SDP;
        pmem[1] = Type;
        pmem[2] = 0;
        pmem[3] = 0;
        pmem[4] = 0;
        pmem[5] = 0;
        pmem[6] = 0;
        pmem[7] = END;
        pmem[8] = PKT_TERMINATION;
        *payload_start = &pmem[2];
        return pmem;
        break;
    default:
        VPrint( "CreateDllpTemplate(): %s***Error --- Unrecognised type%s\n", fmterrstr, fmtnormstr);
        return NULL;
        break;
   }
}

// -------------------------------------------------------------------------
// CheckCredits()
//
// Returns true if enough credits to send a packet
//
// -------------------------------------------------------------------------

int CheckCredits(const int DisableFc, const uint32_t fc_state, const uint32_t hdr_credits, const uint32_t data_credits,
                 const uint32_t tx_hdr, const uint32_t tx_data, const int payload_len)
{
    return (DisableFc || ((((hdr_credits - tx_hdr) > 0) || !hdr_credits) /* && (fc_state == INITFC_FI2)*/  &&
           (((data_credits - tx_data) >= (payload_len/4 + ((payload_len%4)?1:0))) || !data_credits)));

}

// -------------------------------------------------------------------------
// InitPcieState()
//
// Initialise the pcie state associated with 'node'.
//
// -------------------------------------------------------------------------

void InitPcieState(const pPcieModelState_t const state, const int node)
{
    int fc_type, fc_vc, i;
    pUserConfig_t     usrconf   = &(state->usrconf);
    pLinkEventCount_t linkevent = &(state->linkevent);
    pFlowControl_t    flw       = &(state->flwcntl);

    // Set default state for this node
    state->thisnode      = node;
    state->LinkWidth     = MAX_LINK_WIDTH;
    state->cpl_head_p    = NULL;
    state->head_p        = state->send_p        = state->end_p = NULL;
    state->ack_to_send_p = state->nak_to_send_p = NULL;
    state->curr_ack      = state->curr_nak      = NULLACK;
    state->seq           = 0;

    for (fc_vc = 0; fc_vc < NUM_VIRTUAL_CHANNELS; fc_vc++)
    {
        for (fc_type = 0; fc_type < FC_NUMTYPES; fc_type++)
        {
            usrconf->InitFcHdrCr[fc_vc][fc_type]       = (fc_type == FC_POST) ? FC_DEFAULT_PHDR_CREDITS  : (fc_type == FC_NONPOST) ? FC_DEFAULT_NPHDR_CREDITS  : FC_DEFAULT_CPLHDR_CREDITS;
            usrconf->InitFcDataCr[fc_vc][fc_type]      = (fc_type == FC_POST) ? FC_DEFAULT_PDATA_CREDITS : (fc_type == FC_NONPOST) ? FC_DEFAULT_NPDATA_CREDITS : FC_DEFAULT_CPLDATA_CREDITS;

            flw->FlowCntlHdrCredits[fc_vc][fc_type]    = 0;
            flw->FlowCntlDataCredits[fc_vc][fc_type]   = 0;
            flw->TxHdrCredits[fc_vc][fc_type]          = 0;
            flw->TxDataCredits[fc_vc][fc_type]         = 0;

            flw->ConsumedHdrCredits[fc_vc][fc_type]    = usrconf->InitFcHdrCr[fc_vc][fc_type];
            flw->ConsumedDataCredits[fc_vc][fc_type]   = usrconf->InitFcDataCr[fc_vc][fc_type];
            flw->AdvertisedHdrCredits[fc_vc][fc_type]  = usrconf->InitFcHdrCr[fc_vc][fc_type];
            flw->AdvertisedDataCredits[fc_vc][fc_type] = usrconf->InitFcDataCr[fc_vc][fc_type];
            flw->ConsumedHdrUpdated[fc_vc][fc_type]    = 0;
            flw->ConsumedDataUpdated[fc_vc][fc_type]   = 0;
            flw->RxHdrCredits[fc_vc][fc_type]          = 0;
            flw->RxDataCredits[fc_vc][fc_type]         = 0;
            flw->LastSentFcTime[fc_vc][fc_type]        = 0;
        }
        flw->fc_state[fc_vc] = 0;
        flw->rx_fc_state[fc_vc] = INITFC_IDLE;
    }

    for (i = 0; i < MAX_LINK_WIDTH; i++)
    {
        linkevent->IdleCount[i]   = 0;
        linkevent->SkipCount[i]   = 0;
        linkevent->FtsCount[i]    = 0;
        linkevent->Ts1Count[i]    = 0;
        linkevent->Ts2Count[i]    = 0;
        linkevent->OsState[i]     = 0;
        linkevent->OsCount[i]     = 0;
        linkevent->FlaggedIdle[i] = 0;
    }

    usrconf->HdrConsumptionRate   = DEFAULT_HFC_CONSUMPTION_RATE;
    usrconf->DataConsumptionRate  = DEFAULT_DFC_CONSUMPTION_RATE;
    usrconf->CompletionRate       = DEFAULT_COMPLETION_RATE;
    usrconf->CompletionSpread     = DEFAULT_COMPLETION_SPREAD;
    usrconf->DisableMem           = 0;
    usrconf->DisableAck           = 0;
    usrconf->DisableFc            = 0;
    usrconf->DisableSkips         = 0;
    usrconf->DisableUrCpl         = 0;
    usrconf->DisableScrambling    = 0;
    usrconf->Disable8b10b         = 0;
    usrconf->DisableEcrcCmpl      = 0;
    usrconf->SkipInterval         = DEFAULT_SKIP_INTERVAL;
    usrconf->AckRate              = DEFAULT_ACK_RATE;
    usrconf->ContDispIdx          = 0;
    usrconf->ActiveContDisp       = 0;
    usrconf->BackNodeNum          = node ^ 1; // Assumes connected devices are one node apart

    ContDisp(usrconf);


    state->RandNum                = node;

    state->CompletionEvent        = 0;
    state->vuser_cb               = NULL;
    state->usrptr                 = NULL;
    state->CplId                  = 0;

    state->vuser_os_cb            = NULL;

    state->OutstandingCompletions = 0;

    state->LastTxSkipTime         = 0;
    state->SkipScheduled          = 0;

    state->RxActive               = 0;
    state->RxDataIdx              = 0;

    state->draining_queue         = false;
    state->tx_disabled            = false;

    // Initialise this node's physical layer coder/decoder state
    InitCodec(node);
}

// -------------------------------------------------------------------------
// AddPktToQueue()
//
// TLP packet pointer added to end of retry queue.
// head_p always points to oldest unacknowledged packet (or
// NULL if queue empty). end_p always points to youngest
// packet added to queue (could be equal to head_p). send_p
// points to next packet to send over the link. This could be
// either the first transmission, or a retry. When no packets
// to send, send_p is NULL.
//
// -------------------------------------------------------------------------

void AddPktToQueue(const pPcieModelState_t const state, const pPkt_t const packet)
{
    // Remove any previous linkage
    packet->NextPkt = NULL;


    // Queue is empty, so all pointers set to the end packet
    if (state->head_p == NULL)
    {
        state->end_p = state->send_p = state->head_p = packet;
    // Queue has at least one packet
    }
    else
    {
        // All packets sent, so set send_p to this packet
        if (state->send_p == NULL)
        {
            state->send_p = packet;
        }

        // Point current end of queue NextPkt pointer to
        // the new packet, and then update end_p to the
        // added packet.
        state->end_p->NextPkt = packet;
        state->end_p = packet;
    }
}

// -------------------------------------------------------------------------
// AddPktToQueueDelay()
//
// Similar to AddPktToQueue(), but keeps a separate queue
// of completions with a calculated process timestamp
// (based on config values). List is re-ordered on
// modified timestamps.
//
// -------------------------------------------------------------------------

void AddPktToQueueDelay (const pPcieModelState_t const state, const pPkt_t const packet)
{
    int num;

    // If a completion delay programmed, add this value to the time stamp
    if (state->usrconf.CompletionRate)
    {
        packet->TimeStamp += state->usrconf.CompletionRate;
    }

    // If a spread of delay programmed, munge in a random element to the TimeStamp
    if (state->usrconf.CompletionSpread)
    {
        num = (int)(PcieRand(state->thisnode) % state->usrconf.CompletionSpread) - state->usrconf.CompletionSpread/2;
        packet->TimeStamp = num + (int)packet->TimeStamp;
    }

    if (state->cpl_head_p == NULL)
    {
        state->cpl_head_p = state->cpl_end_p = packet;
    }
    else
    {
        state->cpl_end_p->NextPkt = packet;
        state->cpl_head_p = PcieSortQueue (state->cpl_head_p, &(state->cpl_end_p), state->thisnode);
    }
}

// -------------------------------------------------------------------------
// ExtractPhyInput()
//
// This is called once per symbol time. It constructs
// incoming DLLP and TLP packets, and calls ProcessInput()
// when a new input packet is complete. If order sets or
// training sequences are seen, the event is passed to
// ProcessOS().
//
// -------------------------------------------------------------------------

void ExtractPhyInput(const pPcieModelState_t const state, const unsigned int* const rawlinkin)
{
    PktData_t linkin [MAX_LINK_WIDTH];
    pPkt_t pkt;
    int idx, i;
    pLinkEventCount_t linkevent = &(state->linkevent);

    for (idx = 0; idx < state->LinkWidth; idx++)
    {
        linkin[idx] = Decode (rawlinkin[idx], state->usrconf.DisableScrambling, state->usrconf.Disable8b10b, idx, state->LinkWidth, state->thisnode);

        // ----- Extracting Ordered Sets/Training Sequences for each lane -----

        // Seen Comma for this lane
        if (linkin[idx] == COM)
        {
            // If active OS, check that we haven't had a bad OS boundary.
            if (linkevent->OsState[idx] && ((linkevent->OsCount [idx] > 15) ||
                (linkevent->OsState [idx] == IDL && linkevent->OsCount [idx] != 3) ||
                (linkevent->OsState [idx] == FTS && linkevent->OsCount [idx] != 3) ||
                (linkevent->OsState [idx] == SKP && (linkevent->OsCount [idx] > 6 || linkevent->OsCount [idx] < 1))))
            {
                    DebugVPrint("ExtractPhyInput: Warning --- bad count for ordered sets on lane %d at node %d\n", idx, state->thisnode);
            }
            linkevent->OsState [idx] = COM;
            linkevent->OsCount [idx] = 0;
            linkevent->FlaggedIdle [idx] = 0;

        // If active OS from last cycle, check for this cycle...
        }
        else if (linkevent->OsState [idx])
        {
            // If last state was a Comma and incoming is an OS symbol, or last state is an OS symbol....
            if ((linkevent->OsState [idx] == COM && (linkin[idx] == IDL || linkin[idx] == SKP || linkin[idx] == FTS) ||
                 linkevent->OsState [idx] == IDL || linkevent->OsState [idx] == SKP || linkevent->OsState [idx] == FTS))
            {
                // ... and if Comma or current input is same as previous symbol, then we are continuing an OS
                if (linkevent->OsState[idx] == COM || linkin[idx] == linkevent->OsState[idx])
                {
                    // Increment count of OS symbols
                    linkevent->OsCount[idx]++;
                    // Update current state to input symbol
                    linkevent->OsState[idx] = linkin[idx];
                    // If we've seen 3 OS symbols (or just 1 for skip), then an OS event has triggered
                    if (linkevent->OsCount[idx] == 3 || (linkevent->OsState[idx] == SKP && linkevent->OsCount[idx] == 1))
                    {
                        ProcessOS(state, linkevent, idx, linkevent->OsState[idx], NULL, state->vuser_os_cb, state->usrptr, state->thisnode);
                        if (linkevent->OsState[idx] != SKP)
                        {
                            linkevent->OsState[idx] = 0;
                            linkevent->OsCount[idx] = 0;
                        }
                    }
                }
                else
                {
                    // Since Skip OSs can be any length from 1 to 6, don't print an error
                    // if skip is interrupted
                    if (linkevent->OsState [idx] != SKP)
                        DebugVPrint("ExtractPhyInput: Warning --- invalid control symbols seen on lane %d at node %d\n", idx, state->thisnode);
                    linkevent->OsState [idx] = 0;
                    linkevent->OsCount [idx] = 0;
                }
            // If active state (but not an OS symbol, caught above)
            // assume we're processing a training sequence
            }
            else
            {
                linkevent->OsCount [idx]++;
                linkevent->OsState [idx] = TS1_ID;

                DebugVPrint("ExtractPhyInput: Training sequence on lane %d count=%d at node %d (linkin=%02x)\n", idx, linkevent->OsCount [idx], state->thisnode, linkin[idx]);

                // Symbol selections for training sequence
                switch (linkevent->OsCount [idx])
                {
                case  1:
                    linkevent->Tseq[idx].linknum  = linkin[idx];
                    break;
                case  2: linkevent->Tseq[idx].lanenum  = linkin[idx];
                    break;
                case  3: linkevent->Tseq[idx].n_fts    = linkin[idx];
                    break;
                case  4: linkevent->Tseq[idx].datarate = linkin[idx];
                    break;
                case  5: linkevent->Tseq[idx].control  = linkin[idx];
                    break;
                case  6: linkevent->Tseq[idx].id       = linkin[idx];
                    break;
                case  7: case  8: case  9: case 10:
                case 11: case 12: case 13: case 14: case 15:
                    if (linkevent->Tseq[idx].id != linkin[idx] || (linkin[idx] != TS1_ID && linkin[idx] != TS2_ID))
                    {
                        DebugVPrint("ExtractPhyInput: Warning --- invalid training sequence seen on at count %d, lane %d at node %d\n", linkevent->OsCount [idx], idx, state->thisnode);
                        linkevent->OsState [idx] = 0;
                        linkevent->OsCount [idx] = 0;
                    }
                    else if (linkevent->OsCount [idx] == 15)
                    {
                        DebugVPrint("ExtractPhyInput: Calling ProcessOS() with TS\n");
                        ProcessOS(state, linkevent, idx, linkin[idx], &linkevent->Tseq[idx], state->vuser_os_cb, state->usrptr, state->thisnode);
                        linkevent->OsState [idx] = 0;
                        linkevent->OsCount [idx] = 0;
                    }
                    break;
                }
            }
        }
        else
        {
            // If not processing an OS/TS, flag to ProcessOS() an 'idle' status
            // in case it is checking for consecutive ordered sets
            ProcessOS(state, linkevent, idx, 0, NULL, state->vuser_os_cb, state->usrptr, state->thisnode);
        }

        // ----- Extracting DLLP/TLPs across lanes -----

        if (linkin[idx] == STP || linkin[idx] == SDP)
        {
            if (state->RxActive)
            {
                VPrint( "ExtractPhyInput: %s***Error --- New STP/SDP (lane %d) whilst packet active at node %d%s\n", fmterrstr, idx, state->thisnode, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, state->thisnode);
            }
            state->RxActive = true;
            state->RxDataIdx = 0;
            // Allocate some space for a new packet data
            if ((state->pRxPktData = (PktData_t *)calloc((MAX_RAW_PKT_SIZE) * sizeof(PktData_t), 1)) == NULL)
            {
                VPrint( "ExtractPhyInput: %s***Error --- memory allocation failure at node %d%s\n", fmterrstr, state->thisnode, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, state->thisnode);
            }
        }

        // If a TLP or Dllp is arriving...
        if (state->RxActive)
        {
            // Copy byte to indata buffer
            if (state->RxDataIdx < (MAX_RAW_PKT_SIZE-1))
            {
                (state->pRxPktData)[state->RxDataIdx++] = linkin[idx];
            }
            else
            {
                VPrint( "ExtractPhyInput: %s***Error --- packet overflow at node %d%s\n", fmterrstr, state->thisnode, fmtnormstr);
                VWrite(PVH_FATAL, 0, 0, state->thisnode);
            }

            // If we've reached the end of a packet...
            if (linkin[idx] == END || linkin[idx] == EDB)
            {
                // Mark buffer with terminations
                (state->pRxPktData)[state->RxDataIdx++] = PKT_TERMINATION;

                // reallocate memory for the new packet data's actual size
                if ((state->pRxPktData = (PktData_t *)realloc(state->pRxPktData, state->RxDataIdx * sizeof(PktData_t))) == NULL)
                {
                    VPrint( "ExtractPhyInput: %s***Error --- memory reallocation failure at node %d%s\n", fmterrstr, state->thisnode, fmtnormstr);
                    VWrite(PVH_FATAL, 0, 0, state->thisnode);
                }

                if ((pkt = calloc(sizeof(sPkt_t), 1)) == NULL)
                {
                    VPrint( "ExtractPhyInput: %s***Error --- memory allocation failure%s\n", fmterrstr, fmtnormstr);
                    VWrite(PVH_FATAL, 0, 0, state->thisnode);
                }

                pkt->NextPkt = NULL;
                pkt->data = state->pRxPktData;
                pkt->seq  = (pkt->data[0] == SDP) ? DLLP_SEQ_ID : ((((pkt->data[1] & 0xff) << 8) | (pkt->data[2] & 0xff)) & 0xfff);
                pkt->Retry = 0;
                pkt->ByteCount = (pkt->data[0] == SDP) ? 8 : (4 * ((pkt->data[5] & 0x3) | (pkt->data[6] & 0xff)));
                pkt->TimeStamp = GetCycleCount(state->thisnode);

                state->RxActive = false;

                ProcessInput(state, pkt, (linkin[idx] == EDB));
            }
        }
    }

    DispRaw(state, linkin, true);

    // Keep track of time
    state->TicksSinceReset++;

    // Check ContDisp
    CheckContDisp(&state->usrconf, state->thisnode);

    // Check completion delay queue
    CheckDelayQueue(state);

    // Update Rx consumption counts
    if (!state->usrconf.DisableFc)
    {
        UpdateConsumedFC(state);
    }

    // Check Skip requirements
    CheckSkips(state);
}

// -------------------------------------------------------------------------
// TxFcInitInt()
//
// -------------------------------------------------------------------------

void TxFcInitInt (const pFlowControl_t const flw, const pUserConfig_t const usrcfg, const int node)
{
    while (flw->fc_state[0] == INITFC_IDLE)
    {
        SendFC(DL_INITFC1_P,   0, usrcfg->InitFcHdrCr[0][FC_POST],    usrcfg->InitFcDataCr[0][FC_POST],    QUEUE, node);
        SendFC(DL_INITFC1_NP,  0, usrcfg->InitFcHdrCr[0][FC_NONPOST], usrcfg->InitFcDataCr[0][FC_NONPOST], QUEUE, node);
        SendFC(DL_INITFC1_CPL, 0, usrcfg->InitFcHdrCr[0][FC_CMPL],    usrcfg->InitFcDataCr[0][FC_CMPL],    SEND,  node);
        if (INITFC_DELAY != 0)
        {
            SendIdle(INITFC_DELAY, node);
        }
    }

    while (flw->fc_state[0] == INITFC_FI1 || flw->fc_state[0] == INITFC_FI2)
    {
        SendFC(DL_INITFC2_P,   0, usrcfg->InitFcHdrCr[0][FC_POST],    usrcfg->InitFcDataCr[0][FC_POST],    QUEUE, node);
        SendFC(DL_INITFC2_NP,  0, usrcfg->InitFcHdrCr[0][FC_NONPOST], usrcfg->InitFcDataCr[0][FC_NONPOST], QUEUE, node);
        SendFC(DL_INITFC2_CPL, 0, usrcfg->InitFcHdrCr[0][FC_CMPL],    usrcfg->InitFcDataCr[0][FC_CMPL],    SEND,  node);
        if (INITFC_DELAY != 0)
        {
            SendIdle(INITFC_DELAY, node);
        }

        // Ensure that at least one InitFC2 set is transmitted, even if at INITFC_FI2 already
        if (flw->fc_state[0] == INITFC_FI2)
        {
            break;
        }
    }
}

// -------------------------------------------------------------------------
// RxFcInit()
//
// -------------------------------------------------------------------------

void RxFcInit (const pFlowControl_t const flw, const int type, const int hdrval, const int dataval)
{
    DebugVPrint("** Received InitFC type %d hdrval=%d dataval=%d rx_fc_state=%d fc_state=%d\n",
            type, hdrval, dataval, flw->rx_fc_state[0], flw->fc_state[0]);

    if (type == DL_INITFC1_P || type == DL_INITFC2_P)
    {
        flw->rx_fc_state[0] |= RCVD_P;
        // Only update flow control credits when no FI flag set
        if (flw->fc_state[0] == INITFC_IDLE)
        {
            flw->FlowCntlHdrCredits[0][FC_POST] = hdrval;
            flw->FlowCntlDataCredits[0][FC_POST] = dataval;
        }
    }

    if (type == DL_INITFC1_NP || type == DL_INITFC2_NP)
    {
        flw->rx_fc_state[0] |= RCVD_NP;
        // Only update flow control credits when no FI flag set
        if (flw->fc_state[0] == INITFC_IDLE)
        {
            flw->FlowCntlHdrCredits[0][FC_NONPOST] = hdrval;
            flw->FlowCntlDataCredits[0][FC_NONPOST] = dataval;
        }
    }

    if (type == DL_INITFC1_CPL || type == DL_INITFC2_CPL)
    {
        flw->rx_fc_state[0] |= RCVD_CPL;
        // Only update flow control credits when no FI flag set
        if (flw->fc_state[0] == INITFC_IDLE)
        {
            flw->FlowCntlHdrCredits[0][FC_CMPL] = hdrval;
            flw->FlowCntlDataCredits[0][FC_CMPL] = dataval;
        }
    }

    // If we have at least one of each INITFC type, move on flow control state
    if (flw->rx_fc_state[0] == RCVD_ALL)
    {
        if (flw->fc_state[0] == INITFC_IDLE)
        {
            flw->fc_state[0] = INITFC_FI1;
        }
        else if (flw->fc_state[0] == INITFC_FI1)
        {
            flw->fc_state[0] = INITFC_FI2;
        }

        // Reset the receive flags
        flw->rx_fc_state[0] = 0;
    }
}

