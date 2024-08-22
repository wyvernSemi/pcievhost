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
// $Id: Crc16Gen.v,v 1.2 2016/10/10 11:51:57 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/lib/Crc16Gen.v,v $
//
//=============================================================

//=============================================================
// 16 bit functional crc generator
//
// The following gives the equations required to perform a full CRC-16
// CRC-16: X^16+X^12+X^3+X+1
//
// CRC-16 then has each byte bit reversed and (normally) complemented before output.
//
// The CRC block may be chained where the ShiftIn input is connected to the 
// ShiftChain output of the previous stage. The first stage has its ShiftIn 
// connected to the Shift (registered) output of the last stage. If only one stage
// connect the ShiftIn directly to the Shift output.
//
//=============================================================

`ifdef VIVADO
`include "allheaders.v"
`endif 

`WsTimeScale

module Crc16Gen (Data, Complement, ShiftIn, ShiftChain, Crc, CombCrc);
input Complement;
input [15:0] Data, ShiftIn;             // 16 bit  wide input data.
output [15:0] Crc, CombCrc, ShiftChain;         // 16 bit wide Crc value.

// Registered CRC
assign Crc             = {16{Complement}} ^ {ShiftIn[8],  ShiftIn[9],  ShiftIn[10], ShiftIn[11], 
                                             ShiftIn[12], ShiftIn[13], ShiftIn[14], ShiftIn[15],
                                             ShiftIn[0],  ShiftIn[1],  ShiftIn[2],  ShiftIn[3],  
                                             ShiftIn[4],  ShiftIn[5],  ShiftIn[6],  ShiftIn[7]};

// Combinatorial CRC 
assign CombCrc         = {16{Complement}} ^ {ShiftChain[8],  ShiftChain[9],  ShiftChain[10], ShiftChain[11], 
                                             ShiftChain[12], ShiftChain[13], ShiftChain[14], ShiftChain[15],
                                             ShiftChain[0],  ShiftChain[1],  ShiftChain[2],  ShiftChain[3],  
                                             ShiftChain[4],  ShiftChain[5],  ShiftChain[6],  ShiftChain[7]};

// LSB first
wire [15:0] DtXorShift = { Data[8],  Data[9],  Data[10], Data[11], Data[12], Data[13], Data[14], Data[15],
                           Data[0],  Data[1],  Data[2],  Data[3],  Data[4],  Data[5],  Data[6],  Data[7]
                          } ^ ShiftIn;

assign ShiftChain[00]  = ^(DtXorShift & 16'hb111);
assign ShiftChain[01]  = ^(DtXorShift & 16'hd333);
assign ShiftChain[02]  = ^(DtXorShift & 16'ha666);
assign ShiftChain[03]  = ^(DtXorShift & 16'hfddd);
assign ShiftChain[04]  = ^(DtXorShift & 16'hfbba);
assign ShiftChain[05]  = ^(DtXorShift & 16'hf774);
assign ShiftChain[06]  = ^(DtXorShift & 16'heee8);
assign ShiftChain[07]  = ^(DtXorShift & 16'hddd0);
assign ShiftChain[08]  = ^(DtXorShift & 16'hbba0);
assign ShiftChain[09]  = ^(DtXorShift & 16'h7740);
assign ShiftChain[10]  = ^(DtXorShift & 16'hee80);
assign ShiftChain[11]  = ^(DtXorShift & 16'hdd00);
assign ShiftChain[12]  = ^(DtXorShift & 16'h0b11);
assign ShiftChain[13]  = ^(DtXorShift & 16'h1622);
assign ShiftChain[14]  = ^(DtXorShift & 16'h2c44);
assign ShiftChain[15]  = ^(DtXorShift & 16'h5888);

endmodule

// 16 bit CRC for 32 bit wide input data
module Crc16x2Gen (Data, Complement, Crc);
input         Complement;
input  [31:0] Data;
output [15:0] Crc;

wire [15:0] ShiftChain;

    // Stage used combinatorially only
    Crc16Gen crc0 (.Data       (Data[31:16]), 
                   .Complement (1'b0),       
                   .ShiftIn    (16'hffff), 
                   .ShiftChain (ShiftChain),
                   .Crc        (),
                   .CombCrc    ()
                   );

    Crc16Gen crc1 (.Data       (Data[15:0]),
                   .Complement (Complement),
                   .ShiftIn    (ShiftChain),
                   .ShiftChain (),
                   .Crc        (),
                   .CombCrc    (Crc)
                   );

//endtask


endmodule

