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
// $Id: ScrambleCodec.v,v 1.2 2016/10/10 11:51:56 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/lib/ScrambleCodec.v,v $
//
//=============================================================

//=============================================================
// A PCI-Express compliant data byte scrambler/descrambler.
// See Base Specification Revision 1.0a section 4.2.3.
//=============================================================

`ifdef VPROC_SV
`include "allheaders.v"
`endif

`WsTimeScale

module ScrambleCodec(ClkPci, notResetPci, MovePipe, Shift, NextShift, OutShift, NextXorWord, XorWord);

input         ClkPci;
input         notResetPci;
input         MovePipe;
input  [15:0] Shift;
output  [7:0] XorWord;
output  [7:0] NextXorWord;
output [15:0] OutShift;
output [15:0] NextShift;

reg    [15:0] OutShift;
reg     [7:0] XorWord;

// G(X) = X^16 + X^5 + X^4 + X^3 + 1

assign NextShift   = (MovePipe ? {Shift[7:0], Shift[15:8]} ^ {5'h00, Shift[15:8], 3'h0} ^ {4'h0, Shift[15:8], 4'h0} ^ {3'h0, Shift[15:8], 5'h00} : 
                                  Shift) | {16{~notResetPci}};

assign NextXorWord = {NextShift[8] , NextShift[9] , NextShift[10], NextShift[11], NextShift[12], NextShift[13], NextShift[14], NextShift[15]};

always @(posedge ClkPci)
begin
    OutShift <= #`RegDel NextShift;
    XorWord  <= #`RegDel NextXorWord;
end

endmodule

