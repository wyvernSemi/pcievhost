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
// $Id: test.v,v 1.3 2016/10/10 11:55:14 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/test/test.v,v $
//
//=============================================================

`WsTimeScale

//-------------------------------------------------------------
//-------------------------------------------------------------
module test;

export "DPI-C" task PcieGetReset;

reg     Clk;
integer Count;

wire #`RegDel notReset = (Count > 10);

wire   [9:0] LinkDown0,  LinkDown1,  LinkDown2,  LinkDown3;
wire   [9:0] LinkDown4,  LinkDown5,  LinkDown6,  LinkDown7;
wire   [9:0] LinkDown8,  LinkDown9,  LinkDown10, LinkDown11;
wire   [9:0] LinkDown12, LinkDown13, LinkDown14, LinkDown15;

wire   [9:0] LinkUp0,    LinkUp1,    LinkUp2,    LinkUp3;
wire   [9:0] LinkUp4,    LinkUp5,    LinkUp6,    LinkUp7;
wire   [9:0] LinkUp8,    LinkUp9,    LinkUp10,   LinkUp11;
wire   [9:0] LinkUp12,   LinkUp13,   LinkUp14,   LinkUp15;

// Bundle the links for the Disps
wire [159:0] DownLink = {LinkDown15, LinkDown14, LinkDown13, LinkDown12,
                         LinkDown11, LinkDown10, LinkDown9,  LinkDown8,
                         LinkDown7,  LinkDown6,  LinkDown5,  LinkDown4,
                         LinkDown3,  LinkDown2,  LinkDown1,  LinkDown0};

wire [159:0] UpLink   = {LinkUp15,   LinkUp14,   LinkUp13,   LinkUp12,
                         LinkUp11,   LinkUp10,   LinkUp9,    LinkUp8,
                         LinkUp7,    LinkUp6,    LinkUp5,    LinkUp4,
                         LinkUp3,    LinkUp2,    LinkUp1,    LinkUp0};

wire [`DispDataInBits]  DispDataIn;
wire [`DispDataOutBits] DispDataOut;
wire [`DispBits]        DispVal;

wire [31:0] LinkWidth            = `PCIE_NUM_PHY_LANES;
wire [31:0] NodeNumDown          = `VPCIE_HOST_NODE_NUM;
wire [31:0] NodeNumUp            = `VPCIE_EP_NODE_NUM;

wire        DisableScrambleDown  = 1'b0;
wire        DisableScrambleUp    = 1'b0;
wire [15:0] InvertTxPolarityDown = 16'h0000;
wire [15:0] InvertTxPolarityUp   = 16'h0000;

task PcieGetReset(output int nRstVal);
  $display("PcieGetReset");
  nRstVal = notReset;
endtask


 PcieDispLink #(`PCIE_NUM_PHY_LANES)
   dispd (.ExtClk(Clk),
          .Link             (DownLink[`PCIE_NUM_PHY_LANES*10-1:0]),
          .notReset         (notReset),
          .FwdName          ("D"),
          .BckName          ("U"),
          .DispDataIn       (DispDataIn),
          .DispDataOut      (DispDataOut),
          .DispValIn        (DispVal),
          .LinkWidth        (LinkWidth[4:0]),
          .DisableScramble  (DisableScrambleDown),
          .InvertTxPolarity (InvertTxPolarityDown),
          .NodeNum          (NodeNumDown[7:0])
          );

 PcieDispLink #(`PCIE_NUM_PHY_LANES)
   dispu (.ExtClk(Clk),
          .Link             (UpLink[`PCIE_NUM_PHY_LANES*10-1:0]),
          .notReset         (notReset),
          .FwdName          ("U"),
          .BckName          ("D"),
          .DispDataOut      (DispDataIn),
          .DispDataIn       (DispDataOut),
          .DispValIn        (DispVal),
          .LinkWidth        (LinkWidth[4:0]),
          .DisableScramble  (DisableScrambleUp),
          .InvertTxPolarity (InvertTxPolarityUp),
          .NodeNum          (NodeNumUp[7:0])
          );


 // Host

 PcieVhostRc #(`PCIE_NUM_PHY_LANES, `VPCIE_HOST_NODE_NUM)
   host (.Clk       (Clk),
         .notReset  (notReset),

         .LinkIn0   (LinkUp0),    .LinkIn1   (LinkUp1),    .LinkIn2   (LinkUp2),    .LinkIn3   (LinkUp3),
         .LinkIn4   (LinkUp4),    .LinkIn5   (LinkUp5),    .LinkIn6   (LinkUp6),    .LinkIn7   (LinkUp7),
         .LinkIn8   (LinkUp8),    .LinkIn9   (LinkUp9),    .LinkIn10  (LinkUp10),   .LinkIn11  (LinkUp11),
         .LinkIn12  (LinkUp12),   .LinkIn13  (LinkUp13),   .LinkIn14  (LinkUp14),   .LinkIn15  (LinkUp15),

         .LinkOut0  (LinkDown0),  .LinkOut1  (LinkDown1),  .LinkOut2  (LinkDown2),  .LinkOut3  (LinkDown3),
         .LinkOut4  (LinkDown4),  .LinkOut5  (LinkDown5),  .LinkOut6  (LinkDown6),  .LinkOut7  (LinkDown7),
         .LinkOut8  (LinkDown8),  .LinkOut9  (LinkDown9),  .LinkOut10 (LinkDown10), .LinkOut11 (LinkDown11),
         .LinkOut12 (LinkDown12), .LinkOut13 (LinkDown13), .LinkOut14 (LinkDown14), .LinkOut15 (LinkDown15)
       );

 // Endpoint
 PcieVhostEp #(`PCIE_NUM_PHY_LANES, `VPCIE_EP_NODE_NUM)
   ep   (.Clk       (Clk),
         .notReset  (notReset),

         .LinkIn0   (LinkDown0),  .LinkIn1   (LinkDown1),  .LinkIn2   (LinkDown2),  .LinkIn3   (LinkDown3),
         .LinkIn4   (LinkDown4),  .LinkIn5   (LinkDown5),  .LinkIn6   (LinkDown6),  .LinkIn7   (LinkDown7),
         .LinkIn8   (LinkDown8),  .LinkIn9   (LinkDown9),  .LinkIn10  (LinkDown10), .LinkIn11  (LinkDown11),
         .LinkIn12  (LinkDown12), .LinkIn13  (LinkDown13), .LinkIn14  (LinkDown14), .LinkIn15  (LinkDown15),

         .LinkOut0  (LinkUp0),    .LinkOut1  (LinkUp1),    .LinkOut2  (LinkUp2),    .LinkOut3  (LinkUp3),
         .LinkOut4  (LinkUp4),    .LinkOut5  (LinkUp5),    .LinkOut6  (LinkUp6),    .LinkOut7  (LinkUp7),
         .LinkOut8  (LinkUp8),    .LinkOut9  (LinkUp9),    .LinkOut10 (LinkUp10),   .LinkOut11 (LinkUp11),
         .LinkOut12 (LinkUp12),   .LinkOut13 (LinkUp13),   .LinkOut14 (LinkUp14),   .LinkOut15 (LinkUp15)
       );

 // Control display module
 ContDisps cd (Clk, DispVal);

initial
begin
    Clk = 1;

    #0                  // Ensure first x->1 clock edge is complete before initialisation
    Count = 0;
    forever # (`CLK_PERIOD/2) Clk = ~Clk;
end

always @(posedge Clk)
begin
    Count = Count + 1;
    if (Count == `TIMEOUT_COUNT)
    begin
        `fatal
    end
end

// Top level fatal task, which can be called from anywhere in verilog code.
// via the `fatal definition in pciedispheader.v. Any data logging, error
// message displays etc., on a fatal, should be placed in here.
task Fatal;
begin
    $display("***FATAL ERROR...calling $finish!");
    $finish;
end
endtask
endmodule

