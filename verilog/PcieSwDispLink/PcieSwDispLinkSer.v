//=============================================================
//
// Copyright (c) 2026 Simon Southwell. All rights reserved.
//
// Date: 12th Feb 2026
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
// PcieSwDispLinkSer
//-------------------------------------------------------------

module PcieSwDispLinkSer
#(
  parameter    LinkWidth = 16,
  parameter    NodeNum   = 8,
  parameter    EndPoint  = 0
)
(
  input        Clk,
  input        SerClk,
  input        notReset,
  input [15:0] LinkIn
);

//-------------------------------------------------------------
// Instantiate (wide) software display link component
//-------------------------------------------------------------

wire [159:0] PLinkVec;

  PcieSwDispLink
  #(
    .LinkWidth         (LinkWidth),
    .NodeNum           (NodeNum),
    .EP                (EndPoint),
    .DisableScrambling (0),
    .Disable8b10b      (0)
  ) psdl_i
  (
    .Clk               (Clk),
    .notReset          (notReset),

    .LinkIn0           (PLinkVec[  9:0]),
    .LinkIn1           (PLinkVec[ 19:10]),
    .LinkIn2           (PLinkVec[ 29:20]),
    .LinkIn3           (PLinkVec[ 39:30]),
    .LinkIn4           (PLinkVec[ 49:40]),
    .LinkIn5           (PLinkVec[ 59:50]),
    .LinkIn6           (PLinkVec[ 69:60]),
    .LinkIn7           (PLinkVec[ 79:70]),
    .LinkIn8           (PLinkVec[ 89:80]),
    .LinkIn9           (PLinkVec[ 99:90]),
    .LinkIn10          (PLinkVec[109:100]),
    .LinkIn11          (PLinkVec[119:110]),
    .LinkIn12          (PLinkVec[129:120]),
    .LinkIn13          (PLinkVec[139:130]),
    .LinkIn14          (PLinkVec[149:140]),
    .LinkIn15          (PLinkVec[159:150])
  );


  Serialiser serdes (
    .SerClk            (SerClk),
    .BitReverse        (1'b0),

    .ParInVec          (160'h0),
    .SerOut            (),
    .SerIn             (LinkIn),
    .ParOut            (PLinkVec)
  );

endmodule