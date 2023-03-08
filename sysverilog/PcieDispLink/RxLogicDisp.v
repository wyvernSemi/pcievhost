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
// $Id: RxLogicDisp.v,v 1.2 2016/10/10 11:48:16 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/PcieDispLink/RxLogicDisp.v,v $
//
//=============================================================

//=============================================================
// A PCI-Express Physical (logical) layer Rx RxControl logic
// See Base Specification Revision 1.0a section 4.2
//=============================================================

`WsTimeScale

module RxLogicDisp (Clk, notReset, DecodeCtrl, DecodeByte, OutByteRaw, 
                    LinkIn, Synced,
                    notResetScrambler, MoveScrambler, Scramble, DisableScramble,
                    ElecIdleOrderedSet, FtsOrderedSet, SkpOrderedSet, RxTrainingSeq,
                    RxControl);

input          Clk;
input          notReset;
input          DecodeCtrl;
input          Synced;
input          DisableScramble;
input    [7:0] DecodeByte;
input    [9:0] LinkIn;
input    [7:0] OutByteRaw;

output         notResetScrambler;
output         MoveScrambler;
output         Scramble;
output         RxControl;
output         ElecIdleOrderedSet;
output         FtsOrderedSet;
output         SkpOrderedSet;
output   [1:0] RxTrainingSeq;

reg            RxControl;
reg            RxCommaReg;
reg            DisparityEnabledLast;
reg      [9:0] LastNon0LinkIn;
reg            CommaLast;
reg      [3:0] TSByteCount;
reg            TSRx_H;
reg            RxIdleLast;
reg            RxFtsLast;
reg            RxSkpLast;
reg      [1:0] IdleSetCount;
reg      [1:0] FtsSetCount;
reg      [1:0] SkpSetCount;
reg    [127:0] OSBuffer;

wire RxIdle                   = DecodeCtrl & (DecodeByte == `IDL);
wire RxComma                  = DecodeCtrl & (DecodeByte == `COM);
wire RxPad                    = DecodeCtrl & (DecodeByte == `PAD);
wire RxFts                    = DecodeCtrl & (DecodeByte == `FTS);
wire RxSkp                    = DecodeCtrl & (DecodeByte == `SKP);

wire TSRx                     = ((TSRx_H & |TSByteCount) | (CommaLast & (RxPad | ~DecodeCtrl))) & & Synced;

assign MoveScrambler          = ~(RxControl & OutByteRaw == `SKP) & Synced;
assign notResetScrambler      = Synced & ~RxCommaReg;
assign Scramble               = ~(RxControl | ~Synced | TSRx | DisableScramble/* | CompliancePattern */);

// Enable disparity checks only if synced and received a COMMA
wire DisparityEnabled         = (DisparityEnabledLast | (LinkIn == `PCOMMA || LinkIn == `NCOMMA)) & Synced;

// Flag incoming word has a non-zero disparity (even parity)
wire Disp                     = DisparityEnabled & ~(^LinkIn);

// Remember the last nonzero disparity word
wire [9:0] NextLastNon0LinkIn = (Disp ? LinkIn : LastNon0LinkIn);

// Bad disparity if non-zero input disparity combined with last non-zero disparity 
// is itself non-zero (odd parity)
wire BadDisparity             = DisparityEnabledLast & Disp & ^{LinkIn, LastNon0LinkIn};

// Flag run lengths of 6 or more
wire BadRunLength             = DisparityEnabled & 
                                ((&LinkIn[5:0])  |  (&LinkIn[6:1]) |  (&LinkIn[7:2]) |  &(LinkIn[8:3]) |  (&LinkIn[9:4]) |
                                ~(|LinkIn[5:0])  | ~(|LinkIn[6:1]) | ~(|LinkIn[7:2]) | ~(|LinkIn[8:3]) | ~(|LinkIn[9:4]));

wire [3:0] NextTSByteCount    = (RxComma ? 4'h0 : TSByteCount + 4'h1) & {4{notReset}};

wire CommaOrIdleLast          = CommaLast | (RxIdleLast & ~(&IdleSetCount));
wire [1:0] NextIdleSetCount   = (IdleSetCount - {1'b0, CommaOrIdleLast}) | {2{~notReset | RxComma | ~CommaOrIdleLast}};

// Idle ordered set detection
assign ElecIdleOrderedSet     = ~(|IdleSetCount);

// FTS ordered set detection
wire FtsOrCommaLast           =  CommaLast | (RxFtsLast & ~(&FtsSetCount));
wire [1:0] NextFtsSetCount    = (FtsSetCount - {1'b0, FtsOrCommaLast}) | {2{RxComma | ~FtsOrCommaLast}}; 
assign FtsOrderedSet          = ~(|FtsSetCount);

// SKP ordered set detection
wire SkpOrCommaLast           = CommaLast | (RxSkpLast & ~(&SkpSetCount));
wire [1:0] NextSkpSetCount    = (SkpSetCount - {1'b0, SkpOrCommaLast}) | {2{RxComma | ~SkpOrCommaLast}}; 
assign SkpOrderedSet          = ~(|SkpSetCount);

wire [127:0] NextOSBuffer     = (OSBuffer & ~(128'h00000000000000ff << {TSByteCount, 3'b000})) |  
                                ({120'h0, OutByteRaw} << {TSByteCount, 3'b000});

assign RxTrainingSeq          = {2{~|TSByteCount}} & ({OSBuffer[127:120] == `TS2_ID, OSBuffer[127:120] == `TS1_ID}) & 
                                                     ({OSBuffer[119:112] == `TS2_ID, OSBuffer[119:112] == `TS1_ID}) &
                                                     ({OSBuffer[111:104] == `TS2_ID, OSBuffer[111:104] == `TS1_ID}) &
                                                     ({OSBuffer[103:96 ] == `TS2_ID, OSBuffer[103:96 ] == `TS1_ID}) &
                                                     ({OSBuffer[95:88]   == `TS2_ID, OSBuffer[95:88]   == `TS1_ID}) &
                                                     ({OSBuffer[87:80]   == `TS2_ID, OSBuffer[87:80]   == `TS1_ID}) &
                                                     ({OSBuffer[79:72]   == `TS2_ID, OSBuffer[79:72]   == `TS1_ID}) &
                                                     ({OSBuffer[71:64]   == `TS2_ID, OSBuffer[71:64]   == `TS1_ID}) &
                                                     ({OSBuffer[63:56]   == `TS2_ID, OSBuffer[63:56]   == `TS1_ID}) &
                                                     ({OSBuffer[55:48]   == `TS2_ID, OSBuffer[55:48]   == `TS1_ID}) ;


always @(posedge Clk)
begin
    TSByteCount          <= #`RegDel NextTSByteCount;
    RxControl            <= #`RegDel DecodeCtrl & Synced;
    RxCommaReg           <= #`RegDel RxComma;
    DisparityEnabledLast <= #`RegDel DisparityEnabled;
    LastNon0LinkIn       <= #`RegDel NextLastNon0LinkIn;
    CommaLast            <= #`RegDel RxComma & notReset;
    TSRx_H               <= #`RegDel TSRx;

    IdleSetCount         <= #`RegDel NextIdleSetCount;
    FtsSetCount          <= #`RegDel NextFtsSetCount;
    SkpSetCount          <= #`RegDel NextSkpSetCount;
    OSBuffer             <= #`RegDel NextOSBuffer;
    RxIdleLast           <= #`RegDel RxIdle  & notReset;
    RxFtsLast            <= #`RegDel RxFts   & notReset;
    RxSkpLast            <= #`RegDel RxSkp   & notReset;

end

always @(posedge Clk)
begin
    if (Synced && (BadRunLength || BadDisparity))
    begin
        $display ("****RxLogic had bad runlength/disparity error");
        `fatal
    end
end

endmodule
