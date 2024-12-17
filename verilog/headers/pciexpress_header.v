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
//=============================================================

`ifndef _PCIEXPRESS_HEADER_V_
`define _PCIEXPRESS_HEADER_V_

///////////////////////////////
// Physical layer definitions

// 10 bit COMMA codes
`define PCOMMA                      10'b1010000011   // 0x283
`define NCOMMA                      10'b0101111100   // 0x17c

// Physical Layer Special symbols
`define COM                         8'hbc       // K28.5
`define STP                         8'hfb       // K27.7
`define SDP                         8'h5c       // K28.2
`define END                         8'hfd       // K29.7
`define EDB                         8'hfe       // K30.7
`define PAD                         8'hf7       // K23.7
`define SKP                         8'h1c       // K28.0
`define FTS                         8'h3c       // K28.1
`define IDL                         8'h7c       // K28.3
`define EIE                         8'hfc       // K28.7
`define RV2                         8'h9c       // K28.4
`define RV3                         8'hdc       // K28.6

// Training sequence IDs
`define TS1_ID                      8'h4a       // D10.2
`define TS2_ID                      8'h45       // D5.2

// First header DW checked against this to ensure all reserved bits are zero
`define RSVD_MASK                   32'h808f0c00

// Training sequence ordered set indexes
`define TSX_LINKNUM                 1
`define TSX_LANENUM                 2
`define TSX_N_FTS                   3
`define TSX_DATARATE                4
`define TSX_LINKCONTROL             5

// Training sequence ordered set link reset bit masks
`define TSX_LNKCTRL_RESET_MASK      8'b00000001
`define TSX_LNKCTRL_DISABLE_MASK    8'b00000010
`define TSX_LNKCTRL_LOOPBACK_MASK   8'b00000100
`define TSX_LNKCTRL_NOSCRAMBLE_MASK 8'b00001000
`define TSX_LNKCTRL_COMPL_RX_MASK   8'b00010000

// Training sequence ordered set data rate values
`define TSX_DATARATE_GEN1           8'h02
`define TSX_DATARATE_GEN2           8'h06 /* If GEN2, GEN1 bit must be set as well */

///////////////////////////////
// Data Link layer definitions

// DLLP byte 0 definitions
`define DL_ACK                      8'h00
`define DL_NAK                      8'h10
`define DL_INITFC1_P                8'h40   // lower nibble x
`define DL_INITFC1_NP               8'h50   // lower nibble x
`define DL_INITFC1_CPL              8'h60   // lower nibble x
`define DL_INITFC2_P                8'hc0   // lower nibble x
`define DL_INITFC2_NP               8'hd0   // lower nibble x
`define DL_INITFC2_CPL              8'he0   // lower nibble x
`define DL_UPDATEFC_P               8'h80   // lower nibble x
`define DL_UPDATEFC_NP              8'h90   // lower nibble x
`define DL_UPDATEFC_CPL             8'ha0   // lower nibble x
`define DL_PM_ENTER_L1              8'h20
`define DL_PM_ENTER_L23             8'h21
`define DL_PM_REQ_L0S               8'h22
`define DL_PM_REQ_L1                8'h23
`define DL_PM_REQ_ACK               8'h24
`define DL_VENDOR                   8'h30

/////////////////////////////////
// Transaction layer definitions

// Fmt definitions. Base Specification V1.0a sec 2.2.1 Table 2-2
`define TL_CODEBITS                 30:24
`define TL_TYPEBITS                 28:24
`define TL_FMTBITS                  30:29
`define TL_PAYLOAD                  30
`define TL_4DW                      29
`define TL_TCBITS                   23:21
`define TL_TDBITS                   15
`define TL_EPBITS                   14
`define TL_ATTRBITS                 13:12
`define TL_LENGTHBITS               9:0

// Header word 1 (not completions), or word 2 (completions)
`define TL_REQID                    31:16
`define TL_BUSNUM                   31:24
`define TL_DEVNUM                   23:19
`define TL_FUNCNUM                  18:16
`define TL_TAG                      15:8
`define TL_BES                      7:0
`define TL_LASTBE                   7:4             // Mem, conf and I/O
`define TL_FIRSTBE                  3:0             // Mem, conf and I/O
`define TL_MSGCODE                  7:0             // Messages and vendor specific
`define TL_LWRADDR                  6:0             // Completions

// Header word 1 (completions)
`define TL_CID                      31:16
`define TL_COMPSTATUS               15:13
`define TL_BCM                      12
`define TL_BYTECOUNT                11:0

// Type definitions. Base Specification V1.0a sec 2.2.1 Table 2-3
`define TL_MRD32                    7'b0000000
`define TL_MRD64                    7'b0100000
`define TL_MRDLCK32                 7'b0000001
`define TL_MRDLCK64                 7'b0100001
`define TL_MWR32                    7'b1000000
`define TL_MWR64                    7'b1100000
`define TL_IORD                     7'b0000010
`define TL_IOWR                     7'b1000010
`define TL_CFGRD0                   7'b0000100
`define TL_CFGWR0                   7'b1000100
`define TL_CFGRD1                   7'b0000101
`define TL_CFGWR1                   7'b1000101
`ifdef VERILATOR
`define TL_MSG                      7'b0110???
`define TL_MSGD                     7'b1110???
`else
`define TL_MSG                      7'b0110xxx
`define TL_MSGD                     7'b1110xxx
`endif
`define TL_MSGAS                    7'b0111000
`define TL_MSGASD                   7'b1111000
`define TL_CPL                      7'b0001010
`define TL_CPLD                     7'b1001010
`define TL_CPLLK                    7'b0001011
`define TL_CPLDLK                   7'b1001011

// Message routing. Base Specification V1.0a sec 2.2.8 Table 2-11
`define TL_ROUTE_ROOT               3'b000
`define TL_ROUTE_ADDR               3'b001
`define TL_ROUTE_ID                 3'b010
`define TL_ROUTE_BCAST              3'b011
`define TL_ROUTE_LOCAL              3'b100
`define TL_ROUTE_GATHER             3'b101

// Message codes
`define TL_ASSERT_INTA              8'b00100000
`define TL_ASSERT_INTB              8'b00100001
`define TL_ASSERT_INTC              8'b00100010
`define TL_ASSERT_INTD              8'b00100011
`define TL_DEASSERT_INTA            8'b00100100
`define TL_DEASSERT_INTB            8'b00100101
`define TL_DEASSERT_INTC            8'b00100110
`define TL_DEASSERT_INTD            8'b00100111
`define TL_PM_ACTIVE_STATE_NAK      8'b00010100
`define TL_PM_PME                   8'b00011000
`define TL_PM_TURN_OFF              8'b00011001
`define TL_PM_TO_ACK                8'b00011011
`define TL_ERR_COR                  8'b00110000
`define TL_ERR_NON_FATAL            8'b00110001
`define TL_ERR_FATAL                8'b00110011
`define TL_UNLOCK                   8'b00000000
`define TL_SET_SLOT_PWR_LIM         8'b01010000
`define TL_VENDOR_TYPE0             8'b01111110
`define TL_VENDOR_TYPE1             8'b01111111

// Completion statuses
`define TL_COMPL_SC                 3'b000
`define TL_COMPL_UC                 3'b001
`define TL_COMPL_CRS                3'b010
`define TL_COMPL_CA                 3'b100

///////////////////////////////
// Data link layer buffer params

//Protocol error number
`define size_difference             1
`define TXN_2_0_2                   2
`define TXN_2_1_2                   3
`define TXN_2_1_4                   4
`define TXN_2_1_5                   5
`define TXN_2_2_5                   6
`define TXN_2_2_6                   7
`define TXN_2_3_1                   8
`define TXN_2_4_1                   9
`define TXN_2_6_1                   10
`define TXN_2_6_2                   11
`define TXN_2_6_3                   12
`define TXN_2_7_2                   13
`define TXN_2_8_1                   14 
`define TXN_2_9_1                   15
`define TXN_2_11_4                  16
`define TXN_2_11_7                  17
`define TXN_3_2_3                   18
`define TXN_2_21_3                  19
`define TXN_2_21_19                 20
`define TXN_2_21_13                 21
`define TXN_2_21_22                 22
`define TXN_2_4_2                   23
`define TXN_2_6_9                   24
`define TXN_3_3_1                   25
`define TXN_2_7_1                   26
`define TXN_7_1_6                   27

`define DLL_5_2_20                  28
`define DLL_4_1_34                  29
`define DLL_4_1_5_to_33             30

`define PHY_2_1_1                   31

`define TXN_2_12_7                  32
`define TXN_2_21_7                  33
`define TXN_2_12_2                  34
`define TXN_2_21_8                  35

`endif
