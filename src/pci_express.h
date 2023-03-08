//=============================================================
// 
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Mar 2023
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

#ifndef _PCI_EXPRESS_H_
#define _PCI_EXPRESS_H_

// -------------------------------------------------------------------------
// PCIe specific definitions
// -------------------------------------------------------------------------

#define MAX_PAYLOAD_BYTES          4096
#define MAX_VIRTUAL_CHANNELS       8

#define MAX_HDR_CREDITS            255
#define MAX_DATA_CREDITS           4095

// Control codes (decoded)
#define COM                        0x1bc // K28.5
#define STP                        0x1fb // K27.7
#define SDP                        0x15c // K28.2
#define END                        0x1fd // K29.7
#define EDB                        0x1fe // K30.7
#define PAD                        0x1f7 // K23.7
#define SKP                        0x11c // K28.0
#define FTS                        0x13c // K28.1
#define IDL                        0x17c // K28.3
#define RV1                        0x1fc // K28.7
#define RV2                        0x19c // K28.4
#define RV3                        0x1dc // K28.6

// Training sequence definitions
#define TS1_ID                     0x4a
#define TS2_ID                     0x45
#define INV_TS1_ID                 0xb5
#define INV_TS2_ID                 0xba

#define TS_DATA_RATE_GEN1          0x02
#define TS_DATA_RATE_GEN2          0x06

#define TS_CNTL_HOT_RESET          0x01
#define TS_CNTL_DISABLE_LINK       0x02
#define TS_CNTL_LOOPBACK           0x04
#define TS_CNTL_NO_SCRAMBLING      0x08 
#define TS_CNTL_COMPLIANCE_RX      0x10 
#define TS_CNTL_MAX_VALUE          0x1f
#define TS_N_FTS_MAX_VALUE         0xff
#define TS_LINK_NUM_MAX_VALUE      0xff

// TLP types
#define TL_MRD32                   0x00 // 0000000
#define TL_MRD64                   0x20 // 0100000
#define TL_MRDLCK32                0x01 // 0000001
#define TL_MRDLCK64                0x21 // 0100001
#define TL_MWR32                   0x40 // 1000000
#define TL_MWR64                   0x60 // 1100000
#define TL_IORD                    0x02 // 0000010
#define TL_IOWR                    0x42 // 1000010
#define TL_CFGRD0                  0x04 // 0000100
#define TL_CFGWR0                  0x44 // 1000100
#define TL_CFGRD1                  0x05 // 0000101
#define TL_CFGWR1                  0x45 // 1000101
#define TL_MSG                     0x30 // 0110xxx
#define TL_MSGD                    0x70 // 1110xxx
#define TL_CPL                     0x0a // 0001010
#define TL_CPLD                    0x4a // 1001010
#define TL_CPLLK                   0x0b // 0001011
#define TL_CPLDLK                  0x4b // 1001011

// DLLP types
#define DL_ACK                     0x00
#define DL_NAK                     0x10
#define DL_INITFC1_P               0x40 // lower nibble x
#define DL_INITFC1_NP              0x50 // lower nibble x
#define DL_INITFC1_CPL             0x60 // lower nibble x
#define DL_INITFC2_P               0xc0 // lower nibble x
#define DL_INITFC2_NP              0xd0 // lower nibble x
#define DL_INITFC2_CPL             0xe0 // lower nibble x
#define DL_UPDATEFC_P              0x80 // lower nibble x
#define DL_UPDATEFC_NP             0x90 // lower nibble x
#define DL_UPDATEFC_CPL            0xa0 // lower nibble x
#define DL_PM_ENTER_L1             0x20
#define DL_PM_ENTER_L23            0x21
#define DL_PM_REQ_L0S              0x22
#define DL_PM_REQ_L1               0x23
#define DL_PM_REQ_ACK              0x24
#define DL_VENDOR                  0x30

#define DL_VC_BITS                 0x07
#define DL_MAX_HDRFC               0xff
#define DL_MAX_DATAFC              0xfff

// Message types
#define MSG_ROUTE_ROOT             0x0
#define MSG_ROUTE_ADDR             0x1
#define MSG_ROUTE_ID               0x2
#define MSG_ROUTE_BCAST            0x3
#define MSG_ROUTE_LOCAL            0x4
#define MSG_ROUTE_GATHER           0x5

#define MSG_ASSERT_INTA            0x20
#define MSG_ASSERT_INTB            0x21
#define MSG_ASSERT_INTC            0x22
#define MSG_ASSERT_INTD            0x23
#define MSG_DEASSERT_INTA          0x24
#define MSG_DEASSERT_INTB          0x25
#define MSG_DEASSERT_INTC          0x26
#define MSG_DEASSERT_INTD          0x27
#define MSG_PM_ACTIVE_NAK          0x14
#define MSG_PM_PME                 0x18
#define MSG_PME_OFF                0x19
#define MSG_PME_TO_ACK             0x1b
#define MSG_ERR_COR                0x30
#define MSG_ERR_NONFATAL           0x31
#define MSG_ERR_FATAL              0x33
#define MSG_UNLOCK                 0x00
#define MSG_SET_PWR_LIMIT          0x50
#define MSG_VENDOR_0               0x7e
#define MSG_VENDOR_1               0x7f

// Completion status

#define CPL_SUCCESS                0
#define CPL_UNSUPPORTED            1
#define CPL_CRS                    2
#define CPL_ABORT                  4

// Flow control
#define FC_DATA_CREDIT_BYTES       16
#define FC_INFINITE_CREDITS        0
#define DEFAULT_FC_TIME            7500

#define OS_LENGTH                  4
#define TS_LENGTH                  16

#endif
