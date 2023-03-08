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
// A PCI-Express Physical (logical) layer Rx Data Path
// See Base Specification Revision 1.0a section 4.2
//=============================================================

`WsTimeScale

module RxDP (ClkPci, NextScramble, DecodeByte, NextScXor, OutByteRaw, OutByteSc);

input        ClkPci;
input        NextScramble;
input  [7:0] DecodeByte;
input  [7:0] NextScXor;

output [7:0] OutByteSc;
output [7:0] OutByteRaw;

reg    [7:0] OutByteRaw;
reg    [7:0] OutByteSc;

always @(posedge ClkPci)
begin
    OutByteRaw  <= #`RegDel DecodeByte;
    OutByteSc   <= #`RegDel DecodeByte ^ (NextScXor & {8{NextScramble}});
end

endmodule
