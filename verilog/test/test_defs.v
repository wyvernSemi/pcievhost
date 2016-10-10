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
// $Id: test_defs.v,v 1.1 2016/10/10 11:54:15 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/test/test_defs.v,v $
//
//=============================================================

`ifndef __TEST_DEFS_V_
`define __TEST_DEFS_V_

// Absolute path to fatal error task
`define fatal test.Fatal;

//-------------------------------------------------------------
// The following can be overridden on the compile command line.
//-------------------------------------------------------------

// Clock period in ps (500MHz)
`ifndef CLK_PERIOD
`define CLK_PERIOD               2000
`endif

// Simulation timeout (500ms)
`ifndef TIMEOUT_COUNT
`define TIMEOUT_COUNT            250000000
`endif

// Number of lanes (max is 16---other valid values are 8, 4, 2 and 1)
`ifndef PCIE_NUM_PHY_LANES
`define PCIE_NUM_PHY_LANES       16
`endif

// VProc node numbers must be different between host and endpoint,
// and be between 0 and VP_MAX_NODES-1, as defined in VProc package,
// or overridden in local makefile (via NUM_VPROC makefile variable).
`ifndef VPCIE_HOST_NODE_NUM
`define VPCIE_HOST_NODE_NUM      0
`endif

`ifndef VPCIE_EP_NODE_NUM
`define VPCIE_EP_NODE_NUM        (`VPCIE_HOST_NODE_NUM+1)
`endif


//-------------------------------------------------------------
// The following can be uncommented, or defined on the compile 
// command line to affact the behaviour of the simulation.
//-------------------------------------------------------------

// Set timescale to be 1ps/10ps, instead of 1ps/1ps, for faster simulation
//`define TIMESCALE_RESOL_10PS

// Use wide (10 bit) link lanes, without serialisation
//`define DISP_LINK_WIDE

// Descramble symbols before displaying in PcieDispLink when `DispRawSym bt set in ContDisps.hex
//`define DESCRAMBLE_RAW_PCIE

// Disable the (minimal) compliance checklist checking in PcieDispLink
//`define DISABLE_PCI_CHECKLIST

// If DISABLE_PCI_CHECKLIST defined, do not fatal on compliance error
//`define DISABLE_FATAL_ON_PCI_CHECKLIST

// Disable fatal on seeing a x on a lane after synchronisation
//`define DISABLE_PCIE_FATAL_ON_X

`endif
