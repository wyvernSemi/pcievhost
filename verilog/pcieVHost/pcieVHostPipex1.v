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
#(parameter NodeNum   = 8,
            EndPoint  = 1,
            DataWidth = 8  // 8, 16, 32 or 64 only
)
(
  input                          pcieclk, // For GEN1 = 250MHz, for GEN2 = 500MHz
  input                          pclk,    // pcieclk /(DataWidth/8), synchronous
  input                          nreset,

`ifdef VERILATOR
  output                         ElecIdleOut,
  input                          ElecIdleIn,
`endif

  input  [DataWidth-1:0]         RxData,
  input  [(DataWidth/8)-1:0]     RxDataK,

  output reg [DataWidth-1:0]     TxData,
  output reg [(DataWidth/8)-1:0] TxDataK

);


localparam LINKWIDTH         = 1;

wire  [9:0]                  LinkOut;
wire  [9:0]                  LinkIn;
wire                         TxSampleClk;

wire  [DataWidth-1:0]        RxDataSampleShift;
wire  [DataWidth/8-1:0]      RxDataKSampleShift;
wire  [DataWidth-1:0]        TxDataPipeShift;
wire  [DataWidth/8-1:0]      TxDataKPipeShift;

reg   [DataWidth-1:0]        RxDataSample;
reg   [DataWidth/8-1:0]      RxDataKSample;
reg   [DataWidth-1:0]        TxDataPipe;
reg   [DataWidth/8-1:0]      TxDataKPipe;

integer                      rxcount;

`ifdef VERILATOR
wire [15:0] ElecIdleOutInt;
assign ElecIdleOut            = ElecIdleOutInt[0];
`endif

// If DataWidth is 8 use the pcieclk and make the pclk unused
// (for backward compatibility)
generate
  if (DataWidth == 8)
  begin
    assign TxSampleClk        = pcieclk;
    assign RxDataSampleShift  = RxData;
    assign RxDataKSampleShift = RxDataK;
    assign TxDataPipeShift    = LinkOut[7:0];
    assign TxDataKPipeShift   = LinkOut[8];
  end
  else
  begin
    assign TxSampleClk        = pclk;
    assign RxDataSampleShift  = {8'h00, RxDataSample[DataWidth-1:8]};
    assign RxDataKSampleShift = {1'b0,  RxDataKSample[DataWidth/8-1: 1]};
    assign TxDataPipeShift    = {LinkOut[7:0], TxDataPipe[DataWidth-1:8]};
    assign TxDataKPipeShift   = {LinkOut[8], TxDataKPipe[DataWidth/8-1:1]};
  end
endgenerate

// Link input is the lower bits of the sampled RX input
assign LinkIn                = {1'b0, RxDataKSample[0], RxDataSample[7:0]};

// Process PIPE interface signals
always @(posedge pcieclk or negedge nreset)
begin

  if (nreset == 1'b0)
  begin
    rxcount                  <= 0;
  end
  else
  begin
    // Sample the RX input every PIPE width bytes
    if (rxcount == 0)
    begin
      RxDataSample           <= RxData;
      RxDataKSample          <= RxDataK;
      
      // After sampling, set count to PIPE bytes minus 1
      rxcount                <= (DataWidth/8)-1;
    end
    // When rxcount non-zero, shift right the sample registers and decrement the count
    else
    begin
      RxDataSample           <= RxDataSampleShift;
      RxDataKSample          <= RxDataKSampleShift;
      
      rxcount                <= rxcount - 1;
    end
  end
  
  // At each cycle, shift right TX PIPE registers, adding link output to top
  TxDataPipe                 <= TxDataPipeShift;
  TxDataKPipe                <= TxDataKPipeShift;

end

// At the PIPE clock rate, update TX outputs with TX PIPE data
always @(posedge TxSampleClk)
begin
  TxData                     <= TxDataPipe;
  TxDataK                    <= TxDataKPipe;
end

  // pcievhost configured with x1 link
  PcieVhost #(LINKWIDTH, NodeNum, EndPoint) pcievh_i
  (
    .Clk                   (pcieclk),
    .notReset              (nreset),

    
`ifdef VERILATOR
    .ElecIdleOut           (ElecIdleOutInt),
    .ElecIdleIn            ({15'h0000, ElecIdleIn}),
`endif

     // Link lane input
    .LinkIn0               (LinkIn),

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