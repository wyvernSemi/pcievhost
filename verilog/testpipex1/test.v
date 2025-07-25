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
  parameter DEBUG_STOP     = 0
);

localparam RcNodeNum       = 0;
localparam EpNodeNum       = 1;
localparam ROOTCMPLX       = 0;
localparam ENDPOINT        = 1;

reg     Clk;
integer Count;

wire [7:0] LinkDownData;
wire       LinkDownDataK;

wire [7:0] LinkUpData;
wire       LinkUpDataK;

wire  ElecIdleUp, ElecIdleDown;

// Generate a reset signal
wire #`RegDel notReset = (Count > 10);

`ifdef VERILATOR
reg  [7:0] LinkDownDataInt;
reg        LinkDownDataKInt;

reg  [7:0] LinkUpDataInt;
reg        LinkUpDataKInt;

always @(negedge Clk)
begin

  LinkDownDataInt            <= LinkDownData;
  LinkDownDataKInt           <= LinkDownDataK;
  LinkUpDataInt              <= LinkUpData;
  LinkUpDataKInt             <= LinkUpDataK;
end

`else

wire [7:0] LinkDownDataInt   = LinkDownData;
wire       LinkDownDataKInt  = LinkDownDataK;

wire [7:0] LinkUpDataInt     = LinkUpData;
wire       LinkUpDataKInt    = LinkUpDataK;

`endif
    pcieVHostPipex1 #(RcNodeNum, ROOTCMPLX) rc
    (
       .pclk                (Clk),
       .nreset              (notReset),

`ifdef VERILATOR
       .ElecIdleOut         (ElecIdleDown),
       .ElecIdleIn          (ElecIdleUp),
`endif
       
       .TxData              (LinkDownData),
       .TxDataK             (LinkDownDataK),
       
       .RxData              (LinkUpDataInt),
       .RxDataK             (LinkUpDataKInt)
    );
    
    pcieVHostPipex1 #(EpNodeNum, ENDPOINT) ep
    (
       .pclk                (Clk),
       .nreset              (notReset),

`ifdef VERILATOR
       .ElecIdleOut         (ElecIdleUp),
       .ElecIdleIn          (ElecIdleDown),
`endif 
 
       .TxData              (LinkUpData),
       .TxDataK             (LinkUpDataK),
       
       .RxData              (LinkDownDataInt),
       .RxDataK             (LinkDownDataKInt)
    );

initial
begin
  // If specified, dump a VCD file
  if (VCD_DUMP != 0)
  begin
    $dumpfile("waves.vcd");
    $dumpvars(0, test);
  end

    Clk = 1;

`ifndef VERILATOR
    #0                  // Ensure first x->1 clock edge is complete before initialisation
`endif

    // If specified, stop for debugger attachement
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

