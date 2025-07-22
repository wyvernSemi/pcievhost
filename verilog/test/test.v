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

`ifdef VPROC_SV
`include "allheaders.v"
`endif

`WsTimeScale

//-------------------------------------------------------------
//-------------------------------------------------------------
module test
#(parameter VCD_DUMP       = 0,
  parameter DEBUG_STOP     = 0,
  parameter PIPE           = 0
);

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

wire [15:0] ElecIdleUp, ElecIdleDown;

wire [`DispDataInBits]  DispDataIn;
wire [`DispDataOutBits] DispDataOut;
wire [`DispBits]        DispVal;

wire [31:0] LinkWidth            = `PCIE_NUM_PHY_LANES;
wire [31:0] NodeNumDown          = `VPCIE_HOST_NODE_NUM;
wire [31:0] NodeNumUp            = `VPCIE_EP_NODE_NUM;

wire        DisableScrambleDown  = (PIPE != 0) ? 1'b1 : 1'b0;
wire        DisableScrambleUp    = (PIPE != 0) ? 1'b1 : 1'b0;
wire        Disable8b10bDown     = (PIPE != 0) ? 1'b1 : 1'b0;
wire        Disable8b10bUp       = (PIPE != 0) ? 1'b1 : 1'b0;
wire [15:0] InvertTxPolarityDown = 16'h0000;
wire [15:0] InvertTxPolarityUp   = 16'h0000;

`ifdef VERILATOR
reg [160:0] DownLink;
reg [160:0] UpLink;

reg   [9:0] IntLinkDown0,  IntLinkDown1,  IntLinkDown2,  IntLinkDown3;
reg   [9:0] IntLinkDown4,  IntLinkDown5,  IntLinkDown6,  IntLinkDown7;
reg   [9:0] IntLinkDown8,  IntLinkDown9,  IntLinkDown10, IntLinkDown11;
reg   [9:0] IntLinkDown12, IntLinkDown13, IntLinkDown14, IntLinkDown15;

reg   [9:0] IntLinkUp0,    IntLinkUp1,    IntLinkUp2,    IntLinkUp3;
reg   [9:0] IntLinkUp4,    IntLinkUp5,    IntLinkUp6,    IntLinkUp7;
reg   [9:0] IntLinkUp8,    IntLinkUp9,    IntLinkUp10,   IntLinkUp11;
reg   [9:0] IntLinkUp12,   IntLinkUp13,   IntLinkUp14,   IntLinkUp15;

always @(negedge Clk)
begin
  DownLink      <= {LinkDown15, LinkDown14, LinkDown13, LinkDown12,
                    LinkDown11, LinkDown10, LinkDown9,  LinkDown8,
                    LinkDown7,  LinkDown6,  LinkDown5,  LinkDown4,
                    LinkDown3,  LinkDown2,  LinkDown1,  LinkDown0};

  UpLink        <= {LinkUp15,   LinkUp14,   LinkUp13,   LinkUp12,
                    LinkUp11,   LinkUp10,   LinkUp9,    LinkUp8,
                    LinkUp7,    LinkUp6,    LinkUp5,    LinkUp4,
                    LinkUp3,    LinkUp2,    LinkUp1,    LinkUp0};

  IntLinkDown0  <= LinkDown0;
  IntLinkDown1  <= LinkDown1;
  IntLinkDown2  <= LinkDown2;
  IntLinkDown3  <= LinkDown3;
  IntLinkDown4  <= LinkDown4;
  IntLinkDown5  <= LinkDown5;
  IntLinkDown6  <= LinkDown6;
  IntLinkDown7  <= LinkDown7;
  IntLinkDown8  <= LinkDown8;
  IntLinkDown9  <= LinkDown9;
  IntLinkDown10 <= LinkDown10;
  IntLinkDown11 <= LinkDown11;
  IntLinkDown12 <= LinkDown12;
  IntLinkDown13 <= LinkDown13;
  IntLinkDown14 <= LinkDown14;
  IntLinkDown15 <= LinkDown15;

  IntLinkUp0    <= LinkUp0;
  IntLinkUp1    <= LinkUp1;
  IntLinkUp2    <= LinkUp2;
  IntLinkUp3    <= LinkUp3;
  IntLinkUp4    <= LinkUp4;
  IntLinkUp5    <= LinkUp5;
  IntLinkUp6    <= LinkUp6;
  IntLinkUp7    <= LinkUp7;
  IntLinkUp8    <= LinkUp8;
  IntLinkUp9    <= LinkUp9;
  IntLinkUp10   <= LinkUp10;
  IntLinkUp11   <= LinkUp11;
  IntLinkUp12   <= LinkUp12;
  IntLinkUp13   <= LinkUp13;
  IntLinkUp14   <= LinkUp14;
  IntLinkUp15   <= LinkUp15;

end

`else
// Bundle the links for the Disps
wire [159:0] DownLink = {LinkDown15, LinkDown14, LinkDown13, LinkDown12,
                         LinkDown11, LinkDown10, LinkDown9,  LinkDown8,
                         LinkDown7,  LinkDown6,  LinkDown5,  LinkDown4,
                         LinkDown3,  LinkDown2,  LinkDown1,  LinkDown0};

wire [159:0] UpLink   = {LinkUp15,   LinkUp14,   LinkUp13,   LinkUp12,
                         LinkUp11,   LinkUp10,   LinkUp9,    LinkUp8,
                         LinkUp7,    LinkUp6,    LinkUp5,    LinkUp4,
                         LinkUp3,    LinkUp2,    LinkUp1,    LinkUp0};

wire [9:0] IntLinkDown0   = LinkDown0;
wire [9:0] IntLinkDown1   = LinkDown1;
wire [9:0] IntLinkDown2   = LinkDown2;
wire [9:0] IntLinkDown3   = LinkDown3;
wire [9:0] IntLinkDown4   = LinkDown4;
wire [9:0] IntLinkDown5   = LinkDown5;
wire [9:0] IntLinkDown6   = LinkDown6;
wire [9:0] IntLinkDown7   = LinkDown7;
wire [9:0] IntLinkDown8   = LinkDown8;
wire [9:0] IntLinkDown9   = LinkDown9;
wire [9:0] IntLinkDown10  = LinkDown10;
wire [9:0] IntLinkDown11  = LinkDown11;
wire [9:0] IntLinkDown12  = LinkDown12;
wire [9:0] IntLinkDown13  = LinkDown13;
wire [9:0] IntLinkDown14  = LinkDown14;
wire [9:0] IntLinkDown15  = LinkDown15;

wire [9:0] IntLinkUp0     = LinkUp0;
wire [9:0] IntLinkUp1     = LinkUp1;
wire [9:0] IntLinkUp2     = LinkUp2;
wire [9:0] IntLinkUp3     = LinkUp3;
wire [9:0] IntLinkUp4     = LinkUp4;
wire [9:0] IntLinkUp5     = LinkUp5;
wire [9:0] IntLinkUp6     = LinkUp6;
wire [9:0] IntLinkUp7     = LinkUp7;
wire [9:0] IntLinkUp8     = LinkUp8;
wire [9:0] IntLinkUp9     = LinkUp9;
wire [9:0] IntLinkUp10    = LinkUp10;
wire [9:0] IntLinkUp11    = LinkUp11;
wire [9:0] IntLinkUp12    = LinkUp12;
wire [9:0] IntLinkUp13    = LinkUp13;
wire [9:0] IntLinkUp14    = LinkUp14;
wire [9:0] IntLinkUp15    = LinkUp15;

`endif


`ifndef DISABLE_PCIEDISPLINK
 PcieDispLink #(`PCIE_NUM_PHY_LANES) dispd (.ExtClk(Clk),
                                            .Link             (DownLink[`PCIE_NUM_PHY_LANES*10-1:0]),
                                            .notReset         (notReset),
                                            .FwdName          ("D"),
                                            .BckName          ("U"),
                                            .DispDataIn       (DispDataIn),
                                            .DispDataOut      (DispDataOut),
                                            .DispValIn        (DispVal),
                                            .LinkWidth        (LinkWidth[4:0]),
                                            .DisableScramble  (DisableScrambleDown),
                                            .Disable8b10b     (Disable8b10bDown),
                                            .InvertTxPolarity (InvertTxPolarityDown),
                                            .NodeNum          (NodeNumDown[7:0])
                                            );

 PcieDispLink #(`PCIE_NUM_PHY_LANES) dispu (.ExtClk(Clk),
                                            .Link             (UpLink[`PCIE_NUM_PHY_LANES*10-1:0]),
                                            .notReset         (notReset),
                                            .FwdName          ("U"),
                                            .BckName          ("D"),
                                            .DispDataOut      (DispDataIn),
                                            .DispDataIn       (DispDataOut),
                                            .DispValIn        (DispVal),
                                            .LinkWidth        (LinkWidth[4:0]),
                                            .DisableScramble  (DisableScrambleUp),
                                            .Disable8b10b     (Disable8b10bUp),
                                            .InvertTxPolarity (InvertTxPolarityUp),
                                            .NodeNum          (NodeNumUp[7:0])
                                            );

`endif

 // Host
 PcieVhost #(`PCIE_NUM_PHY_LANES, `VPCIE_HOST_NODE_NUM, 0)
                                      host (.Clk              (Clk),
                                            .notReset         (notReset),
`ifdef VERILATOR
                                            .ElecIdleOut      (ElecIdleDown),
                                            .ElecIdleIn       (ElecIdleUp),
`endif

                                            .LinkIn0          (IntLinkUp0),
                                            .LinkIn1          (IntLinkUp1),
                                            .LinkIn2          (IntLinkUp2),
                                            .LinkIn3          (IntLinkUp3),
                                            .LinkIn4          (IntLinkUp4),
                                            .LinkIn5          (IntLinkUp5),
                                            .LinkIn6          (IntLinkUp6),
                                            .LinkIn7          (IntLinkUp7),
                                            .LinkIn8          (IntLinkUp8),
                                            .LinkIn9          (IntLinkUp9),
                                            .LinkIn10         (IntLinkUp10),
                                            .LinkIn11         (IntLinkUp11),
                                            .LinkIn12         (IntLinkUp12),
                                            .LinkIn13         (IntLinkUp13),
                                            .LinkIn14         (IntLinkUp14),
                                            .LinkIn15         (IntLinkUp15),

                                            .LinkOut0         (LinkDown0),
                                            .LinkOut1         (LinkDown1),
                                            .LinkOut2         (LinkDown2),
                                            .LinkOut3         (LinkDown3),
                                            .LinkOut4         (LinkDown4),
                                            .LinkOut5         (LinkDown5),
                                            .LinkOut6         (LinkDown6),
                                            .LinkOut7         (LinkDown7),
                                            .LinkOut8         (LinkDown8),
                                            .LinkOut9         (LinkDown9),
                                            .LinkOut10        (LinkDown10),
                                            .LinkOut11        (LinkDown11),
                                            .LinkOut12        (LinkDown12),
                                            .LinkOut13        (LinkDown13),
                                            .LinkOut14        (LinkDown14),
                                            .LinkOut15        (LinkDown15)
                                            );

 // Endpoint
 PcieVhost #(`PCIE_NUM_PHY_LANES, `VPCIE_EP_NODE_NUM, 1)
                                      ep   (.Clk              (Clk),
                                            .notReset         (notReset),
`ifdef VERILATOR
                                            .ElecIdleOut      (ElecIdleUp),
                                            .ElecIdleIn       (ElecIdleDown),
`endif
                                            .LinkIn0          (IntLinkDown0),
                                            .LinkIn1          (IntLinkDown1),
                                            .LinkIn2          (IntLinkDown2),
                                            .LinkIn3          (IntLinkDown3),
                                            .LinkIn4          (IntLinkDown4),
                                            .LinkIn5          (IntLinkDown5),
                                            .LinkIn6          (IntLinkDown6),
                                            .LinkIn7          (IntLinkDown7),
                                            .LinkIn8          (IntLinkDown8),
                                            .LinkIn9          (IntLinkDown9),
                                            .LinkIn10         (IntLinkDown10),
                                            .LinkIn11         (IntLinkDown11),
                                            .LinkIn12         (IntLinkDown12),
                                            .LinkIn13         (IntLinkDown13),
                                            .LinkIn14         (IntLinkDown14),
                                            .LinkIn15         (IntLinkDown15),

                                            .LinkOut0         (LinkUp0),
                                            .LinkOut1         (LinkUp1),
                                            .LinkOut2         (LinkUp2),
                                            .LinkOut3         (LinkUp3),
                                            .LinkOut4         (LinkUp4),
                                            .LinkOut5         (LinkUp5),
                                            .LinkOut6         (LinkUp6),
                                            .LinkOut7         (LinkUp7),
                                            .LinkOut8         (LinkUp8),
                                            .LinkOut9         (LinkUp9),
                                            .LinkOut10        (LinkUp10),
                                            .LinkOut11        (LinkUp11),
                                            .LinkOut12        (LinkUp12),
                                            .LinkOut13        (LinkUp13),
                                            .LinkOut14        (LinkUp14),
                                            .LinkOut15        (LinkUp15)
                                            );

 // Control display module
 ContDisps cd (Clk, DispVal);

initial
begin
  // If specified, dumpa VCD file
  if (VCD_DUMP != 0)
  begin
    $dumpfile("waves.vcd");
    $dumpvars(0, test);
  end

    Clk = 1;

`ifndef VERILATOR
    #0                  // Ensure first x->1 clock edge is complete before initialisation
`endif

    // If specified, stop for debugger attcahement
    if (DEBUG_STOP != 0)
    begin
      $display("\n***********************************************");
      $display("* Stopping simulation for debugger attachment *");
      $display("***********************************************\n");
      $stop;
    end

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

