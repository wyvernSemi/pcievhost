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
// PCIe link display routine header
//
//=============================================================

#include "pcie_utils.h"

#ifndef _DIPSLINK_H_
#define _DIPSLINK_H_

// ContDisp

#define DISPALL                      0x001
#define DISPFINISH                   0x002
#define DISPSTOP                     0x004
#define DISPUNUSED                   0x008
#define DISPTL                       0x010
#define DISPDL                       0x020
#define DISPPL                       0x040
#define DISPRAWSYM                   0x080
#define DISPSWTX                     0x100
#define DISPSWENRC                   0x200
#define DISPSWENEP                   0x400

// Definitions for use in formatted packet information display

//#define PCIENOFORMAT

# ifndef PCIENOFORMAT

#define FMT_NORMAL              "\033[0m"
#define FMT_BOLD                "\033[1m"
#define FMT_FAINT               "\033[2m"
#define FMT_ITALIC              "\033[3m"
#define FMT_UNDERLINE           "\033[4m"

#define FMT_BLACK               "\033[30m"
#define FMT_RED                 "\033[31m"
#define FMT_GREEN               "\033[32m"
#define FMT_YELLOW              "\033[33m"
#define FMT_BLUE                "\033[34m"
#define FMT_MAGENTA             "\033[35m"
#define FMT_CYAN                "\033[36m"
#define FMT_WHITE               "\033[37m"
#define FMT_BRIGHT_BLACK        "\033[90m"
#define FMT_BRIGHT_RED          "\033[91m"
#define FMT_BRIGHT_GREEN        "\033[92m"
#define FMT_BRIGHT_YELLOW       "\033[93m"
#define FMT_BRIGHT_BLUE         "\033[94m"
#define FMT_BRIGHT_MAGENTA      "\033[95m"
#define FMT_BRIGHT_CYAN         "\033[96m"
#define FMT_BRIGHT_WHITE        "\033[97m"

#define FMT_DATA_GREY           "\033[38;5;244m"

#ifndef FMT_UP
#define FMT_UP                   FMT_BRIGHT_BLUE
#endif

#ifndef FMT_DOWN
#define FMT_DOWN                 FMT_BRIGHT_GREEN
#endif

# else

#define FMT_NORMAL              ""
#define FMT_BOLD                ""
#define FMT_FAINT               ""
#define FMT_ITALIC              ""
#define FMT_UNDERLINE           ""

#define FMT_BLACK               ""
#define FMT_RED                 ""
#define FMT_GREEN               ""
#define FMT_YELLOW              ""
#define FMT_BLUE                ""
#define FMT_MAGENTA             ""
#define FMT_CYAN                ""
#define FMT_WHITE               ""
#define FMT_BRIGHT_BLACK        ""
#define FMT_BRIGHT_RED          ""
#define FMT_BRIGHT_GREEN        ""
#define FMT_BRIGHT_YELLOW       ""
#define FMT_BRIGHT_BLUE         ""
#define FMT_BRIGHT_MAGENTA      ""
#define FMT_BRIGHT_CYAN         ""
#define FMT_BRIGHT_WHITE        ""

#define FMT_DATA_GREY           ""

#ifndef FMT_UP
#define FMT_UP                   ""
#endif

#ifndef FMT_DOWN
#define FMT_DOWN                 ""
#endif

# endif

void ConstDisp     (pUserConfig_t                 usrconf);
void CheckContDisp (pUserConfig_t usrconf,                 const int      node);
void DispRaw       (const pPcieModelState_t const state,   const PktData_t *linkin, const int rx);
void DispOS        (const pPcieModelState_t const state,   const int          type, const pTS_t   const ts_data, const int lane, const bool rx, const int node);
void DispDll       (const pPcieModelState_t const state,   const pPkt_t const pkt,  const bool rx);
void DispTl        (const pPcieModelState_t const state,   const pPkt_t const pkt,  const bool rx);

#endif
