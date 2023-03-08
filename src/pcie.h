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
// $Id: pcie.h,v 1.5 2016/10/17 11:47:02 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/pcie.h,v $
//
//=============================================================

#ifndef _PCIE_H_
#define _PCIE_H_

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Pull in public mem.c functions and definitions for exporting
// in API. Include common (and public) definition of PktData_t.
#include "mem.h"

#include "pcie_dpi.h"
#include "pci_express.h"
#include "pcie_vhost_map.h"

// -------------------------------------------------------------------------
// Generic definitions
// -------------------------------------------------------------------------

// Used in macros
#define BYTE_MASK                    0xff

// -------------------------------------------------------------------------
// PCIe virtual host definitions
// -------------------------------------------------------------------------

#define MAX_LINK_WIDTH               16

#define QUEUE                        1
#define SEND                         0

#define LCRC_TERMINATION_LOOKAHEAD   5
#define ECRC_TERMINATION_LOOKAHEAD   9
#define PKT_TERMINATION              0xffff

#define DLLP_SEQ_ID                  -1

#define TS_COMMA_SEQ                 0
#define TS_LINK_NUM_SEQ              1
#define TS_LANE_NUM_SEQ              2
#define TS_N_FTS_SEQ                 3
#define TS_DATA_RATE_SEQ             4
#define TS_CONTROL_SEQ               5
#define TS_IDENT_SEQ                 6

// Byte counts
#define HDR3DW                       12
#define HDR4DW                       16
#define TAILDWNODIGEST               4
#define TAILDWDIGEST                 8

#define FIXED_OVERHEAD_START         3
#define FIXED_OVERHEAD_END           1
#define FIXED_OVERHEAD_LENGTH        (FIXED_OVERHEAD_START+FIXED_OVERHEAD_END)

// Masks
#define TLP_DIGEST_MASK              0x80
#define KCODEMASK                    0x100

#define ADDR_LO_BYTE_MASK            0xfc
#define ADDR_DW_OFFSET_MASK          0x03
#define ADDR_HI_BIT_MASK             0xffffffff00000000ULL
#define ADDR_LO_BIT_MASK             0xffffffffULL

#define MSG_IDENTIFIER_BITS          0x18
#define MSG_IDENTIFIER_VALUE         0x10
#define MSG_ROUTE_BIT_MASK           0x78

#define TL_ADDR64_MASK               0x5f
#define TLP_TYPE_MASK                0x7f
#define TLP_CFG_LO_ADDR_MASK         0x7fULL
#define TLP_CPL_LO_ADDR_MASK         0x3fULL
#define TLP_TYPE_VARIANT_BIT         0x01
#define TLP_EP_VARIANT_BIT           0x40

#define TLP_TD_BYTE_MASK             BIT7MASK
#define TLP_EP_BYTE_MASK             BIT6MASK
#define TLP_ATTR_BYTE_MASK           (BIT5MASK | BIT4MASK)

#define DL_VC_MASK                   0xf8
#define DL_ROUTE_MASK                0xf8

// Header field byte offsets
#define DLLP_SEQ_OFFSET              1
#define TLP_TYPE_BYTE_OFFSET         3
#define TLP_TC_BYTE_OFFSET           4
#define TLP_TD_BYTE_OFFSET           5
#define TLP_EP_BYTE_OFFSET           5
#define TLP_LENGTH_OFFSET            5
#define TLP_ATTR_BYTE_OFFSET         5
#define TLP_RID_OFFSET               7
#define TLP_TAG_OFFSET               9
#define TLP_BE_OFFSET                10
#define TLP_ADDR_OFFSET              11
#define TLP_DATA_OFFSET32            15
#define TLP_DATA_OFFSETCPL           15
#define TLP_DATA_OFFSET64            19

#define CPL_CID_OFFSET               7
#define CPL_STATUS_OFFSET            9
#define CPL_BYTE_COUNT_OFFSET        9
#define CPL_RID_OFFSET               11
#define CPL_TAG_OFFSET               13
#define CPL_LOW_ADDR_OFFSET          14

#define CFG_RID_OFFSET               7
#define CFG_TAG_OFFSET               9
#define CFG_BUS_OFFSET               11
#define CFG_DEV_OFFSET               12
#define CFG_FUN_OFFSET               12
#define CFG_EXT_REG_OFFSET           13
#define CFG_REG_OFFSET               14

#define MSG_CODE_OFFSET              10

#define DLLP_HDR_FC_OFFSET           2
#define DLLP_DATA_FC_OFFSET          3
#define DLLP_CRC_OFFSET              5

#define MAX_RAW_PKT_SIZE             4125
#define NULLACK                      9999

#define FC_POST                      0
#define FC_NONPOST                   1
#define FC_CMPL                      2
#define FC_NUMTYPES                  3

#define FC_DEFAULT_PDATA_CREDITS     (16*1008/FC_DATA_CREDIT_BYTES)
#define FC_DEFAULT_PHDR_CREDITS      32

#define FC_DEFAULT_NPDATA_CREDITS    1
#define FC_DEFAULT_NPHDR_CREDITS     32

#define FC_DEFAULT_CPLDATA_CREDITS   FC_INFINITE_CREDITS
#define FC_DEFAULT_CPLHDR_CREDITS    FC_INFINITE_CREDITS

#define RCVD_P                       1
#define RCVD_NP                      2
#define RCVD_CPL                     4
#define RCVD_ALL                     7

#define INITFC_IDLE                  0
#define INITFC_FI1                   1
#define INITFC_FI2                   3

#define INITFC_DELAY                 20

#define DEFAULT_MAX_PAYLOAD_SIZE     (4096/FC_DATA_CREDIT_BYTES)

#define DEFAULT_HFC_CONSUMPTION_RATE 4
#define DEFAULT_DFC_CONSUMPTION_RATE 4

#define DEFAULT_COMPLETION_RATE      0
#define DEFAULT_COMPLETION_SPREAD    0

#define DEFAULT_SKIP_INTERVAL        1180
#define MINIMUM_SKIP_INTERVAL        10
#define DEFAULT_ACK_RATE             1

#define LAST_ACK_NULL                -1

// --------------- user macros ---------------
#define PKT_STATUS_GOOD              0
#define PKT_STATUS_BAD_LCRC          1
#define PKT_STATUS_BAD_DLLP_CRC      1
#define PKT_STATUS_BAD_ECRC          2
#define PKT_STATUS_UNSUPPORTED       4
#define PKT_STATUS_NULLIFIED         8

#define CONFIG_FC_HDR_RATE           0
#define CONFIG_FC_DATA_RATE          1

// Enables even, disables odd
#define CONFIG_ENABLE_FC             2
#define CONFIG_DISABLE_FC            3
#define CONFIG_ENABLE_ACK            4
#define CONFIG_DISABLE_ACK           5
#define CONFIG_ENABLE_MEM            6
#define CONFIG_DISABLE_MEM           7

#define CONFIG_ENABLE_SKIPS          8
#define CONFIG_DISABLE_SKIPS         9

#define CONFIG_ENABLE_UR_CPL         10
#define CONFIG_DISABLE_UR_CPL        11

#define CONFIG_POST_HDR_CR           12
#define CONFIG_POST_DATA_CR          13

#define CONFIG_NONPOST_HDR_CR        14
#define CONFIG_NONPOST_DATA_CR       15

#define CONFIG_CPL_HDR_CR            16
#define CONFIG_CPL_DATA_CR           17

#define CONFIG_CPL_DELAY_RATE        18
#define CONFIG_CPL_DELAY_SPREAD      19

#define CONFIG_LTSSM_LINKNUM         21
#define CONFIG_LTSSM_N_FTS           22
#define CONFIG_LTSSM_TS_CTL          23
#define CONFIG_LTSSM_DETECT_QUIET_TO 24
#define CONFIG_LTSSM_ENABLE_TESTS    25
#define CONFIG_LTSSM_FORCE_TESTS     26

// Valid force/enable test masks
#define ENABLE_DISABLE               0x1
#define ENABLE_COMPLIANCE            0x2
#define ENABLE_LOOPBACK              0x4


// -------------------------------------------------------------------------
// Header field access macros
// -------------------------------------------------------------------------

#define SET_TLP_TAG(_TAG, _PTR){        \
    (_PTR)[TLP_TAG_OFFSET] = ((_TAG)%0xff);    \
}

#define SET_CPL_TAG(_TAG, _PTR){        \
    (_PTR)[CPL_TAG_OFFSET] = (_TAG);    \
}

#define SET_TLP_TC(_TC, _PTR){  \
    (_PTR)[TLP_TC_BYTE_OFFSET] = ((_PTR)[TLP_TC_BYTE_OFFSET] & 0x8f) | (((_TC) & 0x3)<<4);      \
}

#define SET_TLP_TD(_TD, _PTR){  \
    (_PTR)[TLP_TD_BYTE_OFFSET] = ((_PTR)[TLP_TD_BYTE_OFFSET] & 0x7f) | (((_TD) & 0x1)<<7);      \
}

#define SET_TLP_EP(_EP, _PTR){  \
    (_PTR)[TLP_EP_BYTE_OFFSET] = ((_PTR)[TLP_EP_BYTE_OFFSET] & 0xbf) | (((_EP) & 0x1)<<6);      \
}

#define SET_TLP_ATTR(_ATTR, _PTR){      \
    (_PTR)[TLP_ATTR_BYTE_OFFSET] = ((_PTR)[TLP_ATTR_BYTE_OFFSET] & 0xcf) | (((_ATTR) & 0x3)<<4); \
}

#define SET_TLP_RID(_RID, _PTR){        \
    (_PTR)[TLP_RID_OFFSET]   = ((_RID) >> 8) & BYTE_MASK;    \
    (_PTR)[TLP_RID_OFFSET+1] = (_RID) & BYTE_MASK;   \
}

#define SET_CPL_CID(_RID, _PTR){        \
    (_PTR)[CPL_CID_OFFSET]   = ((_RID) >> 8) & BYTE_MASK;    \
    (_PTR)[CPL_CID_OFFSET+1] = (_RID) & BYTE_MASK;   \
}

#define SET_CPL_RID(_RID, _PTR){        \
    (_PTR)[CPL_RID_OFFSET]   = ((_RID) >> 8) & BYTE_MASK;    \
    (_PTR)[CPL_RID_OFFSET+1] = (_RID) & BYTE_MASK;   \
}

#define SET_CPL_BYTE_COUNT(_COUNT, _PTR){        \
    (_PTR)[CPL_BYTE_COUNT_OFFSET]   = ((_PTR)[CPL_BYTE_COUNT_OFFSET] & 0xf0) | ((_COUNT) >> 8) & LO_NIBBLE_MASK;    \
    (_PTR)[CPL_BYTE_COUNT_OFFSET+1] = (_COUNT) & BYTE_MASK;   \
}

#define SET_CPL_STATUS(_STATUS, _PTR){        \
    (_PTR)[CPL_STATUS_OFFSET]   = ((_PTR)[CPL_STATUS_OFFSET] & 0x1f) | ((_STATUS) << 5) & 0xe0;  \
}

#define SET_CPL_LOW_ADDR(_ADDR, _PTR){        \
    (_PTR)[CPL_LOW_ADDR_OFFSET] = ((_ADDR) & 0x7f);    \
}

#define SET_DLLP_SEQ(_SEQ, _PTR){               \
    (_PTR)[DLLP_SEQ_OFFSET]   = ((_SEQ) >> 8) & LO_NIBBLE_MASK;    \
    (_PTR)[DLLP_SEQ_OFFSET+1] = (_SEQ) & BYTE_MASK;  \
}

#define SET_CFG_TAG(_TAG, _PTR){        \
    (_PTR)[CFG_TAG_OFFSET] = (_TAG);    \
}

#define SET_CFG_BUS(_BUS, _PTR){ \
    (_PTR)[CFG_BUS_OFFSET] = (_BUS) & BYTE_MASK; \
}

#define SET_CFG_CID(_CID, _PTR){ \
    (_PTR)[CFG_DEV_OFFSET] = (_CID) & BYTE_MASK; \
    (_PTR)[CFG_BUS_OFFSET] = ((_CID) >> 8) & BYTE_MASK; \
}

#define SET_CFG_RID(_RID, _PTR){        \
    (_PTR)[CFG_RID_OFFSET]   = ((_RID) >> 8) & BYTE_MASK;    \
    (_PTR)[CFG_RID_OFFSET+1] = (_RID) & BYTE_MASK;   \
}

#define SET_CFG_DEV(_DEV, _PTR){ \
    (_PTR)[CFG_DEV_OFFSET] = ((_PTR)[CFG_DEV_OFFSET] & 0x07) | (((_DEV) & 0x1f)<<3); \
}

#define SET_MSG_CODE(_CODE, _PTR){ \
    (_PTR)[MSG_CODE_OFFSET] = (_CODE) & BYTE_MASK; \
}

#define SET_CFG_FUN(_FUN, _PTR){ \
    (_PTR)[CFG_FUN_OFFSET] = ((_PTR)[CFG_FUN_OFFSET] & 0xf8) | ((_FUN) & 0x07); \
}

#define SET_CFG_EXT_REG(_REG, _PTR){ \
    (_PTR)[CFG_EXT_REG_OFFSET] = ((_PTR)[CFG_EXT_REG_OFFSET] & HI_NIBBLE_MASK) | ((_REG) & LO_NIBBLE_MASK); \
}

#define SET_CFG_REG(_REG, _PTR){ \
    (_PTR)[CFG_REG_OFFSET] = ((_PTR)[CFG_REG_OFFSET] & 0x02) | ((_REG) & 0xfc); \
}

#define GET_DLLP_SEQ(_PKT) ((((_PKT)[DLLP_SEQ_OFFSET] & LO_NIBBLE_MASK) << 8) | ((_PKT)[DLLP_SEQ_OFFSET+1]))

#define GET_TLP_TYPE(_PKT)    ((_PKT)[TLP_TYPE_BYTE_OFFSET] & BYTE_MASK)
#define GET_TLP_LENGTH_RAW(_PKT)  ((((_PKT)[TLP_LENGTH_OFFSET] & 0x3) << 8) | ((_PKT)[TLP_LENGTH_OFFSET+1] & BYTE_MASK))
#define GET_TLP_LENGTH_ADJ(_PKT)  (GET_TLP_LENGTH_RAW(_PKT) ? GET_TLP_LENGTH_RAW(_PKT) : 1024)
#define GET_TLP_LENGTH(_PKT)  GET_TLP_LENGTH_ADJ(_PKT)
#define GET_TLP_FBE(_PKT)     ((_PKT)[TLP_BE_OFFSET] & LO_NIBBLE_MASK)
#define GET_TLP_LBE(_PKT)     (((_PKT)[TLP_BE_OFFSET] >> 4) & LO_NIBBLE_MASK)
#define GET_TLP_RID(_PKT)     ((((_PKT)[TLP_RID_OFFSET] & BYTE_MASK) << 8) | ((_PKT)[TLP_RID_OFFSET+1] & BYTE_MASK))
#define GET_TLP_TAG(_PKT)     ((_PKT)[TLP_TAG_OFFSET] & BYTE_MASK)
#define TLP_HAS_DIGEST(_PKT)  (((_PKT)[TLP_TD_BYTE_OFFSET] & TLP_TD_BYTE_MASK) ? 1 : 0)
#define TLP_HDR_4DW(_PKT)     (((_PKT)[TLP_TYPE_BYTE_OFFSET] & 0x20) ? 1 : 0)
#define TLP_IS_POSTED(_PKT)   (((_PKT)[TLP_TYPE_BYTE_OFFSET] & 0x20) ? 1 : 0)
#define OFFSET_FROM_FBE(_FBE) (((((_FBE) & 0x3) == 0x00) ? 0x2 : 0x0) + ( ((((_FBE) & 0x3) == 0x2) || (((_FBE) & 0xc) == 0x80)) ? 0x1 : 0x0))
#define OFFSET_FROM_LBE(_LBE) ((((_LBE) == 0xf) || ((_LBE) == 0x0)) ? 0x0 : (_LBE) == 0x7 ? 1 : (_LBE) == 0x3 ? 0x2 : 0x3)

#define GET_TLP_3DW_ADDRESS(_PKT) ((uint64_t)(_PKT)[TLP_ADDR_OFFSET]          | (uint64_t)(_PKT)[TLP_ADDR_OFFSET+1]<<8ULL  | \
                                   (uint64_t)(_PKT)[TLP_ADDR_OFFSET+2]<<16ULL | (uint64_t)(_PKT)[TLP_ADDR_OFFSET+3]<<24ULL)

#define GET_TLP_4DW_ADDRESS(_PKT) ((uint64_t)(_PKT)[TLP_ADDR_OFFSET]          | (uint64_t)(_PKT)[TLP_ADDR_OFFSET+1]<<8ULL  | \
                                   (uint64_t)(_PKT)[TLP_ADDR_OFFSET+2]<<16ULL | (uint64_t)(_PKT)[TLP_ADDR_OFFSET+3]<<24ULL | \
                                   (uint64_t)(_PKT)[TLP_ADDR_OFFSET+4]<<32ULL | (uint64_t)(_PKT)[TLP_ADDR_OFFSET+5]<<40ULL | \
                                   (uint64_t)(_PKT)[TLP_ADDR_OFFSET+6]<<48ULL | (uint64_t)(_PKT)[TLP_ADDR_OFFSET+7]<<56ULL)
#define GET_TLP_ADDRESS(_PKT) (TLP_HDR_4DW(_PKT) ? GET_TLP_4DW_ADDRESS(_PKT) : GET_TLP_3DW_ADDRESS(_PKT))

#define GET_TLP_PAYLOAD_PTR(_PKT) (TLP_HDR_4DW(_PKT) ? (&(_PKT)[19]) : (&(_PKT)[15]))

#define GET_HDR_FC(_DATA)     (((_DATA)[DLLP_HDR_FC_OFFSET] << 2) | (((_DATA)[DLLP_HDR_FC_OFFSET+1] & 0xc0) >> 6))
#define GET_DATA_FC(_DATA)    ((((_DATA)[DLLP_DATA_FC_OFFSET] & LO_NIBBLE_MASK) << 8) | ((_DATA)[DLLP_DATA_FC_OFFSET+1] & BYTE_MASK))

// Completion only status
#define GET_CPL_TAG(_PKT)     ((_PKT)[CPL_TAG_OFFSET] & BYTE_MASK)
#define GET_CPL_BYTECOUNT(_PKT)  (((_PKT)[CPL_BYTE_COUNT_OFFSET] & 0xf) << 8ULL | \
                                  ((_PKT)[CPL_BYTE_COUNT_OFFSET+1] & BYTE_MASK))
#define GET_CPL_STATUS(_PKT) (((_PKT)[CPL_STATUS_OFFSET] & 0xe0) >> 5)
#define GET_CPL_CID(_PKT)     ((((_PKT)[CPL_CID_OFFSET] & BYTE_MASK) << 8) | (((_PKT)[CPL_CID_OFFSET+1] & BYTE_MASK)))
#define GET_CFG_CID(_PKT)     ((((_PKT)[CFG_BUS_OFFSET] & BYTE_MASK) << 8) | (((_PKT)[CFG_BUS_OFFSET+1] & BYTE_MASK)))

#define DISCARD_PACKET(_PKT)  {free((_PKT)->data); free(_PKT);}

// -------------------------------------------------------------------------
// PCIe model type definitions
// -------------------------------------------------------------------------

typedef struct  pkt_struct *pPkt_t;
typedef struct  pkt_struct {
    pPkt_t    NextPkt;     // Pointer to next packet to be sent
    PktData_t *data;       // Pointer to a raw data packet, terminated by -1
    int       seq;         // DLL sequence number for packet (-1 for DLLP)
    int       Retry;
    uint32_t    TimeStamp;
    uint32_t    ByteCount;
} sPkt_t;

typedef struct {
    int linknum;
    int lanenum;
    int n_fts;
    int datarate;
    int control;
    int id;
} TS_t, *pTS_t;

typedef void (*callback_t)(pPkt_t, int, void *);
typedef void (*os_callback_t)(int, int, pTS_t, void *);

// -------------------------------------------------------------------------
// PCIe model API prototypes (excluding those define in mem.h)
// -------------------------------------------------------------------------

// TLPs
extern pPktData_t MemWrite             (const uint64_t addr, const PktData_t *data, const int length, const int tag, 
                                        const uint32_t rid, const bool queue, const int node);

extern pPktData_t MemRead              (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue, const int node);

extern pPktData_t Completion           (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length, 
                                        const int tag, const uint32_t cid, const uint32_t rid, const bool queue, const int node);

extern pPktData_t PartCompletion       (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength, 
                                        const int length, const int tag, const uint32_t cid, const uint32_t rid, const bool queue, const int node);

extern pPktData_t CompletionDelay      (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length, 
                                        const int tag, const uint32_t cid, const uint32_t rid, const int node);

extern pPktData_t PartCompletionDelay  (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength, 
                                        const int length, const int tag, const uint32_t cid, const uint32_t rid, const bool digest, const bool delay, 
                                        const bool queue, const int node);

extern pPktData_t CfgWrite             (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool queue, 
                                        const int node);

extern pPktData_t CfgRead              (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue, const int node);

extern pPktData_t IoWrite              (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, 
                                        const bool queue, const int node);

extern pPktData_t IoRead               (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue, const int node);

extern pPktData_t Message              (const int code, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool queue, 
                                        const int node);

// TLP variant with digest (ECRC) generation argument
extern pPktData_t MemWriteDigest       (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                        const bool queue, const int node);

extern pPktData_t MemReadDigest        (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue, 
                                        const int node);

extern pPktData_t CompletionDigest     (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length, 
                                        const int tag, const uint32_t cid, const uint32_t rid, const bool digest, const bool queue, const int node);

extern pPktData_t PartCompletionDigest (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength, 
                                        const int length, const int tag , const uint32_t cid, const uint32_t rid, const bool digest, const bool queue, const int node);

extern pPktData_t CfgWriteDigest       (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                        const bool queue, const int node);

extern pPktData_t CfgReadDigest        (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue,
                                        const int node);

extern pPktData_t IoWriteDigest        (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                        const bool queue, const int node);

extern pPktData_t IoReadDigest         (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue,
                                        const int node);

extern pPktData_t MessageDigest        (const int code, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest, 
                                        const bool queue, const int node);


// Link initialisation
extern void       InitFc               (const int node);
extern void       InitLink             (const int linkwidth, const int node);

// Queue flushing
extern void       SendPacket           (const int node);

// Dllps
extern void       SendAck              (const int seq,   const int node);
extern void       SendNak              (const int seq,   const int node);
extern void       SendFC               (const int type,  const int vc,    const int hdrfc, const int datafc, const bool queue, const int node);
extern void       SendPM               (const int type,  const bool queue, const int node);
extern void       SendVendor           (const bool queue, const int node);

// Physical layer Ordered sets etc.
extern void       SendIdle             (const int Ticks, const int node);
extern void       SendOs               (const int Type,  const int node);
extern void       SendTs               (const int identifier, const int lane_num, const int link_num, const int n_fts, const int control, 
                                        const bool is_gen2, const int node);

extern void       WaitForCompletion    (const int node);
extern void       WaitForCompletionN   (const uint32_t count,          const int node);
extern void       InitialisePcie       (const callback_t    cb_func, void *usrptr, const int node);
extern void       RegisterOsCallback   (const os_callback_t cb_func, const int node);
extern uint32_t     GetCycleCount        (const int node);
extern void       ConfigurePcie        (const int type, const int value, const int node);

// Physical layer event routines
extern int        ResetEventCount      (const int type, const int node);
extern int        ReadEventCount       (const int type, uint32_t *ts_data, const int node);
extern TS_t       GetTS                (const int lane, const int node);

// Miscellaneous support routines
extern uint32_t     PcieRand             (const int node);
extern void       PcieSeed             (const uint32_t seed, const int node);
extern void       SetTxEnabled         (const int node);
extern void       SetTxDisabled        (const int node);
#endif

