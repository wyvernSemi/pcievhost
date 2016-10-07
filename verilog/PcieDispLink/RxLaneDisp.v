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
// $Id: RxLaneDisp.v,v 1.1 2016/10/04 15:47:35 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/PcieDispLink/RxLaneDisp.v,v $
//
//=============================================================

//=============================================================
// A PCI-Express Physical (logical) layer Rx lane
// See Base Specification Revision 1.0a section 4.2
//=============================================================


`WsTimeScale

//-------------------------------------------------------------
// RxLaneDisp
//-------------------------------------------------------------

module RxLaneDisp (LinkIn, notReset, Clk, RxByte, RxByteRaw, RxControl, Synced, DisableScramble, InvertTxPolarity,
                   RxTrainingSeq, ElecIdleOrderedSet, FtsOrderedSet, SkpOrderedSet);

input        Clk;
input  [9:0] LinkIn;
input        notReset;
input        Synced;
input        DisableScramble;
input        InvertTxPolarity;
output [7:0] RxByte;
output [7:0] RxByteRaw;
output       RxControl;

output       ElecIdleOrderedSet;
output       FtsOrderedSet;
output       SkpOrderedSet;
output [1:0] RxTrainingSeq; 

wire   [7:0] DecodeByte;
wire   [7:0] ScXor;
wire         DecodeCtrl;
wire  [15:0] Shift;
wire         notResetScrambler;
wire         MoveScrambler;
wire         Scramble;
wire         TSEvent0;

wire [9:0] LinkData = LinkIn ^ {10{InvertTxPolarity}};

    // Combinatorial decode logic
    Decoder        dc (.Clk(1'b0), 
                       .Input              (LinkData),
                       .BitRev             (1'b0), 
                       .InvertDataIn       (1'b0), 
                       .OutputRaw          (DecodeByte), 
                       .ControlRaw         (DecodeCtrl),
                       .Output             (),
                       .Control            ()
                       );

    // Receiver logic controlling scrambling and flow control, as well as flagging
    // reception events
    RxLogicDisp   rxl (.Clk                (Clk), 
                       .notReset           (notReset),
                       .DecodeCtrl         (DecodeCtrl),
                       .DecodeByte         (DecodeByte),
                       .OutByteRaw         (RxByteRaw),
                       .LinkIn             (LinkIn),
                       .Synced             (Synced),
                       .notResetScrambler  (notResetScrambler),
                       .MoveScrambler      (MoveScrambler),
                       .Scramble           (Scramble),
                       .DisableScramble    (DisableScramble),
                       .ElecIdleOrderedSet (ElecIdleOrderedSet),
                       .FtsOrderedSet      (FtsOrderedSet),
                       .SkpOrderedSet      (SkpOrderedSet),
                       .RxTrainingSeq      (RxTrainingSeq),
                       .RxControl          (RxControl)
                       );

    // Generates scrambling code
    ScrambleCodec rsc (.ClkPci             (Clk),
                       .notResetPci        (notResetScrambler),
                       .MovePipe           (MoveScrambler),
                       .Shift              (Shift),
                       .NextShift          (),
                       .OutShift           (Shift),
                       .NextXorWord        (),
                       .XorWord            (ScXor)
                       );

    // Receiver data path
    RxDpDisp     rxdp (.Clk                (Clk), 
                       .Scramble           (Scramble),
                       .DecodeByte         (DecodeByte),
                       .ScXor              (ScXor),
                       .OutByteRaw         (RxByteRaw), 
                       .OutByteSc          (RxByte)
                       );


endmodule

//-------------------------------------------------------------
// RxDpDisp
//-------------------------------------------------------------

module RxDpDisp (Clk, Scramble, DecodeByte, ScXor, OutByteRaw, OutByteSc);

input        Clk;
input        Scramble;
input  [7:0] DecodeByte;
input  [7:0] ScXor;

output [7:0] OutByteSc;
output [7:0] OutByteRaw;

reg    [7:0] OutByteRaw;

assign OutByteSc = OutByteRaw ^ (ScXor & {8{Scramble}});

always @(posedge Clk)
begin
    OutByteRaw  <= #`RegDel DecodeByte;
end

`ifdef TEST_HARNESS
task DispState;
begin
    $display("RxByte=%h OutByteSc=%h",
              OutByteRaw, OutByteSc);
end
endtask
`endif
endmodule
