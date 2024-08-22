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
// $Id: ContDisps.v,v 1.2 2016/10/05 08:39:33 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/test/ContDisps.v,v $
//
//=============================================================

`ifdef VIVADO
`include "allheaders.v"
`endif

`WsTimeScale

module ContDisps (Clk, DispValOut);

input                        Clk;
output [`DispBits]           DispValOut;


reg [(`NoDispContBits-1):0]  ControlDisps[0:`DispEntries*2]; 
reg [(`NoDispContBits-1):0]  DispVal;
reg [(`NoDispContBits-1):0]  NextDispVal;
reg [(`NoDispContBits-1):0]  MaskDispVal;

reg [(`NoDecTimeDigs*4)-1:0] StartDispTime;

reg [31:0]                   ControlDispsIndex;
integer                      CycleNo;

assign DispValOut = DispVal[`DispBits];

//-------------------------------------------------------------
//

function [(`NoDecTimeDigs*4)-1:0] MakeDecimal;

input [(`NoDecTimeDigs*4)-1:0] InVal;
reg   [(`NoDecTimeDigs*4)-1:0] i, MulBy, DecVal;
reg   [(`NoDecTimeDigs*4)-1:0] HexVal;

begin
    HexVal = InVal;
    DecVal = 0;
    MulBy  = 1;

    for (i = 0; i < `NoDecTimeDigs; i = i + 1)
    begin
        DecVal = ((HexVal & 64'hf) * MulBy) + DecVal;
        HexVal = HexVal >> 4;
        MulBy = MulBy * 10;
    end

    MakeDecimal = DecVal;
end
endfunction

//-------------------------------------------------------------
//

task ReReadContDisps;
begin
    $readmemh({"hex/ContDisps.hex"}, ControlDisps);

    if (ControlDisps[ControlDispsIndex] !== {`DispEntries*2{1'bx}})
    begin
        if (StartDispTime != MakeDecimal(ControlDisps[ControlDispsIndex+1]))
        begin
            StartDispTime = MakeDecimal(ControlDisps[ControlDispsIndex+1]);
            $display("Read in new ContDisps.hex.");
            $display("Will now start to print state on cycle %0d for %h.", StartDispTime, NextDispVal);
        end

        NextDispVal = ControlDisps[ControlDispsIndex];

        if (CycleNo >= StartDispTime)
        begin
            $display("Will start to print state now for %h.", NextDispVal);
        end
    end
end
endtask

//-------------------------------------------------------------
//

initial
begin
    $readmemh({"hex/ContDisps.hex"}, ControlDisps);

    @(posedge Clk);
    
    CycleNo           = 0;
    ControlDispsIndex = 0;
    DispVal           = 0;
    StartDispTime     = MakeDecimal(ControlDisps[ControlDispsIndex+1]);
    NextDispVal       = ControlDisps[ControlDispsIndex];

    $display("Will start to print state on cycle %0d for %h.", StartDispTime, NextDispVal);

    forever
    begin
        if (ControlDisps[ControlDispsIndex] !== {`DispEntries*2{1'bx}})
        begin
            if (CycleNo >= StartDispTime)
            begin
                ControlDispsIndex = ControlDispsIndex + 2;
                DispVal           = NextDispVal;
                StartDispTime     = MakeDecimal(ControlDisps[ControlDispsIndex+1]);
                NextDispVal       = ControlDisps[ControlDispsIndex];

                $display("Will start to print state on cycle %0d for %h.", StartDispTime, NextDispVal);
            end
        end 

        if ((CycleNo % 4000) == 0)
        begin
            ReReadContDisps;
        end

        @(posedge Clk);
        CycleNo = CycleNo + 1;
   end 
end

endmodule
