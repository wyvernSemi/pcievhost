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
// $Id: pcie_utils.h,v 1.5 2016/10/17 11:47:01 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/pcie_utils.h,v $
//
//=============================================================

#ifndef _PCIE_UTILS_H_
#define _PCIE_UTILS_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pci_express.h"
#include "pcie_vhost_map.h"

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

#define NUM_VIRTUAL_CHANNELS         1

#define MAX_DLLP_BYTES               9

#define BIT0MASK                     0x01
#define BIT1MASK                     0x02
#define BIT2MASK                     0x04
#define BIT3MASK                     0x08
#define BIT4MASK                     0x10
#define BIT5MASK                     0x20
#define BIT6MASK                     0x40
#define BIT7MASK                     0x80

#define USE_DEFAULT                  -1

#define LO_NIBBLE_MASK               0x0f
#define HI_NIBBLE_MASK               0xf0

#define MASK_4K_BITS                 0xfffULL

#define MAX_LINE_SIZE                1024
#define MAX_BYTE_BLOCK               4096

#define PENT_BASEADDR                0x0ULL


// -------------------------------------------------------------------------
// MACROS
// -------------------------------------------------------------------------

#define CharToHex(_x) (((_x) >= '0' && (_x) <= '9') ? ((_x) - '0') : ((_x) >= 'a' && (_x) <= 'f') ? ((_x) - 'a') : ((_x) - 'A'))

#define PcieOddParity(_X)\
           (ByteParity[((_X) >>  0) & 0xff] ^ \
            ByteParity[((_X) >>  8) & 0xff] ^ \
            ByteParity[((_X) >> 16) & 0xff] ^ \
            ByteParity[((_X) >> 24) & 0xff])

#define PARITY_ARRAY_INIT {                             \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,        \
 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,        \
}

#define PRN_CHECKTAB_INIT {                             \
    0xf7011641, 0x19033ac3, 0xc50763c7, 0x8a0ec78e,     \
    0xe31c995d, 0x313824fb, 0x627049f6, 0x33e185ad,     \
    0x90c21d1b, 0x21843a36, 0xb409622d, 0x9f13d21b,     \
    0xc926b277, 0x924d64ee, 0x249ac9dc, 0x493593b8,     \
    0x656a3131, 0xcad46262, 0x95a8c4c4, 0x2b518988,     \
    0x56a31310, 0xad462620, 0xad8d5a01, 0xac1ba243,     \
    0x58374486, 0xb06e890c, 0x97dc0459, 0x2fb808b2,     \
    0x5f701164, 0xbee022c8, 0x7dc04590, 0xfb808b20,     \
}

#define FC_DATA_CHK 0
#define FC_HDR_CHK  1

// -------------------------------------------------------------------------
// TYPEDEFS
// -------------------------------------------------------------------------

////////////////////////
// Ordered set and Training sequence reception state
typedef struct {
    uint32_t     IdleCount    [MAX_LINK_WIDTH];
    uint32_t     SkipCount    [MAX_LINK_WIDTH];
    uint32_t     FtsCount     [MAX_LINK_WIDTH];
    uint32_t     Ts1Count     [MAX_LINK_WIDTH];
    uint32_t     Ts2Count     [MAX_LINK_WIDTH];

    // Received training sequence data
    TS_t       LastTS       [MAX_LINK_WIDTH];

    // Input ordered Set state
    int        OsState      [MAX_LINK_WIDTH];
    int        OsCount      [MAX_LINK_WIDTH];
    int        FlaggedIdle  [MAX_LINK_WIDTH];
    TS_t       Tseq         [MAX_LINK_WIDTH];
} LinkEventCount_t, *pLinkEventCount_t;

////////////////////////
// User configurable state. 
typedef struct {
    uint32_t     HdrConsumptionRate;
    uint32_t     DataConsumptionRate;
    int        AckRate;
    int        CompletionRate;
    int        CompletionSpread;
    int        SkipInterval;
    int        DisableMem;
    int        DisableAck;
    int        DisableFc;
    int        DisableSkips;
    int        DisableUrCpl;

    // Rx buffer sizes
    uint32_t     InitFcDataCr        [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     InitFcHdrCr         [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];

} UserConfig_t, *pUserConfig_t;

////////////////////////
// Flow control state
typedef struct {
    // Received Flow control state
    uint32_t     FlowCntlHdrCredits  [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     FlowCntlDataCredits [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     TxHdrCredits        [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     TxDataCredits       [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];

    // Transmit flow control
    uint32_t     ConsumedDataCredits   [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     ConsumedHdrCredits    [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     AdvertisedDataCredits [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     AdvertisedHdrCredits  [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     ConsumedDataUpdated   [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     ConsumedHdrUpdated    [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     RxHdrCredits          [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     RxDataCredits         [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];
    uint32_t     LastSentFcTime        [NUM_VIRTUAL_CHANNELS][FC_NUMTYPES];

    uint32_t     fc_state              [NUM_VIRTUAL_CHANNELS];
    uint32_t     rx_fc_state           [NUM_VIRTUAL_CHANNELS];

} FlowControl_t, *pFlowControl_t;

////////////////////////
// PCIe model node state
typedef struct {
    // Node identifier for this structure
    int        thisnode;

    // Configuration state set by verilog parameters
    int        LinkWidth;
    int        Endpoint;
    uint32_t     RandNum;

    // 'real' time (cycle count)
    uint32_t     TicksSinceReset;

    // Send queue pointers
    pPkt_t     head_p;
    pPkt_t     send_p;
    pPkt_t     end_p;

    // Completion delay queue pointers
    pPkt_t     cpl_head_p;
    pPkt_t     cpl_end_p;

    // AckNak state
    pPkt_t     ack_to_send_p;
    pPkt_t     nak_to_send_p;
    int        curr_ack;
    int        curr_nak;
    uint32_t     seq;

    // Skip Timing
    int        LastTxSkipTime;
    int        SkipScheduled;

    // Input packet state
    int        CompletionEvent;
    int        OutstandingCompletions;
    int        CplId;
    callback_t vuser_cb;
    void       *usrptr;

    // Input TLP/DLLP state
    bool       RxActive;
    int        RxDataIdx;
    PktData_t  *pRxPktData;

    // Input OS State
    os_callback_t vuser_os_cb;

    // Ordered set and Training sequence reception state
    LinkEventCount_t linkevent;

    // User configurable state
    UserConfig_t  usrconf;

    // Flow control state
    FlowControl_t flwcntl;

    bool draining_queue;
    bool tx_disabled;

} PcieModelState_t, *pPcieModelState_t;

// -------------------------------------------------------------------------
// EXTERNAL REFERENCES
// -------------------------------------------------------------------------

extern const unsigned int Bitrev8[];

// -------------------------------------------------------------------------
// PCIe model support function prototypes
// -------------------------------------------------------------------------

void        InitPcieState        (const pPcieModelState_t const state, const int node);
PktData_t * CreateDllpTemplate   (const int Type, PktData_t **payload_start);
PktData_t * CreateTlpTemplate    (const int Type, const uint64_t addr, const int bytelen, const int digest_present, PktData_t **payload_start);
void        CalcLcrc             (PktData_t *pkt);
void        CalcEcrc             (PktData_t *pkt);
void        CalcDllpCrc          (PktData_t *dllp);
int         CalcBe               (const int inaddr, const int byte_len);
int         CalcLoAddr           (const int fbe);
int         CalcByteCount        (const int len, int fbe, int lbe);
int         CheckCredits         (const int disable_fc, const uint32_t fc_state, const uint32_t hdr_credits, const uint32_t data_credits, 
                                  const uint32_t tx_hdr, const uint32_t tx_data, const int payload_len);
void        AddPktToQueue        (const pPcieModelState_t const state, const pPkt_t const packet);
void        AddPktToQueueDelay   (const pPcieModelState_t const state, const pPkt_t const packet);
void        ExtractPhyInput      (const pPcieModelState_t const state, const uint32_t* const rawlinkin);

uint32_t      CalcNewRand          (const uint32_t Seed);
void        CheckFree            (void *ptr);
void        TxFcInitInt          (const pFlowControl_t const flw, const pUserConfig_t usrcfg, const int node);
void        RxFcInit             (const pFlowControl_t const flw, const int dllptype, const int hdrval, const int dataval);

#endif

