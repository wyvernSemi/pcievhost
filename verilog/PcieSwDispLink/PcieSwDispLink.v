//=============================================================
//
// Copyright (c) 2026 Simon Southwell. All rights reserved.
//
// Date: 11th Feb 2026
//
// This file is part of the pcieVHost package.
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

`ifdef VPROC_SV
`include "allheaders.v"
`endif

`WsTimeScale

//-------------------------------------------------------------
// PcieSwDispLink
//-------------------------------------------------------------

module PcieSwDispLink
#(
  parameter LinkWidth          = 16,
  parameter NodeNum            = 8,
  parameter EP                 = 0,
  parameter DisableScrambling  = 0,
  parameter Disable8b10b       = 0
)
(
  input       Clk,
  input       notReset,

  input [9:0] LinkIn0,   LinkIn1,   LinkIn2,   LinkIn3,
  input [9:0] LinkIn4,   LinkIn5,   LinkIn6,   LinkIn7,
  input [9:0] LinkIn8,   LinkIn9,   LinkIn10,  LinkIn11,
  input [9:0] LinkIn12,  LinkIn13,  LinkIn14,  LinkIn15
);

  PcieVhost
  #(
    .LinkWidth         (LinkWidth),
    .NodeNum           (NodeNum),
    .EndPoint          (EP),
    .DisableScrambling (DisableScrambling),
    .Disable8b10b      (Disable8b10b),
    .Gen2Clk           (0)
  ) pcievhost_i
  (
    .Clk             (Clk),
    .notReset        (notReset),

    .Gen2ClkSel      (),
    .ClkOut          (),

`ifdef VERILATOR
    .ElecIdleOut      (),
    .ElecIdleIn       (16'h0000),
`endif

    .LinkIn0          (LinkIn0),
    .LinkIn1          (LinkIn1),
    .LinkIn2          (LinkIn2),
    .LinkIn3          (LinkIn3),
    .LinkIn4          (LinkIn4),
    .LinkIn5          (LinkIn5),
    .LinkIn6          (LinkIn6),
    .LinkIn7          (LinkIn7),
    .LinkIn8          (LinkIn8),
    .LinkIn9          (LinkIn9),
    .LinkIn10         (LinkIn10),
    .LinkIn11         (LinkIn11),
    .LinkIn12         (LinkIn12),
    .LinkIn13         (LinkIn13),
    .LinkIn14         (LinkIn14),
    .LinkIn15         (LinkIn15),

    .LinkOut0         (),
    .LinkOut1         (),
    .LinkOut2         (),
    .LinkOut3         (),
    .LinkOut4         (),
    .LinkOut5         (),
    .LinkOut6         (),
    .LinkOut7         (),
    .LinkOut8         (),
    .LinkOut9         (),
    .LinkOut10        (),
    .LinkOut11        (),
    .LinkOut12        (),
    .LinkOut13        (),
    .LinkOut14        (),
    .LinkOut15        ()
  );

endmodule

