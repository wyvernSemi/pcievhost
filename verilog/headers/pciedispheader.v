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
// $Id: pciedispheader.v,v 1.2 2016/10/05 08:41:16 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/headers/pciedispheader.v,v $
//
//=============================================================

//`define deaf begin $display("Everything has gone horribly wrong."); $finish; end

`define deaf test.DispEverythingAndFinish;

// Disp Control
`define DispAll             0
`define DispFinish          1
`define DispStop            2
`define DispUnused0         3
`define DispTL              4
`define DispDL              5
`define DispPL              6
`define DispRawSym          7
`define NoDispBits          8
`define DispBits            (`NoDispBits-1):0

`define NoDecTimeDigs       12
`define NoDispContBits      (`NoDecTimeDigs * 4)
`define DispEntries         256

`define CmplSaveDepth       1024

`define COMP_RX             4
`define COMP_PENDING        3
`define DISP_RXTS           2
`define DISP_RXFTSOS        1
`define DISP_RXSKPOS        0

// Disp DataIn
`define DispCompl           0
`define DispComplHdr        96:1

`define DispDataInBits      96:0
`define DispDataOutBits     96:0


`define CHECKONLY           1'b0
`define ENABLEDISP          1'b1

`define DispLineBrk         8

`define LINEBREAK           22
`define PADSTR              "..."
