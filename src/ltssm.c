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
// $Id: ltssm.c,v 1.2 2016/10/10 13:09:13 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/ltssm.c,v $
//
//=============================================================
//
// Implements a PCIe LTSSM function. NB. IT IS NOT COMPLETE,
// and is meant only to be able to power up a link to L0. With 
// LTSSM_ABBREVIATED defined, sequence is shortened in various
// places, and timeouts reduced.
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include "ltssm.h"
#include "pcie.h"
#include "pci_express.h"

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

// Assume a clock rate of 500MHz
#define CLK_CYCLE_NS                 2

#define CYCLES_1US                   (1000 / CLK_CYCLE_NS)
#define CYCLES_1MS                   (1000 * CYCLES_1US)
#define CYCLES_12MS                  (12   * CYCLES_1MS)

#define DEFAULT_N_FTS                4
#define DEFAULT_TS_CTL               0
#define DEFAULT_LINKNUM              0
#define ENABLE_LANENUMS              0

#define TS_CTL_HOT_RESET             0x01
#define TS_CTL_DISABLE               0x02
#define TS_CTL_LOOPBACK              0x04
#define TS_CTL_NO_SCRAMBLE           0x08

#define LTSSM_DETECT                 0
#define LTSSM_POLLING                1
#define LTSSM_CONFIG                 2
#define LTSSM_RECOVERY               3
#define LTSSM_DISABLED               4
#define LTSSM_HOTRESET               5
#define LTSSM_LOOPBACK               6
#define LTSSM_L0                     7
#define LTSSM_L0s                    8
#define LTSSM_L1                     9
#define LTSSM_L2                     10

#ifdef LTSSM_ABBREVIATED
#define PCIE_POLLING_ACTIVE_TX_COUNT 16
#else
#define PCIE_POLLING_ACTIVE_TX_COUNT 1024
#endif

#ifdef LTSSM_ABBREVIATED
#define DEFAULT_DETECT_QUIET_TIMEOUT 1500
#else
#define DEFAULT_DETECT_QUIET_TIMEOUT CYCLES_12MS
#endif

#define DEFAULT_MAX_LINK_WIDTH       16
#define DEFAULT_MAX_LINK_WIDTH_MASK  ((1 << DEFAULT_MAX_LINK_WIDTH) - 1)

#define DEFAULT_ENABLED_TESTS        0
#define DEFAULT_FORCE_TESTS          0 

#define LTSSM_SET_MINIMUM            0

// -------------------------------------------------------------------------
// STATICS
// -------------------------------------------------------------------------

// The array initialisations are quite gcc oriented, and may not be portable.
// However, there are other things restricting the code to linux, so use for now.

static int  ltssm_linknum         [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_LINKNUM};
static int  ltssm_ts_ctl          [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_TS_CTL};
static int  ltssm_n_fts           [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_N_FTS };
static int  ltssm_max_link_width  [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_MAX_LINK_WIDTH};
static int  ltssm_max_link_mask   [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_MAX_LINK_WIDTH_MASK};
static int  ltssm_detect_quiet_to [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_DETECT_QUIET_TIMEOUT};
static int  ltssm_enable_tests    [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_ENABLED_TESTS};
static int  ltssm_force_tests     [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = DEFAULT_FORCE_TESTS};

static int  ltssm_tx_n_fts        [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = 0};

static bool config_disable        [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = false};
static bool config_loopback       [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = false};
static bool polling_compliance    [VP_MAX_NODES] = { [0 ... VP_MAX_NODES-1] = false};

// -------------------------------------------------------------------------
// Detect()
// -------------------------------------------------------------------------

static int Detect (const int link_width, const int node)
{
    int    i = 0;
    uint32_t rcvr_idle_status;

    ltssm_max_link_width[node] = link_width;
    ltssm_max_link_mask[node]  = ((1 << ltssm_max_link_width[node])-1) & 0xffff;

    // Quiet
    VPrint("---> Detect Quiet (node %d)\n", node);

    // Loop until rcvr_idle_status indicates at least one lane not idle
    do 
    {
        SendIdle(1, node);
        VRead(LINK_STATE, &rcvr_idle_status, 1, node);
        DebugVPrint ("---> i=%d node=%d ltssm_detect_quiet_to[node]=%d rcvr_idle_status=0x%08x ltssm_max_link_mask[node]=0x%08x\n",
                i, node, ltssm_detect_quiet_to[node], rcvr_idle_status, ltssm_max_link_mask[node]);
    } while ((++i < ltssm_detect_quiet_to[node]) && ((rcvr_idle_status & ltssm_max_link_mask[node]) == ltssm_max_link_mask[node]));

    // Active (If no rcvr detect, assume all 16 lanes are present)
    VPrint("---> Detect Active (node %d)\n", node);
    VRead(LINK_STATE, &rcvr_idle_status, 1, node);
    VPrint("---> rcvr_idle_status = %x (node %d)\n", rcvr_idle_status & ltssm_max_link_mask[node], node);

    // Exit to polling
    return LTSSM_POLLING;
}

// -------------------------------------------------------------------------
// Polling()
// -------------------------------------------------------------------------

static int Polling(int *active_lanes, const int node)
{
    uint32_t ts1_count[MAX_LINK_WIDTH], ts2_count[MAX_LINK_WIDTH];
    TS_t ts_status;
    int i;

    // Assumes here that what is on lane 0 is common to all lanes

    // Clear TS  rx state
    ResetEventCount(TS1_ID, node);
    ResetEventCount(TS2_ID, node);

    // --- force compliance ---
    if (polling_compliance[node] == false && ((ltssm_force_tests[node] & ENABLE_COMPLIANCE) || ((ltssm_enable_tests[node] & ENABLE_COMPLIANCE) && ((PcieRand(node) % 3) == 0))))
    {
        VPrint("---> Polling Compliance (node %d)\n", node);
        polling_compliance[node] = true;
        VWrite(LINK_STATE, (1 << (PcieRand(node) % ltssm_max_link_width[node])) | ~ltssm_max_link_mask[node], 1, node);

        // This is a very nasty hack, of which I am appropriately ashamed.
        // It is an open loop delay long enough for endpoint to timeout in
        // polling.active and go to compliance for a while, before we
        // continue to polling.active.
        SendIdle((28000)/4, node);
    }

    // --- Active ---
    VPrint("---> Polling Active (node %d)\n", node);
    i = 0;
    VWrite(LINK_STATE, (~ltssm_max_link_mask[node]) & 0xffff, 1, node);
    do 
    {
        SendTs(TS1_ID, PAD, PAD, ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        ReadEventCount(TS1_ID, ts1_count, node);
        ReadEventCount(TS2_ID, ts2_count, node);
        ts_status = GetTS(0, node);
        if (ts1_count[0] || ts2_count[0] || i)
        {
            i++;
        }
        if ((ts1_count[0] || ts2_count[0]) && (ts_status.linknum != PAD || ts_status.lanenum != PAD)) 
        {
            ts1_count[0] = ts2_count[0] = 0;
        }
       
    } while(((ts1_count[0] < 8) && (ts2_count[0] < 8)) || (i < PCIE_POLLING_ACTIVE_TX_COUNT));

    // --- Config ---
    VPrint("---> Polling Config (node %d)\n", node);
    i = 0;
    ResetEventCount(TS2_ID, node);
    do
    {
        SendTs(TS2_ID, PAD, PAD, ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        ReadEventCount(TS2_ID, ts2_count, node);
        ts_status = GetTS(0, node);
        if (ts2_count[0] || i)
        {
            i++;
        }
        if (ts2_count[0] && (ts_status.linknum != PAD || ts_status.lanenum != PAD))
        {
            ts2_count[0] = 0;
            ResetEventCount(TS2_ID, node);
        }
    } while((ts2_count[0] < 8) || (i < 16));

    *active_lanes = 0;
    for (i = 0; i < MAX_LINK_WIDTH; i++)
    {
        *active_lanes |= (ts2_count[i] ? 1 : 0) << i;
    }
    VPrint("---> Active lanes = 0x%04x (node %d)\n", *active_lanes, node);

    // Exit to configuration
    return LTSSM_CONFIG;
}

// -------------------------------------------------------------------------
// Configuration()
// -------------------------------------------------------------------------

static int Configuration(const int active_lanes, const int target_state, const int node)
{
    uint32_t ts1_count[MAX_LINK_WIDTH], ts2_count[MAX_LINK_WIDTH];
    int i, lnkwidth;
    TS_t ts_status;

    lnkwidth = (active_lanes & 0x8000) ? 16 :
               (active_lanes & 0x80)   ? 8  :
               (active_lanes & 0x8)    ? 4  :
               (active_lanes & 0x2)    ? 2  :
                                         1;

    // Linkwidth.Start
    VPrint("---> Configuration Start (node %d)\n", node);

    // If not done so before, randomly choose to go to disabled state
    if (config_disable[node] == false && ((ltssm_force_tests[node] & ENABLE_DISABLE) || ((ltssm_enable_tests[node] & ENABLE_DISABLE) && ((PcieRand(node) % 3) == 0))))
    {
        config_disable[node] = true;
        VPrint("---> Going to Disabled from Configuration Start (node %d)\n", node);
        return LTSSM_DISABLED;
    }

    // If not done so before, randomly choose to go to loopback state
    if (config_loopback[node] == false && ((ltssm_force_tests[node] & ENABLE_LOOPBACK) || ((ltssm_enable_tests[node] & ENABLE_LOOPBACK) && ((PcieRand(node) % 3) == 0))))
    {
        config_loopback[node] = true;
        VPrint("---> Going to Loopback from Configuration Start (node %d)\n", node);
        return LTSSM_LOOPBACK;
    }

    ResetEventCount(TS1_ID, node);
    do
    {
        SendTs(TS1_ID, PAD, ltssm_linknum[node], ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        ReadEventCount(TS1_ID, ts1_count, node);
        ts_status = GetTS(0, node);

        if (ts1_count[0] && ts_status.linknum == PAD)
        {
            ts1_count[0] = 0;
            ResetEventCount(TS1_ID, node);
        }
        
    } while(ts1_count[0] < 2 || ts_status.linknum != ltssm_linknum[node]);

    // Linkwidth.Accept (fall through state)
    VPrint("---> Configuration Linkwidth Accept (node %d)\n", node);

    // Lanenum.Wait
    VPrint("---> Configuration Lanenum Wait (node %d)\n", node);
    ResetEventCount(TS1_ID, node);
    do
    {
        SendTs(TS1_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        for (i=0; i < lnkwidth; i++)
        {
            ts_status = GetTS(i, node);
            ReadEventCount(TS1_ID, ts1_count, node);
            if (ts_status.linknum != ltssm_linknum[node] || ts_status.lanenum != i)
            {
                ts1_count[0] = 0;
                ResetEventCount(TS1_ID, node);
            }
        }
    } while (ts1_count[0] < 2);

    // Lanenum.Accept
    VPrint("---> Configuration Lanenum Accept (node %d)\n", node);
    ResetEventCount(TS1_ID, node);
    do
    {
        SendTs(TS1_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        for (i=0; i < lnkwidth; i++)
        {
            ts_status = GetTS(i, node);
            ReadEventCount(TS1_ID, ts1_count, node);
            if (ts_status.linknum != ltssm_linknum[node] && ts_status.lanenum != i)
            {
                ts1_count[i] = 0;
                ResetEventCount(TS1_ID, node);
            }
        }
    } while (ts1_count[0] < 2);

    // Complete
    VPrint("---> Configuration Complete (node %d)\n", node);
    ResetEventCount(TS2_ID, node);
    do
    {
        SendTs(TS2_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        for (i=0; i < lnkwidth; i++)
        {
            ts_status = GetTS(i, node);
            ReadEventCount(TS2_ID, ts2_count, node);
            if (ts_status.linknum != ltssm_linknum[node] && ts_status.lanenum != i) 
            {
                ts2_count[0] = 0;
            }
        }
    } while(ts2_count[0] < 8);

    ltssm_tx_n_fts[node] = ts_status.n_fts;

    // Idle
    VPrint("---> Configuration Idle (node %d)\n", node);
    i = 0;
    ResetEventCount(0, node);
    do
    {
        SendIdle(1, node);
        ReadEventCount(0, ts2_count, node);

        if (ts2_count[0])
        {
            i++;
        }
        DebugVPrint("--->i = %d ts2_count[0] = %d (node %d)\n", i, ts2_count[0], node);
    } while(i < 16 || ts2_count[0] < 8);

    VPrint("---> Configuration exit to L0 (node %d)\n", node);

    // Exit to L0
    return LTSSM_L0;
}

// -------------------------------------------------------------------------
// TxL0s()
// -------------------------------------------------------------------------

static int TxL0s (const int target_state, const int active_lanes, const int ticks, const int node)
{
    int i;

    // ---------------
    VPrint("---> TxL0s Entry (node %d)\n", node);

    // Inform model that the transmitter is down (and thus queue their data)
    SetTxDisabled(node);

    SendOs(IDL, node);
    // Shut down the lanes
    VWrite(LINK_STATE, 0xffff, 1, node);

    // ---------------
    VPrint("---> TxL0s Idle: sleeping for %d ticks (node %d)\n", ticks, node);
    SendIdle(ticks, node);

    // ---------------
    VPrint("---> TxL0s FTS (node %d)\n", node);
    VWrite(LINK_STATE, ~(active_lanes & ltssm_max_link_mask[node]) & 0xffff, 1, node);
    for (i = 0; i < ltssm_tx_n_fts[node]; i++)
    {
        SendOs(FTS, node);
    }

    SendOs(SKP, node);

    // The transmitter is available once again
    SetTxEnabled(node);

    return LTSSM_L0;
}

// -------------------------------------------------------------------------
// Recovery()
// -------------------------------------------------------------------------

static int Recovery (const int target_state, const int node)
{
    uint32_t ts1_count[MAX_LINK_WIDTH], ts2_count[MAX_LINK_WIDTH], idl_count[MAX_LINK_WIDTH];
    int i, change_config = false;
    TS_t ts_status;

    // --- RcvrLock ---
    VPrint("---> Recovery Lock (node %d)\n", node);
    // Clear TS  rx state
    ResetEventCount(TS1_ID, node);
    ResetEventCount(TS2_ID, node);

    // One in 16 chance of altering the configuration
    if (1 || (PcieRand(node) & 0xf) == 0)
    {
        change_config = true;
    }

    // Send TS1 ordered sets (no speed change for now) and look for TS1 or TS2 training sequences.
    // Exit when seen at least 8
    do
    {
        SendTs(TS1_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        ReadEventCount(TS1_ID, ts1_count, node);
        ReadEventCount(TS2_ID, ts2_count, node);
    } while((ts1_count[0] < 8) && (ts2_count[0] < 8));

    if (change_config)
    {
        ltssm_n_fts[node] =  PcieRand(node)%252 + 4; // at least 4
    }

    // --- RcvrCfg ---
    // Clear TS  rx state
    VPrint("---> Recovery RcvrCfg (node %d)\n", node);
    ResetEventCount(IDL, node);
    ResetEventCount(TS2_ID, node);

    // Send TS2 ordered sets and look for TS2 training sequemces.
    // Exit when 8 consecutive TS's received, and at least 16 have been
    // sent after first sequence without interruption from an EIOS.
    i = 0;
    do
    {
        SendTs(TS2_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], ltssm_ts_ctl[VP_MAX_NODES], false, node);
        ReadEventCount(TS2_ID, ts2_count, node);
        ReadEventCount(IDL, idl_count, node);
        if (idl_count[0] || (ts2_count[0] == 0))
        {
            i = 0;
            ResetEventCount(IDL, node);
            ResetEventCount(TS2_ID, node);
        }
        else
        {
            i++;
        }

    } while((ts2_count[0] < 8) || (i < 16));

    ts_status = GetTS(0, node);
    ltssm_tx_n_fts[node] = ts_status.n_fts;

    // If we're updating
    if (0 && change_config)
    {
        VPrint("---> Leaving Recovery (node %d)\n", node);
        return LTSSM_CONFIG;
    }

    // --- Idle ---
    VPrint("---> Recovery Idle (node %d)\n", node);
    i = 0;
    ResetEventCount(0, node);
    do
    {
        SendIdle(1, node);
        ReadEventCount(0, ts2_count, node);

        if (ts2_count[0]) 
        {
            i++;
        }
    } while(i < 16 || ts2_count[0] < 8);

    VPrint("---> Leaving Recovery (node %d)\n", node);
    // Exit to L0
    return LTSSM_L0;
}

// -------------------------------------------------------------------------
// Disabled()
// -------------------------------------------------------------------------

static int Disabled (const int node)
{
    int i, rand_idle;
    uint32_t idl_count[MAX_LINK_WIDTH];

    VPrint("---> Disabled (node %d)\n", node);

    ResetEventCount(IDL, node);
    // Transmit 16 TS1 OS's with disabled set
    for (i=0; i < 16; i++)
    {
       SendTs(TS1_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], TS_CNTL_DISABLE_LINK, false, node); 
    }

    // Tx EIOS
    SendOs(IDL, node);

    // Shut down the lanes
    VWrite(LINK_STATE, 0xffff, 1, node);

    // Wait for an EIOS from unit
    do
    {
        ReadEventCount(IDL, idl_count, node);
        DebugVPrint("---> idl_count[0] = %d (node %d)\n", idl_count[0], node);
        SendIdle(1, node);
    } while (!idl_count[0]);

    //Send a random amount of idles (i.e. wait for a set time---this link is electrically idle)
    //rand_idle = (PcieRand(node) % 1000) + 25;
    rand_idle = 100;

    VPrint("---> Waiting for %d ticks (node %d)\n", rand_idle, node);
    SendIdle(rand_idle, node);

    VPrint("---> Leaving Disabled for Detect (node %d)\n", node);

    return LTSSM_DETECT;
}

// -------------------------------------------------------------------------
// Loopback()
// -------------------------------------------------------------------------

static int Loopback (const int node)
{
    int i, rand_idle;
    TS_t ts_status;
    uint32_t count[MAX_LINK_WIDTH];

    VPrint("---> Loopback (node %d)\n", node);

    // ---- Loopback.Entry ----

    ResetEventCount(TS1_ID, node);

    // Transmit TS1s OS's with loopback set until a TS1 with loopback set is received
    do
    {
        SendTs(TS1_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], TS_CTL_LOOPBACK, false, node); 
        ReadEventCount(TS1_ID, count, node);
        ts_status = GetTS(0, node);
        VPrint("count[0] = %x ts_status.control = %x\n", count[0], ts_status.control);
    } while (count[0] == 0 || !(ts_status.control & TS_CNTL_LOOPBACK));

    // ---- Loopback.Active ----
    VPrint("---> Loopback.Active (node %d)\n", node);

    // Stay in Loopback.Active for a while
    for (i = 0; i < 64; i++)
    {
        SendTs(TS1_ID, ENABLE_LANENUMS, ltssm_linknum[node], ltssm_n_fts[node], TS_CTL_LOOPBACK, false, node); 
    }

    // ---- Loopback.Exit ----

    VPrint("---> Loopback.Exit (node %d)\n", node);
    ResetEventCount(IDL, node);

    // Send and Electrical Idle
    SendOs(IDL, node);

    // Shut down the lanes
    VWrite(LINK_STATE, 0xffff, 1, node);
    
    // Wait for the idle to be returned
    do
    {
        ReadEventCount(IDL, count, node);
        DebugVPrint("---> count[0] = %d (node %d)\n", count[0], node);
        SendIdle(1, node);
    } while (!count[0]);

    //Send a random amount of idles (i.e. wait for a set time---this link is electrically idle)
    //rand_idle = (PcieRand(node) % 1000) + 25;
    rand_idle = 1000;

    VPrint("---> Waiting for %d ticks (node %d)\n", rand_idle, node);
    SendIdle(rand_idle, node);

    VPrint("---> Leaving Loopback for Detect (node %d)\n", node);

    return LTSSM_DETECT;
}

// -------------------------------------------------------------------------
// HotReset()
// -------------------------------------------------------------------------

static int HotReset (const int HotResetTO, const int node)
{
    int loops = HotResetTO;
    int i;

    if (loops < 2)
    {
       loops = 2;
    }

    for (i=0; i < loops; i++) 
    {
        SendTs(TS1_ID, 0, ltssm_linknum[node], ltssm_n_fts[node], TS_CNTL_HOT_RESET, false, node);
    }

    return LTSSM_DETECT;
}

// -------------------------------------------------------------------------
// L1()
// -------------------------------------------------------------------------

static int L1 (const int time_in_l1_param, const int node)
{
    int time_in_l1 = time_in_l1_param;

    if (time_in_l1 < 30)
    {
        time_in_l1 = 30;
    }

    // Shut down the lanes
    VWrite(LINK_STATE, 0xffff, 1, node);

    // Stay in L1 state for a minimum amount of time
    SendIdle(time_in_l1, node);

    return LTSSM_RECOVERY;
}

// -------------------------------------------------------------------------
// L2()
// -------------------------------------------------------------------------

static int L2 (const int time_in_l2_param, const int node)
{
    int time_in_l2 = time_in_l2_param;

    if (time_in_l2 < 10)
    {
        time_in_l2 = 10;
    }

    // Shut down the lanes
    VWrite(LINK_STATE, 0xffff, 1, node);

    // Stay in L2 state for a minimum amount of time
    SendIdle(time_in_l2, node);

    return LTSSM_DETECT;
}

// -------------------------------------------------------------------------
// LinkState()
//
// Top level LTSSM state machine function
//
// -------------------------------------------------------------------------

static int LinkState (const int ltssm_state, const int target_state, const int link_width, const int node)
{
    int active_lanes = 0;

    switch (ltssm_state) {
    case LTSSM_DETECT:
        return Detect(link_width, node);
        break;

    case LTSSM_POLLING:
        return Polling(&active_lanes, node);
        break;

    case LTSSM_CONFIG:
        return Configuration(active_lanes, target_state, node);
        break;

    case LTSSM_DISABLED:
        return Disabled(node);
        break;

    case LTSSM_LOOPBACK:
        return Loopback(node);
        break;

    case LTSSM_HOTRESET:
        return HotReset(LTSSM_SET_MINIMUM, node);
        break;

    case LTSSM_L1:
        return L1(LTSSM_SET_MINIMUM, node);
        break;

    case LTSSM_L2:
        return L2(LTSSM_SET_MINIMUM, node);
        break;

    case LTSSM_RECOVERY:
        return Recovery(target_state, node);
        break;

    case LTSSM_L0s:
    default:
        VPrint ("***Error: InitLink() reached unsupported or invalid LTSSM state (%d)\n", ltssm_state);
        VWrite(PVH_FATAL, 0, 0, node);
        break;
    }

    return ltssm_state;
}
    
// -------------------------------------------------------------------------
// InitLink()
// 
// Exported user function to initiate a PCIe link initialisation.
//
// -------------------------------------------------------------------------

void InitLink(const int link_width, const int node)
{
    int ltssm_state = LTSSM_DETECT;

    do
    {
        ltssm_state = LinkState(ltssm_state, LTSSM_L0, link_width, node);
    } while (ltssm_state != LTSSM_L0);
}

// -------------------------------------------------------------------------
// ConfigLinkInit
//
// Unless specified as a 'no change', update the configurations for this
// mode with limiting masks to avoid bad values.
//
// -------------------------------------------------------------------------

void ConfigLinkInit (const ConfigLinkInit_t cfg, const int node)
{
    ltssm_linknum[node]          = (cfg.ltssm_linknum         == LINK_INIT_NO_CHANGE) ? ltssm_linknum[node]         : cfg.ltssm_linknum         & 0xff;
    ltssm_n_fts[node]            = (cfg.ltssm_n_fts           == LINK_INIT_NO_CHANGE) ? ltssm_n_fts[node]           : cfg.ltssm_n_fts           & 0xff;
    ltssm_ts_ctl[node]           = (cfg.ltssm_ts_ctl          == LINK_INIT_NO_CHANGE) ? ltssm_ts_ctl[node]          : cfg.ltssm_ts_ctl          & 0x1f;
    ltssm_detect_quiet_to[node]  = (cfg.ltssm_detect_quiet_to == LINK_INIT_NO_CHANGE) ? ltssm_detect_quiet_to[node] : cfg.ltssm_detect_quiet_to;
    ltssm_enable_tests[node]     = (cfg.ltssm_enable_tests    == LINK_INIT_NO_CHANGE) ? ltssm_enable_tests[node]    : cfg.ltssm_enable_tests;
    ltssm_force_tests[node]      = (cfg.ltssm_force_tests     == LINK_INIT_NO_CHANGE) ? ltssm_force_tests[node]     : cfg.ltssm_force_tests;
}
