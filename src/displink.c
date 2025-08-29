//=============================================================
//
// Copyright (c) 2025 Simon Southwell. All rights reserved.
//
// Date: 14th May 2025
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
// PCIe link display routines
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include "pcie.h"
#include "pcie_utils.h"
#include "displink.h"

char fmtupstr[FMT_STR_SIZE]   = {0};
char fmtdnstr[FMT_STR_SIZE]   = {0};
char fmterrstr[FMT_STR_SIZE]  = {0};
char fmtnormstr[FMT_STR_SIZE] = {0};
char fmtdatastr[FMT_STR_SIZE] = {0};

// -------------------------------------------------------------------------
// IsDispEnabled()
//
// Returns true if okay to do software display
//
// -------------------------------------------------------------------------

static bool IsDispEnabled(const pPcieModelState_t const state, const int rx, int flags)
{
    return (rx || (!rx && (state->usrconf.ActiveContDisp & DISPSWTX))) &&
                  ((state->Endpoint && (state->usrconf.ActiveContDisp & DISPSWENEP)) || (!state->Endpoint && (state->usrconf.ActiveContDisp & DISPSWENRC))) &&
                  (state->usrconf.ActiveContDisp & flags);
}

// -------------------------------------------------------------------------
// ConfigDispFormat()
// 
// Enable or disable link display colour formatting
// 
// -------------------------------------------------------------------------

void ConfigDispFormat(bool enable)
{
    if (enable)
    {
        strncpy(fmtupstr,   FMT_BRIGHT_BLUE,  FMT_STR_SIZE);
        strncpy(fmtdnstr,   FMT_BRIGHT_GREEN, FMT_STR_SIZE);
        strncpy(fmterrstr,  FMT_RED,          FMT_STR_SIZE);
        strncpy(fmtnormstr, FMT_NORMAL,       FMT_STR_SIZE);
        strncpy(fmtdatastr, FMT_DATA_GREY,    FMT_STR_SIZE);
    }
    else
    {
        strncpy(fmtupstr,   "", FMT_STR_SIZE);
        strncpy(fmtdnstr,   "", FMT_STR_SIZE);
        strncpy(fmterrstr,  "", FMT_STR_SIZE);
        strncpy(fmtdatastr, "", FMT_STR_SIZE);
    }
}

// -------------------------------------------------------------------------
// ContDisps()
//
// Read ContDisps.hex file and load in to this node's state
//
// -------------------------------------------------------------------------

void ConstDisp (pUserConfig_t usrconf)
{
    int error = 0;

    // Default the colour formatting strings
    ConfigDispFormat(true);

    FILE* fp = fopen("hex/ContDisps.hex", "r");

    if (fp == NULL)
    {
        error++;
        VPrint("%s**ERROR**%s: ConstDisp() failed to read ContDisps.hex file, No display output.\n", fmterrstr, fmtnormstr);
    }

    char buf [STRBUFSIZE];

    int dispidx = 0;

    if (!error)
    {
        while (fgets(buf, STRBUFSIZE, fp))
        {
            int control;
            uint64_t time;
        
            // Removing whitespace
            int sidx = 0;
            while (buf[sidx] == ' ' || buf[sidx] == '\t')
            {
                sidx++;
            }
        
            if ((buf[sidx] >= '0' && buf[sidx] <= '9') ||
                (buf[sidx] >= 'a' && buf[sidx] <= 'f') ||
                (buf[sidx] >= 'A' && buf[sidx] <= 'F'))
            {
                sscanf(&buf[sidx], "%x %llu", &usrconf->contdisp[dispidx].control, (long long unsigned *)&usrconf->contdisp[dispidx].time);
        
                if (++dispidx == MAXCONSTDISP)
                {
                    break;
                }
            }
        }
    }

    fclose(fp);
}

// -------------------------------------------------------------------------
// CheckContDisp()
//
// Check if to activate next ContDisp entry
// -------------------------------------------------------------------------

inline void CheckContDisp (pUserConfig_t usrconf, const int node)
{
    uint32_t cycleCount = GetCycleCount(node);

    // If current time is at, or beyond, the indexed ContDisp...
    if (usrconf->ContDispIdx < MAXCONSTDISP && GetCycleCount(node) >= usrconf->contdisp[usrconf->ContDispIdx].time)
    {
        // Update active control with next value
        usrconf->ActiveContDisp = usrconf->contdisp[usrconf->ContDispIdx].control;

        // If simulation control bits set, action the control
        if (usrconf->ActiveContDisp & (DISPFINISH | DISPSTOP))
        {
            // Halt the simulation
            VWrite((usrconf->ActiveContDisp & DISPSTOP) ? PVH_STOP : PVH_FINISH, 0, 0, node);
        }

        // Increment the ContDisp index (up to maximum)
        if (usrconf->ContDispIdx < MAXCONSTDISP)
        {
            usrconf->ContDispIdx++;
        }
    }
}

// -------------------------------------------------------------------------
// DispTc()
//
// Display traffic class and other attributes
// -------------------------------------------------------------------------

inline static void DispTc (const char const *prefixstr, const char const *offstr,
                           const uint32_t tl_type, const uint32_t tl_tc, const uint32_t tl_td,
                           const uint32_t tl_ep, const uint32_t tl_attr, const uint32_t tl_length)
{
    VPrint("%s: %sTraffic Class=%d%s%s%s%s", prefixstr, offstr, tl_tc,
        tl_td           ? ", TLP Digest" : "",
        tl_ep           ? ", Poisoned"   : "",
        (tl_attr & 0x2) ? ", Relaxed ordering (PCI-X)" : ", Strong ordering (PCI)",
        (tl_attr & 0x1) ? ", No snoop" : ""
    );

    if (tl_type & TL_TYPE_WRITE)
        VPrint(", Payload Length=0x%03x DW",  tl_length ? tl_length : 1024);
    VPrint("\n");
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

static inline void DispPayload(const char const *prefixstr, const char const *tloffstr, pPkt_t pkt, const uint32_t data_offset, const uint32_t tl_length)
{
    unsigned idx;

    for (idx = 0; idx < (4 * tl_length); idx += 4)
    {
        uint32_t wdata = (pkt->data[data_offset+ idx] << 24) | (pkt->data[data_offset+ idx + 1] << 16)  | (pkt->data[data_offset+ idx + 2] << 8)  | (pkt->data[data_offset+ idx + 3]);

        if (!((idx / 4) % 8))
            VPrint("%s: %s%s", prefixstr, tloffstr, fmtdatastr);

        VPrint("%08x",wdata);

        if (((idx / 4) % 8) == 7)
            VPrint("%s\n", fmtnormstr);
        else
            VPrint(" ");
    }

    if ((idx/4)%8)
        VPrint("%s\n", fmtnormstr);
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------

inline static void DispTlpCrc(const char const *prefixstr, const char const *tloffstr, const char const *dlloffstr,
                              const uint32_t tl_td, pPkt_t pkt)
{
    PktData_t tl_type = pkt->data[3] & 0x7f;

    // Calculate LCRC and ECRC (if present)
    uint32_t crc[4], ecrc[4];
    uint32_t exp_lcrc, got_lcrc;
    uint32_t exp_ecrc, got_ecrc;

    unsigned lcrc_offset = 15 + 4 * (((tl_type & TL_TYPE_WRITE) ? GET_TLP_LENGTH(pkt->data) : 0) + TLP_HAS_DIGEST(pkt->data) + TLP_HDR_4DW(pkt->data));
    crc[0] = pkt->data[lcrc_offset+0];
    crc[1] = pkt->data[lcrc_offset+1];
    crc[2] = pkt->data[lcrc_offset+2];
    crc[3] = pkt->data[lcrc_offset+3];
    CalcLcrc(pkt->data);

    exp_lcrc = ((uint32_t)pkt->data[lcrc_offset+0] << 24) | ((uint32_t)pkt->data[lcrc_offset+1] << 16) | ((uint32_t)pkt->data[lcrc_offset+2] << 8) | ((uint32_t)pkt->data[lcrc_offset+3]);
    got_lcrc = ((uint32_t)crc[0] << 24) | ((uint32_t)crc[1] << 16) | ((uint32_t)crc[2] << 8) | ((uint32_t)crc[3]);

    if (TLP_HAS_DIGEST(pkt->data))
    {
        unsigned ecrc_offset = lcrc_offset - 4;
        ecrc[0] = pkt->data[ecrc_offset+0];
        ecrc[1] = pkt->data[ecrc_offset+1];
        ecrc[2] = pkt->data[ecrc_offset+2];
        ecrc[3] = pkt->data[ecrc_offset+3];
        CalcEcrc(pkt->data);
        exp_ecrc = ((uint32_t)pkt->data[ecrc_offset+0] << 24) | ((uint32_t)pkt->data[ecrc_offset+1] << 16) | ((uint32_t)pkt->data[ecrc_offset+2] << 8) | ((uint32_t)pkt->data[ecrc_offset+3]);
        got_ecrc = ((uint32_t)ecrc[0] << 24) | ((uint32_t)ecrc[1] << 16) | ((uint32_t)ecrc[2] << 8) | ((uint32_t)ecrc[3]);
    }

    if (tl_td)
    {
        if (got_ecrc == exp_ecrc)
            VPrint("%s: %sTL Good ECRC (%08x)\n", prefixstr, tloffstr, got_ecrc);
        else
            VPrint("%s: %sTL %s**Bad ECRC**%s (%08x v %08x)\n", prefixstr, tloffstr, fmterrstr, fmtnormstr, got_ecrc, exp_ecrc);
    }
    else
    {
        VPrint("%s: %sTL No ECRC\n", prefixstr, tloffstr);
    }
    if (got_lcrc == exp_lcrc)
        VPrint("%s: %sDL Good LCRC (%08x)\n", prefixstr, dlloffstr, got_lcrc);
    else
        VPrint("%s: %sDL%s **Bad LCRC%s** (%08x v %08x)\n", prefixstr, dlloffstr, fmterrstr, fmtnormstr, got_lcrc, exp_lcrc);
}

// -------------------------------------------------------------------------
// DispRaw()
//
// Display raw (decoded and descrambled) lane data
//
// -------------------------------------------------------------------------

void DispRaw(const pPcieModelState_t const state, const PktData_t *linkin, const int rx)
{

    if (IsDispEnabled(state, rx, DISPRAWSYM | DISPALL))
    {
        //VPrint("==> ep=%d rx=%d conf=%03x\n", state->Endpoint, rx,state->usrconf.ActiveContDisp);
        int is_down = (rx && state->Endpoint) | (!rx && !state->Endpoint);

        VPrint("%sPCIE%s%d: %s", is_down ? fmtdnstr : fmtupstr, is_down ? "D" : "U", rx ? state->usrconf.BackNodeNum : state->thisnode, fmtnormstr);

        for (int idx = 0; idx < state->LinkWidth; idx++)
        {
            switch (linkin[idx])
            {
            case COM: VPrint("COM "); break;
            case STP: VPrint("STP "); break;
            case SDP: VPrint("SDP "); break;
            case END: VPrint("END "); break;
            case EDB: VPrint("EDB "); break;
            case PAD: VPrint("PAD "); break;
            case SKP: VPrint("SKP "); break;
            case FTS: VPrint("FTS "); break;
            case IDL: VPrint("IDL "); break;
            case RV1: VPrint("RV1 "); break;
            case RV2: VPrint("RV2 "); break;
            case RV3: VPrint("RV3 "); break;
            default:
              if (linkin[idx] & SYMCTRLBIT)
                  VPrint("??? ");
              else
                  VPrint (" %02x ", linkin[idx]);
              break;
            }
        }
        VPrint("\n");
    }
}

// -------------------------------------------------------------------------
// DispOs()
//
// Display Ordered Sets to output
//
// -------------------------------------------------------------------------

void DispOS(const pPcieModelState_t const state, const int type, const pTS_t const ts_data, const int lane, const bool rx, const int node)
{
    if (IsDispEnabled(state, rx, DISPPL | DISPALL))
    {
        char lanestr[STRBUFSIZE], linkstr[STRBUFSIZE], dirstr[STRBUFSIZE];

        int nodenum = rx ? state->usrconf.BackNodeNum : node;

        int is_down = (rx && state->Endpoint) | (!rx && !state->Endpoint);

        snprintf(dirstr,  STRBUFSIZE, "%s", is_down ? "D" : "U");

        switch (type)
        {
        case TS1_ID:
        case TS2_ID:
            snprintf(lanestr, STRBUFSIZE, "%3d", ts_data->lanenum);
            snprintf(linkstr, STRBUFSIZE, "%3d", ts_data->linknum);
            
            VPrint("%sPCIE%s%d%s %02d: PL TS%d OS Link=%s Lane=%s N_FTS=%2d DataRate=%s %s %s %s %s %s\n",
                   (is_down ? fmtdnstr : fmtupstr),
                   dirstr, nodenum, fmtnormstr,
                   lane, (type == TS1_ID) ? 1 : 2,
                   (ts_data->linknum == PAD) ? "PAD" : linkstr,
                   (ts_data->lanenum == PAD) ? "PAD" : lanestr,
                   ts_data->n_fts,
                   (ts_data->datarate == 6)  ? "GEN2+GEN1" :
                   (ts_data->datarate == 4)  ? "GEN2" :
                   (ts_data->datarate == 2)  ? "GEN1" : "GEN?",
                   (ts_data->control & 0x01) ? "AssertReset" : "",
                   (ts_data->control & 0x02) ? "DisableLink" : "",
                   (ts_data->control & 0x04) ? "Loopback"    : "",
                   (ts_data->control & 0x08) ? "NoScramble"  : "",
                   (ts_data->control & 0x10) ? "ComplianceRx"  : ""
                   );
            break;
        case IDL: VPrint("%sPCIE%s%d%s %02d: PL Electrical idle ordered set\n", is_down ? fmtdnstr : fmtupstr, dirstr, nodenum, fmtnormstr, lane); break;
        case FTS: VPrint("%sPCIE%s%d%s %02d: PL Fast training sequence ordered set\n", is_down ? fmtdnstr : fmtupstr, dirstr, nodenum, fmtnormstr, lane); break;
        case SKP: VPrint("%sPCIE%s%d%s %02d: PL Skip ordered set\n", is_down ? fmtdnstr : fmtupstr, dirstr, nodenum, fmtnormstr, lane); break;
        default:  VPrint("%sPCIE%s%d%s %02d: PL %s**Unrecognised ordered set**%s\n", is_down ? fmtdnstr : fmtupstr, dirstr, nodenum, fmtnormstr, lane, fmterrstr, fmtnormstr); break;
        }
    }
}

// -------------------------------------------------------------------------
// DispDll()
//
// Display data link data
//
// -------------------------------------------------------------------------

void DispDll(const pPcieModelState_t const state, const pPkt_t const pkt, const bool rx)
{
    char prefixstr[STRBUFSIZE];
    char offstr[STRBUFSIZE];
    bool phyen   = IsDispEnabled(state, rx, DISPPL | DISPALL);
    bool dllen   = IsDispEnabled(state, rx, DISPDL | DISPALL);
    bool is_down = (rx && state->Endpoint) | (!rx && !state->Endpoint);

    sprintf(prefixstr, "%sPCIE%s%d%s", is_down ? fmtdnstr : fmtupstr, is_down ? "D" : "U", rx ? state->usrconf.BackNodeNum : state->thisnode, fmtnormstr);

    if (phyen)
    {
        VPrint("%s: {SDP\n", prefixstr);
        VPrint("%s%s:", prefixstr, fmtdatastr);
        for (int idx = 1; idx <= 6; idx++)
        {
            VPrint(" %02x", pkt->data[idx]);
        }
        VPrint("%s", fmtnormstr);

        VPrint("\n%s: %s}\n", prefixstr, pkt->data[pkt->ByteCount-1] == EDB ? "EDB" : "END");
    }

    if (dllen)
    {
        // Check if CRC good
        PktData_t crc[2];

        crc[0] = pkt->data[5];
        crc[1] = pkt->data[6];
        CalcDllpCrc(pkt->data);

        PktData_t gotcrc = (crc[0] << 8)| crc[1];
        PktData_t expcrc = (pkt->data[5] << 8) | pkt->data[6];

        uint32_t type = pkt->data[1] & (((pkt->data[1] & 0x30) == 0x20) ? 0xff : 0xf8); // Mask VC bits

        uint32_t data = (pkt->data[2] << 16) | (pkt->data[3] << 8) | pkt->data[4];

        bool is_good_crc = (expcrc == gotcrc);

        sprintf(offstr, "%s", phyen ? "..." : "");

        switch (type & 0xf8) // Type with VC squelched
        {
        case DL_ACK:
        case DL_NAK:
            VPrint("%s: %sDL %s seq %02d\n", prefixstr, offstr, (type == DL_ACK) ? "Ack" : "Nak" , data & 0xfff);
            break;
        case DL_INITFC1_P:
        case DL_INITFC1_NP:
        case DL_INITFC1_CPL:
        case DL_INITFC2_P:
        case DL_INITFC2_NP:
        case DL_INITFC2_CPL:
            VPrint("%s: %sDL %s%s VC%d  HdrFC=%d DataFC=%d\n", prefixstr, offstr, (type & 0x80) ? "InitFC2-" : "InitFC1-",
                                                    (type & 0x30) == 0x00 ? "P   " : (type & 0x30) == 0x10 ? "NP  " : "Cpl ", type & 0x7,
                                                    (data >> 14) & 0xff,
                                                    (data & 0xfff)
                                                    );
            break;
        case DL_UPDATEFC_P:
        case DL_UPDATEFC_NP:
        case DL_UPDATEFC_CPL:
            VPrint("%s: %sDL UpdataFC-%sVC%d  HdrFC=%d DataFC=%d\n", prefixstr, offstr,
                (type & 0x30) == 0x00 ? "P   " : (type & 0x30) == 0x10 ? "NP  ": "Cpl ", type & 0x7,
                (data >> 14) & 0xff,
                (data & 0xfff)
            );
            break;
        case DL_PM_ENTER_L1:
            // All PM DLLPs match on DL_PM_ENTER_L1 with low 3 bits masked, so complete decode here
            switch (type)
            {
            case DL_PM_ENTER_L1  : VPrint("%s: %sDL PM_Enter_L1\n", prefixstr, offstr); break;
            case DL_PM_ENTER_L23 : VPrint("%s: %sDL PM_Enter_L23\n", prefixstr, offstr); break;
            case DL_PM_REQ_L0S   : VPrint("%s: %sDL PM_Active_State_Request_L0s\n", prefixstr, offstr); break;
            case DL_PM_REQ_L1    : VPrint("%s: %sDL PM_Active_State_Request_L1\n", prefixstr, offstr); break;
            case DL_PM_REQ_ACK   : VPrint("%s: %sDL PM_Request_Ack\n", prefixstr, offstr); break;
            default: VPrint("%s:*** Unknown DLLP packet type\n", prefixstr); break;
            }
            break;
        case DL_VENDOR:
            VPrint("%s: %sDL Vendor Specific DLLP\n", prefixstr, offstr);
            break;
        default:
            VPrint("%s:*** Unknown DLLP packet type\n", prefixstr);
            break;
        }

        VPrint("%s: %sDL ", prefixstr, offstr);
        if (is_good_crc)
        {
            VPrint("Good DLLP CRC (%04x)\n", gotcrc);
        }
        else
        {
            VPrint("**BAD PCIe DLLP CRC (%04x v %04x)\n", gotcrc, expcrc);
        }
    }
}

// -------------------------------------------------------------------------
// DispTl()
//
// Display data link data
//
// -------------------------------------------------------------------------

void DispTl(const pPcieModelState_t const state, const pPkt_t const pkt, const bool rx)
{
    int idx;
    char     prefixstr[STRBUFSIZE];
    char     tloffstr[STRBUFSIZE];
    char     dlloffstr[STRBUFSIZE];
    bool     phyen = IsDispEnabled(state, rx, DISPPL | DISPALL);
    bool     dllen = IsDispEnabled(state, rx, DISPDL | DISPALL);
    bool     tlen  = IsDispEnabled(state, rx, DISPTL | DISPALL);
    uint32_t data_offset;

    int is_down = (rx && state->Endpoint) | (!rx && !state->Endpoint);

    // Create marker suffix
    sprintf(prefixstr, "%sPCIE%s%d%s", is_down ? fmtdnstr : fmtupstr, is_down ? "D" : "U", rx ? state->usrconf.BackNodeNum : state->thisnode, fmtnormstr);

    if (phyen)
    {
        VPrint("%s: {STP\n", prefixstr);

        for (idx = 1; pkt->data[idx] != EDB && pkt->data[idx] != END; idx++)
        {
            if ((idx-1)%22 == 0)
                VPrint("%s:%s", prefixstr, fmtdatastr);

            VPrint(" %02x", pkt->data[idx]);

            if ((idx-1)%22 == 21)
                VPrint("%s\n", fmtnormstr);
        }
        VPrint("%s", fmtnormstr);
        VPrint("%s", !((idx-1)%22) ? "" : "\n");
        VPrint("%s: %s}\n", prefixstr, pkt->data[idx] == EDB ? "EDB" : "END");
    }

    if (tlen)
    {
        sprintf(dlloffstr, "%s", (phyen & dllen) ? "..." : "");
        sprintf(tloffstr, "%s", (phyen & dllen) ? "......" : (phyen ^ dllen) ? "..." : "");

        uint32_t dl_seq_num  =  (pkt->data[1]  << 8)  | (pkt->data[2]);
        uint32_t dllp_word   =  (pkt->data[1]  << 24) | (pkt->data[2]  << 16) | (pkt->data[3]  << 8) | (pkt->data[4]);
        uint32_t tl_word0    =  (pkt->data[3]  << 24) | (pkt->data[4]  << 16) | (pkt->data[5]  << 8) | (pkt->data[6]);
        uint32_t tl_word1    =  (pkt->data[7]  << 24) | (pkt->data[8]  << 16) | (pkt->data[9]  << 8) | (pkt->data[10]);
        uint32_t tl_word2    =  (pkt->data[11] << 24) | (pkt->data[12] << 16) | (pkt->data[13] << 8) | (pkt->data[14]);
        uint32_t tl_word3    =  (pkt->data[15] << 24) | (pkt->data[16] << 16) | (pkt->data[17] << 8) | (pkt->data[18]);

        // Word 0 header decode
        uint32_t tl_fmt      = (tl_word0 >> 29) & 0x3;
        uint32_t tl_td       = (tl_word0 >> 15) & 0x1;
        uint32_t tl_length   = tl_word0 & 0x3ff;
        uint32_t tl_type     = (tl_word0 >> 24) & 0x7f; // Includes fmt bits
        uint32_t tl_tc       = (tl_word0 >> 20) & 0x7;
        uint32_t tl_ep       = (tl_word0 >> 14) & 0x1;
        uint32_t tl_attr     = (tl_word0 >> 12) & 0x3;

        // word 1 decode for mem, i/o and config
        uint32_t tl_id       = (tl_word1 >> 16) & 0xffff;
        uint32_t tl_tag      = (tl_word1 >>  8) & 0xff;
        uint32_t tl_lbe      = (tl_word1 >>  4) & 0xf; tl_lbe = ((tl_lbe & 0x8) ? 0x1000 : 0) | ((tl_lbe & 0x4) ? 0x0100 : 0) | ((tl_lbe & 0x2) ? 0x0010 : 0) | ((tl_lbe & 0x1) ? 0x0001 : 0);
        uint32_t tl_fbe      = (tl_word1 >>  0) & 0xf; tl_fbe = ((tl_fbe & 0x8) ? 0x1000 : 0) | ((tl_fbe & 0x4) ? 0x0100 : 0) | ((tl_fbe & 0x2) ? 0x0010 : 0) | ((tl_fbe & 0x1) ? 0x0001 : 0);

        // mask route type bits for messages
        uint32_t tl_type_adj = tl_type & (((tl_type & 0x30) == 0x30) ? ~0x7 : ~0x0);

        switch (tl_type_adj) // Type with route squelched on messages
        {
        // Memory accesses
        case TL_MRD32:
        case TL_MRDLCK32:
        case TL_MRD64:
        case TL_MRDLCK64:
        case TL_MWR32:
        case TL_MWR64:

            VPrint("%s: %sDL Sequence number=%d\n", prefixstr, dlloffstr, dl_seq_num);
            VPrint("%s: %sTL Mem %s req Addr=", prefixstr, tloffstr, (tl_type & TL_TYPE_WRITE) ? "write" : "read");

            if (tl_type & TL_TYPE_ADDR64)
                VPrint("%016lx (64) ", ((uint64_t)tl_word2 << 32) | ((uint64_t)tl_word3));
            else
                VPrint("%08x (32) ", tl_word2);
            VPrint("%s=%04x TAG=%02x FBE=%04x LBE=%04x ", (tl_type & TL_TYPE_MEMLOCK) ? "LOCKED ID" : "RID", tl_id, tl_tag, tl_fbe, tl_lbe);

            if (!(tl_type & TL_TYPE_WRITE))
                VPrint("Len=%03x", tl_length);
            VPrint("\n");

            // Display traffic class and other attributes
            DispTc (prefixstr, tloffstr, tl_type, tl_tc, tl_td, tl_ep, tl_attr, tl_length);

            // If a write, display payload data
            if (tl_type & TL_TYPE_WRITE)
            {
                // Calculate start of data offset, depending on 3 or 4 DW header (plus STP and 2 byte seq number)
                data_offset = 15 + ((tl_type & TL_TYPE_ADDR64)? 4 : 0);

                DispPayload(prefixstr, tloffstr, pkt, data_offset, tl_length);
            }

            // Display LCRC and (if present) ECRC associated with the TLP
            DispTlpCrc(prefixstr, tloffstr, dlloffstr, tl_td, pkt);

            break;

        // Completions
        case TL_CPL:
        case TL_CPLD:
        case TL_CPLLK:
        case TL_CPLDLK:
        {
            uint32_t tl_cstatus   = (tl_word1 >> 13) & 0x3;
            uint32_t tl_bcm       = (tl_word1 >> 12) & 0x1;
            uint32_t tl_byte_cnt  = (tl_word1 >>  0) & 0xfff;
            uint32_t tl_crid      = (tl_word2 >> 16) & 0xffff;
            uint32_t tl_ctag      = (tl_word2 >>  8) & 0xff;
            uint32_t tl_laddr     = (tl_word2 >>  0) & 0x7f;

            VPrint("%s: %sDL Sequence number=%d\n", prefixstr, dlloffstr, dl_seq_num);
            VPrint("%s: %sTL Completion %s%s%s", prefixstr, tloffstr, (tl_type & TL_TYPE_WRITE) ? "with data " : "", (tl_type & TL_TYPE_MEMLOCK) ? "LOCKED " : "",
                      (tl_cstatus == 0) ? "Successful "              :
                      (tl_cstatus == 1) ? "Unsupported Request "     :
                      (tl_cstatus == 2) ? "Config Req Retry Status " :
                                          "Completer Abort "
                );
            VPrint("CID=%04x BCM=%d Byte Count=%03x RID=%04x TAG=%02x Lower Addr=%02x\n", tl_id, tl_bcm, tl_byte_cnt, tl_crid, tl_ctag, tl_laddr);

            // Display traffic class and other attributes
            DispTc (prefixstr, tloffstr, tl_type, tl_tc, tl_td, tl_ep, tl_attr, tl_length);

            // If a completion with payload, display payload data
            if (tl_type & TL_TYPE_WRITE)
            {
                data_offset = 15; // SDP +  2 byte Seq Num + 3 DW header

                DispPayload(prefixstr, tloffstr, pkt, data_offset, tl_length);
            }

            // Display LCRC and (if present) ECRC associated with the TLP
            DispTlpCrc(prefixstr, tloffstr, dlloffstr, tl_td, pkt);
            
            break;
        }

        // Configuration space accesses
        case TL_CFGRD0:
        case TL_CFGRD1:
        case TL_CFGWR0:
        case TL_CFGWR1:
        {
            uint32_t tl_bus  = (tl_word2 >> 24) & 0xff;
            uint32_t tl_dev  = (tl_word2 >> 20) & 0xf;
            uint32_t tl_func = (tl_word2 >> 16) & 0xf;
            uint32_t tl_ereg = (tl_word2 >>  8) & 0xf;
            uint32_t tl_reg  = (tl_word2 >>  2) & 0x3f;

            VPrint("%s: %sDL Sequence number=%d\n", prefixstr, dlloffstr, dl_seq_num);
            VPrint("%s: %sTL Config %s type %d RID=%04x TAG=%02x FBE=%04x LBE=%04x Bus=%02x Dev=%02x Func=%x EReg=%x Reg=%02x\n",
                prefixstr, tloffstr, (tl_type & TL_TYPE_WRITE) ? "write" : "read", tl_type & 1,
                tl_id, tl_tag, tl_fbe, tl_lbe, tl_bus, tl_dev, tl_func, tl_ereg, tl_reg);

            // Display traffic class and other attributes
            DispTc (prefixstr, tloffstr, tl_type, tl_tc, tl_td, tl_ep, tl_attr, tl_length);

            // If a write, display payload data
            if (tl_type & TL_TYPE_WRITE)
            {
                data_offset = 15; // SDP +  2 byte Seq Num + 3 DW header

                DispPayload(prefixstr, tloffstr, pkt, data_offset, tl_length);
            }

            // Display LCRC and (if present) ECRC associated with the TLP
            DispTlpCrc(prefixstr, tloffstr, dlloffstr, tl_td, pkt);
            break;
        }

        // Messages
        case TL_MSG:
        case TL_MSGD:

            VPrint("%s: %sDL Sequence number=%d\n", prefixstr, dlloffstr, dl_seq_num);
            VPrint("%s: %sTL Message req%s ", prefixstr, tloffstr, (tl_type & TL_TYPE_WRITE) ? " with Data" : "");

            uint32_t msg_code   = tl_word1 & 0xff;
            uint32_t vend_id    = tl_word2 & 0xffff;
            uint32_t route_type = tl_type & 0x7; 

            switch(msg_code)
            {
            case MSG_ASSERT_INTA  : VPrint("Assert INTA "); break;
            case MSG_ASSERT_INTB  : VPrint("Assert INTB "); break;
            case MSG_ASSERT_INTC  : VPrint("Assert INTC "); break;
            case MSG_ASSERT_INTD  : VPrint("Assert INTD "); break;
            case MSG_DEASSERT_INTA: VPrint("Deassert INTA "); break;
            case MSG_DEASSERT_INTB: VPrint("Deassert INTB "); break;
            case MSG_DEASSERT_INTC: VPrint("Deassert INTC "); break;
            case MSG_DEASSERT_INTD: VPrint("Deassert INTD "); break;
            case MSG_PM_ACTIVE_NAK: VPrint("PM Active State NAK "); break;
            case MSG_PM_PME       : VPrint("PM Power Management Enable "); break;
            case MSG_PME_OFF      : VPrint("PM Turn Off "); break;
            case MSG_PME_TO_ACK   : VPrint("PM TO Ack "); break;
            case MSG_ERR_COR      : VPrint("Error Correctable "); break;
            case MSG_ERR_NONFATAL : VPrint("Error Non-fatal "); break;
            case MSG_ERR_FATAL    : VPrint("Error Fatal "); break;
            case MSG_UNLOCK       : VPrint("Unlock locked transaction "); break;
            case MSG_SET_PWR_LIMIT: VPrint("Set slot power limit "); break;
            case MSG_VENDOR_0     : VPrint("Vendor type 0 "); break;
            case MSG_VENDOR_1     : VPrint("Vendor tyoe 1 "); break;
            default               : VPrint("%s**illegal Msg code**%s ", fmterrstr, fmtnormstr);
            }

            VPrint("ID=%04x TAG=%02x ", tl_id, tl_tag);

            if (msg_code == MSG_VENDOR_0 || msg_code == MSG_VENDOR_1)
            {
                VPrint("Vendor ID=%04x Vendor Data=%08x ", vend_id, tl_word3);
            }

            switch(route_type)
            {
            case MSG_ROUTE_ROOT  : VPrint("(route to root complex)\n"); break;
            case MSG_ROUTE_ADDR  : VPrint("(route by address)\n"); break;
            case MSG_ROUTE_ID    : VPrint("(route by ID = 0x%04x)\n", tl_id); break;
            case MSG_ROUTE_BCAST : VPrint("(broadcast from root complex)\n"); break;
            case MSG_ROUTE_LOCAL : VPrint("(Local)\n"); break;
            case MSG_ROUTE_GATHER: VPrint("(Gather and route to root complex)\n"); break;
            default              : VPrint("(%s**illegal route**%s)\n", fmterrstr, fmtnormstr); break;
            }

            // Display traffic class and other attributes
            DispTc (prefixstr, tloffstr, tl_type, tl_tc, tl_td, tl_ep, tl_attr, tl_length);

            // If a message with payload, display payload data
            if (tl_type & TL_TYPE_WRITE)
            {
                data_offset = 19; // SDP +  2 byte Seq Num + 4 DW header

                DispPayload(prefixstr, tloffstr, pkt, data_offset, tl_length);
            }

            // Display LCRC and (if present) ECRC associated with the TLP
            DispTlpCrc(prefixstr, tloffstr, dlloffstr, tl_td, pkt);
            break;

        // IO accesses
        case TL_IORD:
        case TL_IOWR:

            VPrint("%s: %sDL Sequence number=%d\n", prefixstr, dlloffstr, dl_seq_num);
            VPrint("%s: %sTL IO %s req Addr=", prefixstr, tloffstr, (tl_type & TL_TYPE_WRITE) ? "write" : "read");
            VPrint("%08x (32) ", tl_word2);
            VPrint("RID=%04x TAG=%02x FBE=%04x LBE=%04x\n", tl_id, tl_tag, tl_fbe, tl_lbe);

            // Display traffic class and other attributes
            DispTc (prefixstr, tloffstr, tl_type, tl_tc, tl_td, tl_ep, tl_attr, tl_length);

            // If a write, display payload data
            if (tl_type & TL_TYPE_WRITE)
            {
                // Calculate start of data offset, depending on 3 or 4 DW header (plus STP and 2 byte seq number)
                data_offset = 15 + ((tl_type & TL_TYPE_ADDR64)? 4 : 0);

                DispPayload(prefixstr, tloffstr, pkt, data_offset, tl_length);
            }

            // Display LCRC and (if present) ECRC associated with the TLP
            DispTlpCrc(prefixstr, tloffstr, dlloffstr, tl_td, pkt);
            break;
        }
    }
}

