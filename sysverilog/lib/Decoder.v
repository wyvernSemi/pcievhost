//=============================================================
// 
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Mar 2023
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
// An 8b/10b Decoder. See ANSI X3.230-1994.
//=============================================================

`WsTimeScale

module Decoder (Clk, Input, BitRev, InvertDataIn, OutputRaw, ControlRaw, Output, Control);

input        Clk;
input        BitRev;
input        InvertDataIn;
input  [9:0] Input;
output [7:0] Output;
output [7:0] OutputRaw;
output       Control;
output       ControlRaw;

reg [7:0] Output;
reg Control; 

wire [9:0] InRev       = {Input[0], Input[1], Input[2], Input[3], Input[4], 
                          Input[5], Input[6], Input[7], Input[8], Input[9]};

wire [9:0] InVal       = (BitRev ? InRev : Input) ^ {10{InvertDataIn}};

// Run length flags of InVal first 4 bits
wire TwoOnes           = ((^InVal[3:2]) &  (^InVal[1:0])) | (^(InVal[2:1]) & (InVal[3] ^ InVal[0]));
wire ThreeZeros        = ((^InVal[1:0]) & ~(|InVal[3:2])) | (^(InVal[3:2]) & ~(|InVal[1:0]));
wire ThreeOnes         = ((^InVal[1:0]) &  (&InVal[3:2])) | (^(InVal[3:2]) &  (&InVal[1:0]));

// Calculate bit inversion requirements for 6b/5b code
wire Invert21          = TwoOnes        & (&InVal[2:1])  & ~(^InVal[5:4]);
wire Invert430         = TwoOnes        & ~(|InVal[2:1]) & ~(^InVal[5:4]);
wire Invert4           = ThreeZeros     & ~(InVal[5]);
wire Invert3210        = ThreeOnes      &  InVal[5];
wire Invert31          = TwoOnes        &  InVal[0]      & InVal[2]  & ~(^InVal[5:4]);
wire Invert420         = TwoOnes        & ~(InVal[0])    & ~InVal[2] & ~(^InVal[5:4]);
wire Invert42          = ~(|InVal[1:0]) & ~(|InVal[5:4]);
wire Invert310         =  (&InVal[1:0]) & (&InVal[5:4]);
wire InvertAll5        = (ThreeZeros    & ((&InVal[5:3]) | ~InVal[4])) | ~(|InVal[5:2]);

// Create an inversion mask for 6b/5b code
wire [4:0] InvertMask5 = {Invert430 | Invert4    | Invert420 | Invert42  | InvertAll5,
                          Invert430 | Invert3210 | Invert31  | Invert310 | InvertAll5,
                          Invert21  | Invert3210 | Invert420 | Invert42  | InvertAll5,
                          Invert21  | Invert3210 | Invert31  | Invert310 | InvertAll5,
                          Invert430 | Invert3210 | Invert420 | Invert310 | InvertAll5}; 

// Low order 5 bits
wire [4:0] LowBits     =  InVal[4:0] ^ InvertMask5;

// Flag condition where all 3 bits need inverting
wire InvertAll3        = (~(|InVal[5:2]) & (^InVal[9:8])) | (~(|InVal[7:6]) & (&InVal[9:8])) | 
                         ((&InVal[7:6]) & InVal[9]) | (~(|InVal[8:6]));

// Create an inversion mask for 4b/3b code
wire [2:0] InvertMask3 = { ((&InVal[9:8]) & InVal[6]) | ~(|InVal[9:7]),
                           (~(|InVal[9:8]) & ~InVal[6]) | ~(|InVal[9:7]),
                           ((&InVal[9:8]) & InVal[6]) | (&InVal[9:7])} | {3{InvertAll3}};

// High order 3 bits
wire [2:0] Hibits      = InVal[8:6] ^ InvertMask3;

assign OutputRaw       = {Hibits, LowBits};

// Output is a control code, not a data byte
assign ControlRaw      = (&InVal[5:2]) | ~(|InVal[5:2]) | (ThreeZeros & ~InVal[4] & InVal[5] & (&InVal[9:7])) |
                         (ThreeOnes & InVal[4] & ~InVal[5] & ~(|InVal[9:7]));

always @(posedge Clk)
begin
    Output  <= #`RegDel OutputRaw;
    Control <= #`RegDel ControlRaw;
end

endmodule
