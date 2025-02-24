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
// PcieVhost
//-------------------------------------------------------------

module PcieVhost (Clk, notReset,

`ifdef VERILATOR
                  ElecIdleOut,
                  ElecIdleIn,
`endif

                  LinkIn0,   LinkIn1,   LinkIn2,   LinkIn3,
                  LinkIn4,   LinkIn5,   LinkIn6,   LinkIn7,
                  LinkIn8,   LinkIn9,   LinkIn10,  LinkIn11,
                  LinkIn12,  LinkIn13,  LinkIn14,  LinkIn15,
                  LinkOut0,  LinkOut1,  LinkOut2,  LinkOut3,
                  LinkOut4,  LinkOut5,  LinkOut6,  LinkOut7,
                  LinkOut8,  LinkOut9,  LinkOut10, LinkOut11,
                  LinkOut12, LinkOut13, LinkOut14, LinkOut15);

parameter LinkWidth = 16;
parameter NodeNum   = 8;
parameter EndPoint  = 0;

input        Clk;
input        notReset;

input  [9:0] LinkIn0,   LinkIn1,   LinkIn2,   LinkIn3;
input  [9:0] LinkIn4,   LinkIn5,   LinkIn6,   LinkIn7;
input  [9:0] LinkIn8,   LinkIn9,   LinkIn10,  LinkIn11;
input  [9:0] LinkIn12,  LinkIn13,  LinkIn14,  LinkIn15;

output [9:0] LinkOut0,  LinkOut1,  LinkOut2,  LinkOut3;
output [9:0] LinkOut4,  LinkOut5,  LinkOut6,  LinkOut7;
output [9:0] LinkOut8,  LinkOut9,  LinkOut10, LinkOut11;
output [9:0] LinkOut12, LinkOut13, LinkOut14, LinkOut15;

`ifdef VERILATOR
input [15:0] ElecIdleIn;
output[15:0] ElecIdleOut;
reg   [15:0] ElecIdleOut;
`define ELECIDLE 10'b0000000000
`else
reg   [15:0] ElecIdleOut;
`define ELECIDLE 10'bzzzzzzzzzz
`endif


reg    [9:0] Out [0:15];
reg   [31:0] DataIn;
reg          notResetLast;
reg          InvertIn;
reg          InvertOut;
reg          ReverseIn;
reg          ReverseOut;
reg          UpdateResponse;
integer      ClkCount;
integer      i;

// VP Interface wires
wire  [31:0] Addr;
wire  [31:0] DataOut;
wire         WE, RD;

wire         WRAck     = 1'b1;
wire         RDAck     = 1'b1;
wire   [2:0] Interrupt = {notReset & ~notResetLast, 2'b00};

wire  [31:0] Node      = NodeNum;
wire  [31:0] Lanes     = LinkWidth;
wire  [31:0] EP        = EndPoint;
wire         Update;
wire   [9:0] In [0:15];

 // --------------------------------
 // Virtual Processor
 // --------------------------------
 VProc vp (.Clk            (Clk),
           .Addr           (Addr),
           .WE             (WE),
           .RD             (RD),
           .DataOut        (DataOut),
           .DataIn         (DataIn),
           .WRAck          (WRAck),
           .RDAck          (RDAck),
           .Interrupt      (Interrupt),
           .Update         (Update),
           .UpdateResponse (UpdateResponse),
           .Node           (Node[3:0])
           );

// Generate flags for electrical idle stat on input links
`ifndef VERILATOR
wire [15:0] ElecIdleIn = {LinkIn15 === 10'bzzzzzzzzzz, LinkIn14 === 10'bzzzzzzzzzz, LinkIn13 === 10'bzzzzzzzzzz, LinkIn12 === 10'bzzzzzzzzzz,
                          LinkIn11 === 10'bzzzzzzzzzz, LinkIn10 === 10'bzzzzzzzzzz, LinkIn9  === 10'bzzzzzzzzzz, LinkIn8  === 10'bzzzzzzzzzz,
                          LinkIn7  === 10'bzzzzzzzzzz, LinkIn6  === 10'bzzzzzzzzzz, LinkIn5  === 10'bzzzzzzzzzz, LinkIn4  === 10'bzzzzzzzzzz,
                          LinkIn3  === 10'bzzzzzzzzzz, LinkIn2  === 10'bzzzzzzzzzz, LinkIn1  === 10'bzzzzzzzzzz, LinkIn0  === 10'bzzzzzzzzzz};
`endif
// Generate flags for not valid data on links
wire [15:0] RxDetect   = {^LinkOut15 === 1'bx, ^LinkOut14 === 1'bx, ^LinkOut13 === 1'bx, ^LinkOut12 === 1'bx,
                          ^LinkOut11 === 1'bx, ^LinkOut10 === 1'bx, ^LinkOut9  === 1'bx, ^LinkOut8  === 1'bx,
                          ^LinkOut7  === 1'bx, ^LinkOut6  === 1'bx, ^LinkOut5  === 1'bx, ^LinkOut4  === 1'bx,
                          ^LinkOut3  === 1'bx, ^LinkOut2  === 1'bx, ^LinkOut1  === 1'bx, ^LinkOut0  === 1'bx};


initial
begin
    for (i = 0; i < 16; i = i + 1)
    begin
        Out[i] = 10'h000;
    end

    InvertIn       = 1'b0;
    InvertOut      = 1'b0;
    ReverseIn      = 1'b0;
    ReverseOut     = 1'b0;
    ElecIdleOut    = 16'h0000;
    notResetLast   = 1'b0;
    UpdateResponse = 1'b1;
    ClkCount       = 0;
end

always @(posedge Clk)
begin
    notResetLast <= #`RegDel notReset;
    ClkCount     <= #`RegDel ClkCount + 1;
end

always @(Update)
begin
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

    // Finished processing, so flag to VProc
    UpdateResponse <= ~UpdateResponse;
end

// Map the ports to internal variables for VProc access
assign #`PcieVHostSampleDel In[0]  = (ReverseIn  ? LinkIn15 : LinkIn0)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[1]  = (ReverseIn  ? LinkIn14 : LinkIn1)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[2]  = (ReverseIn  ? LinkIn13 : LinkIn2)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[3]  = (ReverseIn  ? LinkIn12 : LinkIn3)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[4]  = (ReverseIn  ? LinkIn11 : LinkIn4)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[5]  = (ReverseIn  ? LinkIn10 : LinkIn5)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[6]  = (ReverseIn  ? LinkIn9  : LinkIn6)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[7]  = (ReverseIn  ? LinkIn8  : LinkIn7)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[8]  = (ReverseIn  ? LinkIn7  : LinkIn8)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[9]  = (ReverseIn  ? LinkIn6  : LinkIn9)  ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[10] = (ReverseIn  ? LinkIn5  : LinkIn10) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[11] = (ReverseIn  ? LinkIn4  : LinkIn11) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[12] = (ReverseIn  ? LinkIn3  : LinkIn12) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[13] = (ReverseIn  ? LinkIn2  : LinkIn13) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[14] = (ReverseIn  ? LinkIn1  : LinkIn14) ^ {10{InvertIn}};
assign #`PcieVHostSampleDel In[15] = (ReverseIn  ? LinkIn0  : LinkIn15) ^ {10{InvertIn}};

assign LinkOut0   = ElecIdleOut[0]  ? `ELECIDLE : (ReverseOut ? Out[15]  : Out[0])   ^ {10{InvertOut}};
assign LinkOut1   = ElecIdleOut[1]  ? `ELECIDLE : (ReverseOut ? Out[14]  : Out[1])   ^ {10{InvertOut}};
assign LinkOut2   = ElecIdleOut[2]  ? `ELECIDLE : (ReverseOut ? Out[13]  : Out[2])   ^ {10{InvertOut}};
assign LinkOut3   = ElecIdleOut[3]  ? `ELECIDLE : (ReverseOut ? Out[12]  : Out[3])   ^ {10{InvertOut}};
assign LinkOut4   = ElecIdleOut[4]  ? `ELECIDLE : (ReverseOut ? Out[11]  : Out[4])   ^ {10{InvertOut}};
assign LinkOut5   = ElecIdleOut[5]  ? `ELECIDLE : (ReverseOut ? Out[10]  : Out[5])   ^ {10{InvertOut}};
assign LinkOut6   = ElecIdleOut[6]  ? `ELECIDLE : (ReverseOut ? Out[9]   : Out[6])   ^ {10{InvertOut}};
assign LinkOut7   = ElecIdleOut[7]  ? `ELECIDLE : (ReverseOut ? Out[8]   : Out[7])   ^ {10{InvertOut}};
assign LinkOut8   = ElecIdleOut[8]  ? `ELECIDLE : (ReverseOut ? Out[7]   : Out[8])   ^ {10{InvertOut}};
assign LinkOut9   = ElecIdleOut[9]  ? `ELECIDLE : (ReverseOut ? Out[6]   : Out[9])   ^ {10{InvertOut}};
assign LinkOut10  = ElecIdleOut[10] ? `ELECIDLE : (ReverseOut ? Out[5]   : Out[10])  ^ {10{InvertOut}};
assign LinkOut11  = ElecIdleOut[11] ? `ELECIDLE : (ReverseOut ? Out[4]   : Out[11])  ^ {10{InvertOut}};
assign LinkOut12  = ElecIdleOut[12] ? `ELECIDLE : (ReverseOut ? Out[3]   : Out[12])  ^ {10{InvertOut}};
assign LinkOut13  = ElecIdleOut[13] ? `ELECIDLE : (ReverseOut ? Out[2]   : Out[13])  ^ {10{InvertOut}};
assign LinkOut14  = ElecIdleOut[14] ? `ELECIDLE : (ReverseOut ? Out[1]   : Out[14])  ^ {10{InvertOut}};
assign LinkOut15  = ElecIdleOut[15] ? `ELECIDLE : (ReverseOut ? Out[0]   : Out[15])  ^ {10{InvertOut}};

endmodule

//-------------------------------------------------------------
// PcieVhostSerial
//-------------------------------------------------------------

module PcieVhostSerial (Clk, SerClk, notReset,
                        LinkIn0,   LinkIn1,   LinkIn2,   LinkIn3,
                        LinkIn4,   LinkIn5,   LinkIn6,   LinkIn7,
                        LinkIn8,   LinkIn9,   LinkIn10,  LinkIn11,
                        LinkIn12,  LinkIn13,  LinkIn14,  LinkIn15,
                        LinkOut0,  LinkOut1,  LinkOut2,  LinkOut3,
                        LinkOut4,  LinkOut5,  LinkOut6,  LinkOut7,
                        LinkOut8,  LinkOut9,  LinkOut10, LinkOut11,
                        LinkOut12, LinkOut13, LinkOut14, LinkOut15);

parameter LinkWidth = 16;
parameter NodeNum   = 8;
parameter EndPoint  = 0;

input      Clk;
input      SerClk;
input      notReset;
input      LinkIn0,    LinkIn1,    LinkIn2,    LinkIn3;
input      LinkIn4,    LinkIn5,    LinkIn6,    LinkIn7;
input      LinkIn8,    LinkIn9,    LinkIn10,   LinkIn11;
input      LinkIn12,   LinkIn13,   LinkIn14,   LinkIn15;

output     LinkOut0,   LinkOut1,   LinkOut2,   LinkOut3;
output     LinkOut4,   LinkOut5,   LinkOut6,   LinkOut7;
output     LinkOut8,   LinkOut9,   LinkOut10,  LinkOut11;
output     LinkOut12,  LinkOut13,  LinkOut14,  LinkOut15;

wire [9:0] PLinkIn0,   PLinkIn1,   PLinkIn2,   PLinkIn3;
wire [9:0] PLinkIn4,   PLinkIn5,   PLinkIn6,   PLinkIn7;
wire [9:0] PLinkIn8,   PLinkIn9,   PLinkIn10,  PLinkIn11;
wire [9:0] PLinkIn12,  PLinkIn13,  PLinkIn14,  PLinkIn15;

wire [9:0] PLinkOut0,  PLinkOut1,  PLinkOut2,  PLinkOut3;
wire [9:0] PLinkOut4,  PLinkOut5,  PLinkOut6,  PLinkOut7;
wire [9:0] PLinkOut8,  PLinkOut9,  PLinkOut10, PLinkOut11;
wire [9:0] PLinkOut12, PLinkOut13, PLinkOut14, PLinkOut15;

 PcieVhost #(LinkWidth, NodeNum, EndPoint) pvh
                   (.Clk      (Clk),
                    .notReset (notReset),
`ifdef VERILATOR
                    .ElecIdleOut (),
                    .ElecIdleIn  ({LinkWidth{1'b0}}),
`endif
                    .LinkIn0   (PLinkIn0),
                    .LinkIn1   (PLinkIn1),
                    .LinkIn2   (PLinkIn2),
                    .LinkIn3   (PLinkIn3),
                    .LinkIn4   (PLinkIn4),
                    .LinkIn5   (PLinkIn5),
                    .LinkIn6   (PLinkIn6),
                    .LinkIn7   (PLinkIn7),
                    .LinkIn8   (PLinkIn8),
                    .LinkIn9   (PLinkIn9),
                    .LinkIn10  (PLinkIn10),
                    .LinkIn11  (PLinkIn11),
                    .LinkIn12  (PLinkIn12),
                    .LinkIn13  (PLinkIn13),
                    .LinkIn14  (PLinkIn14),
                    .LinkIn15  (PLinkIn15),
                    .LinkOut0  (PLinkOut0),
                    .LinkOut1  (PLinkOut1),
                    .LinkOut2  (PLinkOut2),
                    .LinkOut3  (PLinkOut3),
                    .LinkOut4  (PLinkOut4),
                    .LinkOut5  (PLinkOut5),
                    .LinkOut6  (PLinkOut6),
                    .LinkOut7  (PLinkOut7),
                    .LinkOut8  (PLinkOut8),
                    .LinkOut9  (PLinkOut9),
                    .LinkOut10 (PLinkOut10),
                    .LinkOut11 (PLinkOut11),
                    .LinkOut12 (PLinkOut12),
                    .LinkOut13 (PLinkOut13),
                    .LinkOut14 (PLinkOut14),
                    .LinkOut15 (PLinkOut15)
                    );


 Serialiser serdes (.SerClk     (SerClk),
                    .BitReverse (1'b0),

                    .ParInVec   ({PLinkOut15, PLinkOut14, PLinkOut13, PLinkOut12, PLinkOut11, PLinkOut10, PLinkOut9, PLinkOut8,
                                  PLinkOut7,  PLinkOut6,  PLinkOut5,  PLinkOut4,  PLinkOut3,  PLinkOut2,  PLinkOut1, PLinkOut0}),
                    .SerOut     ({LinkOut15,  LinkOut14,  LinkOut13,  LinkOut12,  LinkOut11,  LinkOut10,  LinkOut9,  LinkOut8,
                                  LinkOut7,   LinkOut6,   LinkOut5,   LinkOut4,   LinkOut3,   LinkOut2,   LinkOut1,  LinkOut0}),
                    .SerIn      ({LinkIn15,   LinkIn14,   LinkIn13,   LinkIn12,   LinkIn11,   LinkIn10,   LinkIn9,   LinkIn8,
                                  LinkIn7,    LinkIn6,    LinkIn5,    LinkIn4,    LinkIn3,    LinkIn2,    LinkIn1,   LinkIn0}),
                    .ParOut     ({PLinkIn15,  PLinkIn14,  PLinkIn13,  PLinkIn12,  PLinkIn11,  PLinkIn10,  PLinkIn9,  PLinkIn8,
                                  PLinkIn7,   PLinkIn6,   PLinkIn5,   PLinkIn4,   PLinkIn3,   PLinkIn2,   PLinkIn1,  PLinkIn0})
                    );


endmodule

