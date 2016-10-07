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
// $Id: timescale.v,v 1.1 2016/10/04 15:47:36 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/headers/timescale.v,v $
//
//=============================================================

`ifdef resol_10ps
`define WsTimeScale `timescale 1 ps / 10 ps
`else
`define WsTimeScale `timescale 1 ps / 1 ps
`endif

`ifndef RegDel
`define RegDel             1
`endif

`define PcieVHostSampleDel 10