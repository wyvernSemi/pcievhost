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
//=============================================================

//=============================================================
// A PCI Express single lane disp logic and display.
//=============================================================

`ifdef VPROC_SV
`include "allheaders.v"
`endif

`WsTimeScale

module PcieDispLinkLane(Link, Clk, RxByte, RxByteRaw, RxControl, Synced, notReset, FwdName, DispVal, LaneNum,
                        Enable, DisableScramble, Disable8b10b, InvertTxPolarity, NodeNum);

input  [9:0]       Link; 
input  [7:0]       NodeNum;
input              Clk;
input              Enable;
input              DisableScramble;
input              Disable8b10b;
input              InvertTxPolarity;

input              notReset;
input  [7:0]       FwdName;
input  [`DispBits] DispVal;
input  [4:0]       LaneNum;

output [7:0]       RxByte;
output [7:0]       RxByteRaw;
output             RxControl;
output             Synced;

wire               ElecIdleOrderedSet;
wire               FtsOrderedSet;
wire               SkpOrderedSet;
wire   [1:0]       RxTrainingSeq; 
wire               BadDisparity;
wire               BadRunLength;
wire               RxCommaReg;
wire   [9:0]       LoadReg;

     // Simple model to pull data off the link, generate a clock and give 
     // sync bit and symbol status. Will need to be a bit more sophisticated.
assign Synced       = notReset;
assign LoadReg      = Link;
     
     RxLaneDisp rln (.LinkIn             (LoadReg),
                     .notReset           (notReset),
                     .Clk                (Clk),
                     .RxByte             (RxByte),
                     .RxByteRaw          (RxByteRaw),
                     .RxControl          (RxControl),
                     .Synced             (Synced),
                     .DisableScramble    (DisableScramble),
                     .Disable8b10b       (Disable8b10b),
                     .InvertTxPolarity   (InvertTxPolarity),
                     .RxTrainingSeq      (RxTrainingSeq),
                     .ElecIdleOrderedSet (ElecIdleOrderedSet),
                     .FtsOrderedSet      (FtsOrderedSet),
                     .SkpOrderedSet      (SkpOrderedSet)
                     );

// These signals are only used by the Disp link, but mess up 
// the RxLogic module's interface
assign BadDisparity = rln.rxl.BadDisparity;
assign BadRunLength = rln.rxl.BadRunLength;
assign RxCommaReg   = rln.rxl.RxCommaReg;
   
// Buffer and index for training sequences
reg [7:0] TSBuf [0:15];
integer TSIdx;
integer SeenOS;

initial
begin
   TSIdx = 0;
   SeenOS = 0;
end

always @(posedge Clk)
begin
  if (Enable)
  begin
    // Check for lane token errors
    if (BadDisparity)
    begin
        $write("PHY.2.1#1 - Invalid symbol and/or bad disparity");
    end

    // Check for lane run-length errors
    if (BadRunLength)
    begin
        $write("PHY.2.1#1 - Invalid symbol and/or bad disparity");
    end

    #1
    if (DispVal[`DispPL])
    begin
        TSIdx = (RxCommaReg || TSIdx == 15) ? 0 : (TSIdx + 1);
        TSBuf[TSIdx] = RxByte;
        if (SeenOS === 1 && ^RxByte === 1'bx)
        begin
`ifndef DISABLE_PCIE_FATAL_ON_X
            $display("PCIE%c%0d %0d: ***Error --- Seen X after synchronisation", FwdName, NodeNum, LaneNum);
            `fatal
`else
        TSIdx = 0;
        SeenOS = 0;
`endif
        end
        if (|RxTrainingSeq)
        begin
            if (LaneNum < 10)
                $write("PCIE%c%0d 0%0d: PL TS%0d OS ",  FwdName, NodeNum, LaneNum, RxTrainingSeq[0] ? 1 : 2);
            else
                $write("PCIE%c%0d %0d: PL TS%0d OS ",   FwdName, NodeNum, LaneNum, RxTrainingSeq[0] ? 1 : 2);
            if (TSBuf[`TSX_LINKNUM] == `PAD)
                $write("Link=PAD ");
            else
                $write("Link=  %0d ", TSBuf[`TSX_LINKNUM]);
            if (TSBuf[`TSX_LANENUM] == `PAD)
                $write("Lane=PAD ");
            else
                if (TSBuf[`TSX_LANENUM] < 10)
                    $write("Lane=  %0d ",     TSBuf[`TSX_LANENUM]);
                else
                    $write("Lane= %0d ",      TSBuf[`TSX_LANENUM]);
            if (TSBuf[`TSX_N_FTS] < 10)
                $write  ("N_FTS= %0d ",        TSBuf[`TSX_N_FTS]);
            else
                $write  ("N_FTS=%0d ",         TSBuf[`TSX_N_FTS]);
            if (TSBuf[`TSX_DATARATE] == `TSX_DATARATE_GEN2)
                $write  ("DataRate=GEN2 ");
            else if (TSBuf[`TSX_DATARATE] == `TSX_DATARATE_GEN1)
                $write  ("DataRate=GEN1 ");
            else
                $write  ("DataRate=???%0d ",     TSBuf[`TSX_DATARATE]);
            if (((TSBuf[`TSX_LINKCONTROL] & `TSX_LNKCTRL_RESET_MASK) != 8'h00) ? 1'b1 : 1'b0)
                $write  ("AssertReset ");
            if (((TSBuf[`TSX_LINKCONTROL] & `TSX_LNKCTRL_DISABLE_MASK) != 8'h00) ? 1'b1 : 1'b0)
                $write  ("DisableLink ");
            if (((TSBuf[`TSX_LINKCONTROL] & `TSX_LNKCTRL_LOOPBACK_MASK) != 8'h00) ? 1'b1 : 1'b0)
                $write  ("Loopback ");
            if (((TSBuf[`TSX_LINKCONTROL] & `TSX_LNKCTRL_NOSCRAMBLE_MASK) != 8'h00) ? 1'b1 : 1'b0)
                $write  ("NoScramble ");
            if ((RxTrainingSeq[0] == 1'b0) && (((TSBuf[`TSX_LINKCONTROL] & `TSX_LNKCTRL_COMPL_RX_MASK) != 8'h00) ? 1'b1 : 1'b0))
                $write  ("Compliance RX ");
            $display("");
            SeenOS = 1;
        end

        if (ElecIdleOrderedSet)
            $display("PCIE%c%0d %0d: Electrical idle ordered set", FwdName, NodeNum, LaneNum);

        if (FtsOrderedSet)
        begin
            $display("PCIE%c%0d %0d: Fast training sequence ordered set", FwdName, NodeNum, LaneNum);
            SeenOS = 1;
        end

        if (SkpOrderedSet)
        begin
            $display("PCIE%c%0d %0d: Skip ordered set", FwdName, NodeNum, LaneNum);
            SeenOS = 1;
        end
    end
  end
end

endmodule
