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
#define DISPSWDISRC                  0x200
#define DISPSWDISEP                  0x400

void ConstDisp     (pUserConfig_t                 usrconf);
void CheckContDisp (pUserConfig_t usrconf,                const int      node);
void DispRaw       (const pPcieModelState_t const state,  const unsigned *linkin, const int rx);
void DispTS        (const pPcieModelState_t const state,  const int      ts_type, const pTS_t   const ts_data, const int lane, const bool rx, const int node);
void DispDll       (const pPcieModelState_t const state,  const pPkt_t const pkt, const bool rx, const int node);
void DispTl        (const pPcieModelState_t const state,  const pPkt_t const pkt, const bool rx, const int node);

#endif
