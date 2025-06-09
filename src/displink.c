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

// -------------------------------------------------------------------------
// IsDispEnabled()
//
// Returns true if okay to do software display
//
// -------------------------------------------------------------------------

static bool IsDispEnabled(const pPcieModelState_t const state, const int rx, int flags)
{
    return (rx || (!rx && (state->usrconf.ActiveContDisp & DISPSWTX))) &&
                  ((state->Endpoint && !(state->usrconf.ActiveContDisp & DISPSWDISEP)) || (!state->Endpoint && !(state->usrconf.ActiveContDisp & DISPSWDISRC))) &&
                  (state->usrconf.ActiveContDisp & flags);
}

// -------------------------------------------------------------------------
// ContDisps()
//
// Read ContDisps.hex file and load in to this node's state
//
// -------------------------------------------------------------------------

void ConstDisp (pUserConfig_t usrconf)
{
    FILE* fp = fopen("hex/ContDisps.hex", "r");
    char buf [STRBUFSIZE];

    int dispidx = 0;

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
            sscanf(&buf[sidx], "%x %lld", &usrconf->contdisp[dispidx].control, &usrconf->contdisp[dispidx].time);

            if (++dispidx == MAXCONSTDISP)
            {
                break;
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

void CheckContDisp (pUserConfig_t usrconf, const int node)
{
    uint32_t cycleCount = GetCycleCount(node);

    // If current time is at, or beyond, the indexed ContDisp...
    if (usrconf->ContDispIdx < MAXCONSTDISP && GetCycleCount(node) >= usrconf->contdisp[usrconf->ContDispIdx].time)
    {
        // Update active control with next value
        usrconf->ActiveContDisp = usrconf->contdisp[usrconf->ContDispIdx].control;

        // If simulatoincontrol bits set, action the control
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
// DispRaw()
//
// Display raw (decoded and descrambled) lane data
//
// -------------------------------------------------------------------------

void DispRaw(const pPcieModelState_t const state, const unsigned *linkin, const int rx)
{

    if (IsDispEnabled(state, rx, DISPRAWSYM | DISPALL))
    {
        VPrint("# PCIE%s%d: ", (rx && state->Endpoint) | (!rx && !state->Endpoint) ? "D" : "U", rx ? state->usrconf.BackNodeNum : state->thisnode);

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
// DispTs()
//
// Display Training Sequence to output
//
// -------------------------------------------------------------------------

void DispTS(const pPcieModelState_t const state, const int ts_type, const pTS_t const ts_data, const int lane, const bool rx, const int node)
{
    if (IsDispEnabled(state, rx, DISPPL | DISPALL))
    {
        char lanestr[STRBUFSIZE], linkstr[STRBUFSIZE], dirstr[STRBUFSIZE];

        int nodenum = rx ? state->usrconf.BackNodeNum : node;

        snprintf(dirstr,  STRBUFSIZE, "%s", (rx && state->Endpoint) | (!rx && !state->Endpoint) ? "D" : "U");

        snprintf(lanestr, STRBUFSIZE, "%3d", ts_data->lanenum);
        snprintf(linkstr, STRBUFSIZE, "%3d", ts_data->linknum);

        VPrint("# PCIE%s%d %02d: PL TS%d OS Link=%s Lane=%s N_FTS=%3d DataRate=%s %s %s %s %s\n",
               dirstr, nodenum,  lane, ts_type,
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
    }
}

// -------------------------------------------------------------------------
// DispDll()
//
// Display data link data
//
// -------------------------------------------------------------------------

void DispDll(const pPcieModelState_t const state, const pPkt_t const pkt, const bool rx, const int node)
{
    char str[STRBUFSIZE];
    char offstr[STRBUFSIZE];
    bool phyen = IsDispEnabled(state, rx, DISPPL | DISPALL);
    bool dllen = IsDispEnabled(state, rx, DISPDL | DISPALL);

    sprintf(str, "%s%d", (rx && state->Endpoint) | (!rx && !state->Endpoint) ? "D" : "U", rx ? state->usrconf.BackNodeNum : state->thisnode);

    if (phyen)
    {
        VPrint("# PCIE%s: {SDP\n", str);
        VPrint("# PCIE%s:", str);
        for (int idx = 1; idx <= 6; idx++)
        {
            VPrint(" %02x", pkt->data[idx]);
        }

        VPrint("\n# PCIE%s: %s}\n", str, pkt->data[pkt->ByteCount-1] == EDB ? "EDB" : "END");
    }

    if (dllen)
    {
        sprintf(offstr, "%s", phyen ? "..." : "");
    }
}

// -------------------------------------------------------------------------
// DispTl()
//
// Display data link data
//
// -------------------------------------------------------------------------

void DispTl(const pPcieModelState_t const state, const pPkt_t const pkt, const bool rx, const int node)
{
    int idx;
    char str[STRBUFSIZE];
    char offstr[STRBUFSIZE];
    bool phyen = IsDispEnabled(state, rx, DISPPL | DISPALL);
    bool dllen = IsDispEnabled(state, rx, DISPDL | DISPALL);
    bool tlen  = IsDispEnabled(state, rx, DISPTL | DISPALL);

    // Create marker suffix
    sprintf(str, "%s%d", (rx && state->Endpoint) | (!rx && !state->Endpoint) ? "D" : "U", rx ? state->usrconf.BackNodeNum : state->thisnode);

    if (phyen)
    {
        VPrint("# PCIE%s: {STP\n", str);

        for (idx = 1; pkt->data[idx] != EDB && pkt->data[idx] != END; idx++)
        {
            if ((idx-1)%22 == 0)
                VPrint("# PCIE%s:", str);
            VPrint(" %02x", pkt->data[idx]);
            if ((idx-1)%22 == 21)
                VPrint("\n");
        }

        VPrint("\n# PCIE%s: %s}\n", str, pkt->data[idx] == EDB ? "EDB" : "END");
    }

    if (tlen)
    {
        sprintf(offstr, "%s", (phyen & dllen) ? "......" : (phyen ^ dllen) ? "..." : "");
    }


}

