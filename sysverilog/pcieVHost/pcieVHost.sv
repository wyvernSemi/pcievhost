//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 7th Mar 2023
//
// This file is part of the pcieVHost package.
//
// This code is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The code is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this code. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

`WsTimeScale

// Define a uni-directional interface of PCIe lanes (unserialised)
interface PcieLinkLanes16;

  logic [9:0] Lane0,   Lane1,   Lane2,   Lane3,
              Lane4,   Lane5,   Lane6,   Lane7,
              Lane8,   Lane9,   Lane10,  Lane11,
              Lane12,  Lane13,  Lane14,  Lane15;

endinterface

//-------------------------------------------------------------
// Root Complex wrapper for PcieVHost
//-------------------------------------------------------------

module PcieVhostRc (input           Clk,       notReset,
                    PcieLinkLanes16 LinkIn,    LinkOut);


parameter  LinkWidth  = 16;
parameter  NodeNum   = 0;
localparam EndPoint  = 0;

// VP Interface wires
reg    [31:0] Addr;
reg    [31:0] DataOut;
reg           WE, RD;
reg           Update;
integer       Ticks;

wire          UpdateResponse;
wire   [31:0] DataIn;

initial
begin
  Update <= 1'b0;
end

// Exported DPI task, called from C code
export "DPI-C" task PcieUpdate0;

task PcieUpdate0(input int addr, input int wdata, output int rdata, input int rnw, input int ticks);
  wait (notReset == 1'b1);
  Addr       = addr;
  DataOut    = wdata;
  WE         = rnw ? 1'b0 : 1'b1;
  RD         = rnw ? 1'b1 : 1'b0;
  Ticks      = ticks;
  Update     = ~Update;
  @UpdateResponse;
  rdata      = DataIn;
endtask

 // Instantiation of the common PcieVHost 
 PcieVhost #(LinkWidth, NodeNum, EndPoint) pcievhost_inst
 (
   .Clk      (Clk),
   .notReset (notReset),

   .LinkIn   (LinkIn),     .LinkOut(LinkOut),

   .WE (WE),               .RD (RD),               .Update (Update),       .Addr (Addr),
   .DataOut(DataOut),      .Ticks(Ticks),          .DataIn(DataIn),        .UpdateResponse(UpdateResponse)
  );

endmodule

//-------------------------------------------------------------
// Enpoint wrapper for PcieVHost
//-------------------------------------------------------------

module PcieVhostEp (input           Clk,       notReset,
                    PcieLinkLanes16 LinkIn,    LinkOut);

parameter  LinkWidth  = 16;
parameter  NodeNum    = 1;
localparam EndPoint   = 1;

// VP Interface wires
reg    [31:0] Addr;
reg    [31:0] DataOut;
reg           WE, RD;
reg           Update;
integer       Ticks;

wire          UpdateResponse;
wire   [31:0] DataIn;

initial
begin
  Update <= 1'b0;
end

// Exported DPI task, called from C code
export "DPI-C" task PcieUpdate1;

task PcieUpdate1(input int addr, input int wdata, output int rdata, input int rnw, input int ticks);
  wait (notReset == 1'b1);
  Addr       = addr;
  DataOut    = wdata;
  WE         = rnw ? 1'b0 : 1'b1;
  RD         = rnw ? 1'b1 : 1'b0;
  Ticks      = ticks;
  Update     = ~Update;
  @UpdateResponse;
  rdata      = DataIn;
endtask

 // Instantiation of the common PcieVHost 
 PcieVhost #(LinkWidth, NodeNum, EndPoint) pcievhost_inst
 (
   .Clk      (Clk),
   .notReset (notReset),

   .LinkIn   (LinkIn),     .LinkOut(LinkOut),

   .WE (WE),               .RD (RD),               .Update (Update),       .Addr (Addr),
   .DataOut(DataOut),      .Ticks(Ticks),          .DataIn(DataIn),        .UpdateResponse(UpdateResponse)
  );


endmodule

//-------------------------------------------------------------
// PcieVhost
//-------------------------------------------------------------

module PcieVhost (input             Clk,       notReset,

                  PcieLinkLanes16   LinkIn,    LinkOut,

                  input             WE,        RD,        Update,
                  input      [31:0] Addr,      DataOut,
                  input integer     Ticks,
                  output reg [31:0] DataIn,
                  output reg        UpdateResponse
  );

parameter LinkWidth = 16;
parameter NodeNum   = 8;
parameter EndPoint  = 0;

// -----------------------------------------------
// Imported DPI tasks
// -----------------------------------------------

import "DPI-C" context task PcieInit(input int node);

reg    [9:0] Out [0:15];
reg   [15:0] ElecIdleOut;
reg          notResetLast;
reg          InvertIn;
reg          InvertOut;
reg          ReverseIn;
reg          ReverseOut;
integer      ClkCount;
integer      i;

wire  [31:0] Node      = NodeNum;
wire  [31:0] Lanes     = LinkWidth;
wire  [31:0] EP        = EndPoint;
wire   [9:0] In [0:15];

// -----------------------------------------------
// Signal assignments
// -----------------------------------------------
// Generate flags for electrical idle stat on input links
wire [15:0] ElecIdleIn = {LinkIn.Lane15 === 10'bzzzzzzzzzz, LinkIn.Lane14 === 10'bzzzzzzzzzz, LinkIn.Lane13 === 10'bzzzzzzzzzz, LinkIn.Lane12 === 10'bzzzzzzzzzz,
                          LinkIn.Lane11 === 10'bzzzzzzzzzz, LinkIn.Lane10 === 10'bzzzzzzzzzz, LinkIn.Lane9  === 10'bzzzzzzzzzz, LinkIn.Lane8  === 10'bzzzzzzzzzz,
                          LinkIn.Lane7  === 10'bzzzzzzzzzz, LinkIn.Lane6  === 10'bzzzzzzzzzz, LinkIn.Lane5  === 10'bzzzzzzzzzz, LinkIn.Lane4  === 10'bzzzzzzzzzz,
                          LinkIn.Lane3  === 10'bzzzzzzzzzz, LinkIn.Lane2  === 10'bzzzzzzzzzz, LinkIn.Lane1  === 10'bzzzzzzzzzz, LinkIn.Lane0  === 10'bzzzzzzzzzz};

// Generate flags for not valid data on links
wire [15:0] RxDetect   = {^LinkOut.Lane15 === 1'bx, ^LinkOut.Lane14 === 1'bx, ^LinkOut.Lane13 === 1'bx, ^LinkOut.Lane12 === 1'bx,
                          ^LinkOut.Lane11 === 1'bx, ^LinkOut.Lane10 === 1'bx, ^LinkOut.Lane9  === 1'bx, ^LinkOut.Lane8  === 1'bx,
                          ^LinkOut.Lane7  === 1'bx, ^LinkOut.Lane6  === 1'bx, ^LinkOut.Lane5  === 1'bx, ^LinkOut.Lane4  === 1'bx,
                          ^LinkOut.Lane3  === 1'bx, ^LinkOut.Lane2  === 1'bx, ^LinkOut.Lane1  === 1'bx, ^LinkOut.Lane0  === 1'bx};

// -----------------------------------------------
// Initial process
// -----------------------------------------------
initial
begin

    for (i = 0; i < 16; i = i + 1)
    begin
        Out[i] = 10'h283;
    end

    InvertIn       = 1'b0;
    InvertOut      = 1'b0;
    ReverseIn      = 1'b0;
    ReverseOut     = 1'b0;
    ElecIdleOut    = 16'h0000;
    notResetLast   = 1'b0;
    UpdateResponse = 1'b1;
    ClkCount       = 0;

    // Call the user C code fo this node
    PcieInit(NodeNum);
end

// -----------------------------------------------
// Synchronous signal delay process
// -----------------------------------------------

always @(posedge Clk)
begin
    notResetLast <= #`RegDel notReset;
    ClkCount     <= #`RegDel ClkCount + 1;
end

// -----------------------------------------------
// Main functional process
// -----------------------------------------------
always @(Update)
begin
    wait (notResetLast == 1'b1);

    //$display("Seen update event (node %d)", NodeNum);

    DataIn = 32'h00000000;
    if (WE === 1'b1 || RD === 1'b1)
    begin
        case(Addr)
        `NODENUMADDR: DataIn = Node;
        `LANESADDR:   DataIn = Lanes;
        `EP_ADDR:     DataIn = EP;
        `CLK_COUNT:   DataIn = ClkCount;
        `LINKADDR0,  `LINKADDR1,  `LINKADDR2,  `LINKADDR3,
        `LINKADDR4,  `LINKADDR5,  `LINKADDR6,  `LINKADDR7,
        `LINKADDR8,  `LINKADDR9,  `LINKADDR10, `LINKADDR11,
        `LINKADDR12, `LINKADDR13, `LINKADDR14, `LINKADDR15:
        begin
            if (WE === 1'b1)
                Out[Addr%16] = DataOut[9:0];
            DataIn = {22'h000000, In[Addr%16]};
        end

        `RESET_STATE:
        begin
            DataIn = {15'h0000, ~notReset};
        end

        `LINK_STATE:
        begin
            if (WE === 1'b1)
                ElecIdleOut = DataOut[15:0] ;
            DataIn = {RxDetect, ElecIdleIn};
        end

        `PVH_INVERT:
        begin
            if (WE === 1'b1)
                {ReverseOut, ReverseIn, InvertOut, InvertIn} = DataOut[3:0] ;
            DataIn = {28'h0000000, ReverseOut, ReverseIn, InvertOut, InvertIn};
        end

        `PVH_STOP:    if (WE === 1'b1) $stop;
        `PVH_FINISH:  if (WE === 1'b1) $finish;
        `PVH_FATAL:   if (WE === 1'b1) `fatal
        default:
        begin
            $display("%m: ***Error. PcieVhost---access to invalid address (%h) from VProc", Addr);
            `fatal
        end
        endcase
    end

    // If ticks specified, wait for the number of clocks
    if (Ticks >= 1)
    begin
      for (int i = 0; i < Ticks; i++)
      begin
        @(posedge Clk);
      end
    end

    // Finished processing, so flag to DPI task
    UpdateResponse <= ~UpdateResponse;
end

// -----------------------------------------------
// Update inputs and outputs
// -----------------------------------------------

// Map the ports to internal variables for VProc access
assign #`PcieVHostSampleDel In[0]  = (ReverseIn  ? LinkIn.Lane15 : LinkIn.Lane0)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[1]  = (ReverseIn  ? LinkIn.Lane14 : LinkIn.Lane1)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[2]  = (ReverseIn  ? LinkIn.Lane13 : LinkIn.Lane2)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[3]  = (ReverseIn  ? LinkIn.Lane12 : LinkIn.Lane3)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[4]  = (ReverseIn  ? LinkIn.Lane11 : LinkIn.Lane4)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[5]  = (ReverseIn  ? LinkIn.Lane10 : LinkIn.Lane5)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[6]  = (ReverseIn  ? LinkIn.Lane9  : LinkIn.Lane6)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[7]  = (ReverseIn  ? LinkIn.Lane8  : LinkIn.Lane7)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[8]  = (ReverseIn  ? LinkIn.Lane7  : LinkIn.Lane8)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[9]  = (ReverseIn  ? LinkIn.Lane6  : LinkIn.Lane9)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[10] = (ReverseIn  ? LinkIn.Lane5  : LinkIn.Lane10) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[11] = (ReverseIn  ? LinkIn.Lane4  : LinkIn.Lane11) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[12] = (ReverseIn  ? LinkIn.Lane3  : LinkIn.Lane12) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[13] = (ReverseIn  ? LinkIn.Lane2  : LinkIn.Lane13) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[14] = (ReverseIn  ? LinkIn.Lane1  : LinkIn.Lane14) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[15] = (ReverseIn  ? LinkIn.Lane0  : LinkIn.Lane15) ^ {10{InvertIn}};

assign LinkOut.Lane0   = ElecIdleOut[0]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[15]  : Out[0])   ^ {10{InvertOut}};
assign LinkOut.Lane1   = ElecIdleOut[1]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[14]  : Out[1])   ^ {10{InvertOut}};
assign LinkOut.Lane2   = ElecIdleOut[2]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[13]  : Out[2])   ^ {10{InvertOut}};
assign LinkOut.Lane3   = ElecIdleOut[3]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[12]  : Out[3])   ^ {10{InvertOut}};
assign LinkOut.Lane4   = ElecIdleOut[4]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[11]  : Out[4])   ^ {10{InvertOut}};
assign LinkOut.Lane5   = ElecIdleOut[5]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[10]  : Out[5])   ^ {10{InvertOut}};
assign LinkOut.Lane6   = ElecIdleOut[6]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[9]   : Out[6])   ^ {10{InvertOut}};
assign LinkOut.Lane7   = ElecIdleOut[7]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[8]   : Out[7])   ^ {10{InvertOut}};
assign LinkOut.Lane8   = ElecIdleOut[8]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[7]   : Out[8])   ^ {10{InvertOut}};
assign LinkOut.Lane9   = ElecIdleOut[9]  ? 10'bzzzzzzzzzz : (ReverseOut ? Out[6]   : Out[9])   ^ {10{InvertOut}};
assign LinkOut.Lane10  = ElecIdleOut[10] ? 10'bzzzzzzzzzz : (ReverseOut ? Out[5]   : Out[10])  ^ {10{InvertOut}};
assign LinkOut.Lane11  = ElecIdleOut[11] ? 10'bzzzzzzzzzz : (ReverseOut ? Out[4]   : Out[11])  ^ {10{InvertOut}};
assign LinkOut.Lane12  = ElecIdleOut[12] ? 10'bzzzzzzzzzz : (ReverseOut ? Out[3]   : Out[12])  ^ {10{InvertOut}};
assign LinkOut.Lane13  = ElecIdleOut[13] ? 10'bzzzzzzzzzz : (ReverseOut ? Out[2]   : Out[13])  ^ {10{InvertOut}};
assign LinkOut.Lane14  = ElecIdleOut[14] ? 10'bzzzzzzzzzz : (ReverseOut ? Out[1]   : Out[14])  ^ {10{InvertOut}};
assign LinkOut.Lane15  = ElecIdleOut[15] ? 10'bzzzzzzzzzz : (ReverseOut ? Out[0]   : Out[15])  ^ {10{InvertOut}};

endmodule

// -----------------------------------------------
// PcieVhostSerial Root Complex
// -----------------------------------------------

module PcieVhostRcSerial (input         Clk, SerClk, notReset,
                          input  [15:0] SerLinkIn,
                          output [15:0] SerLinkOut);

parameter LinkWidth = 16;
parameter NodeNum   = 8;
parameter EndPoint  = 0;

PcieLinkLanes16 LinkIn();
PcieLinkLanes16 LinkOut();

 PcieVhostRc #(LinkWidth, NodeNum, EndPoint) pvh
                   (.Clk      (Clk),
                    .notReset (notReset),
                    .LinkIn   (LinkIn),
                    .LinkOut  (LinkOut)
                    );


 Serialiser serdes (.SerClk     (SerClk),
                    .BitReverse (1'b0),

                    .ParInVec   ({LinkOut.Lane15, LinkOut.Lane14, LinkOut.Lane13, LinkOut.Lane12,
                                  LinkOut.Lane11, LinkOut.Lane10, LinkOut.Lane9,  LinkOut.Lane8,
                                  LinkOut.Lane7,  LinkOut.Lane6,  LinkOut.Lane5,  LinkOut.Lane4,
                                  LinkOut.Lane3,  LinkOut.Lane2,  LinkOut.Lane1,  LinkOut.Lane0}),
                    .SerOut     (SerLinkOut),
                    
                    .SerIn      (SerLinkIn),
                    .ParOut     ({LinkOut.Lane15,  LinkOut.Lane14,  LinkOut.Lane13,  LinkOut.Lane12,
                                  LinkOut.Lane11,  LinkOut.Lane10,  LinkOut.Lane9,   LinkOut.Lane8,
                                  LinkOut.Lane7,   LinkOut.Lane6,   LinkOut.Lane5,   LinkOut.Lane4,
                                  LinkOut.Lane3,   LinkOut.Lane2,   LinkOut.Lane1,  LinkOut.Lane0})
                    );

endmodule

// -----------------------------------------------
// PcieVhostSerial Endpoint
// -----------------------------------------------
module PcieVhostEpSerial (input         Clk, SerClk, notReset,
                          input  [15:0] SerLinkIn,
                          output [15:0] SerLinkOut);

parameter LinkWidth = 16;
parameter NodeNum   = 8;
parameter EndPoint  = 0;

PcieLinkLanes16 LinkIn();
PcieLinkLanes16 LinkOut();

 PcieVhostEp #(LinkWidth, NodeNum, EndPoint) pvh
                   (.Clk      (Clk),
                    .notReset (notReset),
                    .LinkIn   (LinkIn),
                    .LinkOut  (LinkOut)
                    );


 Serialiser serdes (.SerClk     (SerClk),
                    .BitReverse (1'b0),

                    .ParInVec   ({LinkOut.Lane15, LinkOut.Lane14, LinkOut.Lane13, LinkOut.Lane12,
                                  LinkOut.Lane11, LinkOut.Lane10, LinkOut.Lane9,  LinkOut.Lane8,
                                  LinkOut.Lane7,  LinkOut.Lane6,  LinkOut.Lane5,  LinkOut.Lane4,
                                  LinkOut.Lane3,  LinkOut.Lane2,  LinkOut.Lane1,  LinkOut.Lane0}),
                    .SerOut     (SerLinkOut),
                    
                    .SerIn      (SerLinkIn),
                    .ParOut     ({LinkOut.Lane15, LinkOut.Lane14, LinkOut.Lane13, LinkOut.Lane12,
                                  LinkOut.Lane11, LinkOut.Lane10, LinkOut.Lane9,  LinkOut.Lane8,
                                  LinkOut.Lane7,  LinkOut.Lane6,  LinkOut.Lane5,  LinkOut.Lane4,
                                  LinkOut.Lane3,  LinkOut.Lane2,  LinkOut.Lane1,  LinkOut.Lane0})
                    );
endmodule

