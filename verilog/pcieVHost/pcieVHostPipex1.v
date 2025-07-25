//=============================================================
//
// Copyright (c) 2025 Simon Southwell. All rights reserved.
//
// Date: 25th July 2025
//
// This file is part of the pcieVHost package.
//
// The code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

`ifdef VPROC_SV
`include "allheaders.v"
`endif

`WsTimeScale

module pcieVHostPipex1
#(parameter NodeNum  = 8,
            EndPoint = 1)
(
  input                    pclk,
  input                    nreset,
  
`ifdef VERILATOR
  output                   ElecIdleOut,
  input                    ElecIdleIn,
`endif

  input  [7:0]             RxData,
  input                    RxDataK,

  output [7:0]             TxData,
  output                   TxDataK

);

localparam LINKWIDTH          = 1;

wire  [9:0] LinkOut;

assign TxData                 = LinkOut[7:0];
assign TxDataK                = LinkOut[8];

`ifdef VERILATOR
wire [15:0] ElecIdleOutInt;
assign ElecIdleOut            = ElecIdleOutInt[0];
`endif

  // pcievhost configured with x1 link
  PcieVhost #(LINKWIDTH, NodeNum, EndPoint) ep
  (
    .Clk                   (pclk),
    .notReset              (nreset),
    
`ifdef VERILATOR
    .ElecIdleOut           (ElecIdleOutInt),
    .ElecIdleIn            ({15'h0000, ElecIdleIn}),
`endif

     // Link lane input
    .LinkIn0               ({1'b0, RxDataK, RxData}),
    
    // Unused inputs
    .LinkIn1               ({10{1'bZ}}), .LinkIn2  ({10{1'bZ}}), .LinkIn3  ({10{1'bZ}}),
    .LinkIn4               ({10{1'bZ}}), .LinkIn5  ({10{1'bZ}}), .LinkIn6  ({10{1'bZ}}),
    .LinkIn7               ({10{1'bZ}}), .LinkIn8  ({10{1'bZ}}), .LinkIn9  ({10{1'bZ}}),
    .LinkIn10              ({10{1'bZ}}), .LinkIn11 ({10{1'bZ}}), .LinkIn12 ({10{1'bZ}}),
    .LinkIn13              ({10{1'bZ}}), .LinkIn14 ({10{1'bZ}}), .LinkIn15 ({10{1'bZ}}),

    // Link lane output
    .LinkOut0              (LinkOut),
    
    // Unused outputs
    .LinkOut1              (), .LinkOut2  (), .LinkOut3  (),
    .LinkOut4              (), .LinkOut5  (), .LinkOut6  (),
    .LinkOut7              (), .LinkOut8  (), .LinkOut9  (),
    .LinkOut10             (), .LinkOut11 (), .LinkOut12 (),
    .LinkOut13             (), .LinkOut14 (), .LinkOut15 ()
  );

endmodule