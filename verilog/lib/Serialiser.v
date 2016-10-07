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
// $Id: Serialiser.v,v 1.1 2016/10/04 15:47:37 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/lib/Serialiser.v,v $
//
//=============================================================

`WsTimeScale

// With BitReverse = 1'b0, serialisation/deserialisation is lsb.

module Serialiser (SerClk, BitReverse, ParInVec, SerOut, SerIn, ParOut);

parameter Width = 16;

input                 SerClk;
input                 BitReverse;
input  [Width*10-1:0] ParInVec;
input  [Width-1:0]    SerIn;

output [Width*10-1:0] ParOut;
output [Width-1:0]    SerOut;

reg    [9:0]          SerialShift   [0:Width-1];
reg    [9:0]          DeserialShift [0:Width-1];
reg    [9:0]          DeserialReg   [0:Width-1];

reg                   Synced;

integer               SerialCount;
integer               DeserialCount;
integer               idx;

wire [9:0] ParIn [0:Width-1];

function [9:0] BitRev10;
input [9:0] InVal;
begin
    BitRev10 = {InVal[0], InVal[1], InVal[2], InVal[3], InVal[4],
                InVal[5], InVal[6], InVal[7], InVal[8], InVal[9]};
end
endfunction

genvar i;
generate
    for (i = 0; i < Width; i = i +1)
    begin : io
        assign ParIn [i]           = BitReverse ? BitRev10(ParInVec[i*10+9:i*10]) : ParInVec[i*10+9:i*10];
        assign ParOut[i*10+9:i*10] = Synced     ? (BitReverse ? BitRev10(DeserialReg[i]) : DeserialReg[i]) : 10'bzzz;
        assign SerOut[i]           = SerialShift[i][0];
    end
endgenerate

initial 
begin
    Synced        = 0;
    SerialCount   = 9;
    DeserialCount = 0;
end

always @(posedge SerClk) 
begin
    if (SerialCount == 0) 
    begin
        SerialCount <=  9;
        for (idx = 0; idx < Width; idx = idx + 1)
            SerialShift[idx] <= ParIn[idx];
    end
    else 
    begin
        SerialCount <=  SerialCount - 1;
        for (idx = 0; idx < Width; idx = idx + 1)
            SerialShift[idx] <= {1'b0, SerialShift[idx][9:1]};
    end

    for (idx = 0; idx < Width; idx = idx + 1)
        DeserialShift[idx] <= {SerIn[idx], DeserialShift[idx][9:1]};

    // Maintain sync
    if (Synced)
    begin
        if (SerIn[0] === 1'bz || SerIn[0] === 1'bx)
            Synced <= 1'b0;
    end
    else
    begin
        if (DeserialShift[0] == `NCOMMA || DeserialShift[0] == `PCOMMA)
            Synced <= 1'b1;
    end

    if (Synced)
    begin
        DeserialCount <=  (DeserialCount + 1) % 10;
    end

    if (DeserialCount == 9 || (!Synced && (DeserialShift[0] == `NCOMMA || DeserialShift[0] == `PCOMMA)))
    begin
        for (idx = 0; idx < Width; idx = idx + 1)
        begin
            DeserialReg[idx] <= DeserialShift[idx];
        end
    end

end

endmodule

