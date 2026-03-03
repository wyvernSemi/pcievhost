//=============================================================
//
// Copyright (c) 2023 - 2026 Simon Southwell. All rights reserved.
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

`include "allheaders.v"

`WsTimeScale

//-------------------------------------------------------------
// Top level SystemVerilog PcieVhost test bench
//-------------------------------------------------------------

module test
#(
  parameter VCD_DUMP       = 0,
  parameter PIPE           = 0,
  parameter DEBUG_STOP     = 0
);

//-------------------------------------------------------------
// Signal declarations
//-------------------------------------------------------------
reg             Clk;
integer         Count;

PcieLinkLanes16 LinkUp();
PcieLinkLanes16 LinkDown();

//-------------------------------------------------------------
// Generate a rest signal
//-------------------------------------------------------------

wire #`RegDel notReset = (Count > 10);


//-------------------------------------------------------------
// Exported DPI task declartions
//-------------------------------------------------------------
export "DPI-C" task PcieGetReset;

task PcieGetReset(output int nRstVal);
  $display("PcieGetReset");
  nRstVal = notReset;
endtask

//-------------------------------------------------------------
// Host
//-------------------------------------------------------------

 PcieVhostRc #(`PCIE_NUM_PHY_LANES, `VPCIE_HOST_NODE_NUM)
   host (.Clk         (Clk),
         .notReset    (notReset),

         .LinkIn      (LinkUp),
         .LinkOut     (LinkDown)
       );

//-------------------------------------------------------------
// Endpoint
//-------------------------------------------------------------

 PcieVhostEp #(`PCIE_NUM_PHY_LANES, `VPCIE_EP_NODE_NUM)
   ep   (.Clk         (Clk),
         .notReset    (notReset),

         .LinkIn      (LinkDown),
         .LinkOut     (LinkUp)
       );

//-------------------------------------------------------------
// Initialisation and clock generation
//-------------------------------------------------------------
initial
begin
  // If specified, dumpa VCD file
  if (VCD_DUMP != 0)
  begin
    $dumpfile("waves.vcd");
    $dumpvars(0, test);
  end

  Clk = 1;

  #0                  // Ensure first x->1 clock edge is complete before initialisation

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

//-------------------------------------------------------------
// Counter and timeout control
//-------------------------------------------------------------

always @(posedge Clk)
begin
    Count = Count + 1;
    if (Count == `TIMEOUT_COUNT)
    begin
        `fatal
    end
end

//-------------------------------------------------------------
// Top level fatal task, which can be called from anywhere in
// verilog code via the `fatal definition in test_defs.v. Any
// data logging, error message displays etc., on a fatal, 
// should be placed in here.
//-------------------------------------------------------------
task Fatal;
begin
    $display("***FATAL ERROR...calling $finish!");
    $finish;
end
endtask
endmodule

