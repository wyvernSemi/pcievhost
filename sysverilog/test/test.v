//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Sep 2023
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

`WsTimeScale

//-------------------------------------------------------------
//-------------------------------------------------------------
module test;

export "DPI-C" task PcieGetReset;

reg     Clk;
integer Count;

PcieLinkLanes16 LinkUp();
PcieLinkLanes16 LinkDown();

wire #`RegDel notReset = (Count > 10);

// Bundle the links for the Disps
wire [159:0] DownLink = {LinkDown.Lane15, LinkDown.Lane14, LinkDown.Lane13, LinkDown.Lane12,
                         LinkDown.Lane11, LinkDown.Lane10, LinkDown.Lane9,  LinkDown.Lane8,
                         LinkDown.Lane7,  LinkDown.Lane6,  LinkDown.Lane5,  LinkDown.Lane4,
                         LinkDown.Lane3,  LinkDown.Lane2,  LinkDown.Lane1,  LinkDown.Lane0};

wire [159:0] UpLink   = {LinkUp.Lane15,   LinkUp.Lane14,   LinkUp.Lane13,   LinkUp.Lane12,
                         LinkUp.Lane11,   LinkUp.Lane10,   LinkUp.Lane9,    LinkUp.Lane8,
                         LinkUp.Lane7,    LinkUp.Lane6,    LinkUp.Lane5,    LinkUp.Lane4,
                         LinkUp.Lane3,    LinkUp.Lane2,    LinkUp.Lane1,    LinkUp.Lane0};

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

         .LinkIn(LinkUp),
         .LinkOut(LinkDown)
       );

 // Endpoint
 PcieVhostEp #(`PCIE_NUM_PHY_LANES, `VPCIE_EP_NODE_NUM)
   ep   (.Clk       (Clk),
         .notReset  (notReset),

         .LinkIn(LinkDown), 
         .LinkOut(LinkUp)
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

