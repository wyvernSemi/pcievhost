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
// $Id: PcieDispLink.v,v 1.2 2016/10/05 08:40:57 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/verilog/PcieDispLink/PcieDispLink.v,v $
//
//=============================================================

//=============================================================
// A PCI Express link display
//=============================================================

`WsTimeScale

`define SEQ_NUM_LEN           2
`define MIN_HDR_LEN          12
`define LCRC_LEN              4  
`define EXTENDED_TAG          1'b0

//-------------------------------------------------------------
// PcieDispLink
//-------------------------------------------------------------

`ifdef DISP_LINK_WIDE
module PcieDispLink (ExtClk,              Link, notReset, FwdName, BckName, DispDataIn, DispDataOut, DispValIn, LinkWidth, DisableScramble, InvertTxPolarity, NodeNum);
`else
module PcieDispLink (ExtClk, ExtClk10, SerLink, notReset, FwdName, BckName, DispDataIn, DispDataOut, DispValIn, LinkWidth, DisableScramble, InvertTxPolarity, NodeNum);
`endif

parameter NumOfLanes = 1;

`ifdef DISP_LINK_WIDE
input [(NumOfLanes*10)-1:0] Link;
`else
input                       ExtClk10;
input [NumOfLanes-1:0]      SerLink;
`endif

input                       ExtClk;
input                       DisableScramble;
input [15:0]                InvertTxPolarity;
input                       notReset;
input [7:0]                 FwdName, BckName;
input [7:0]                 NodeNum;
input [`DispBits]           DispValIn;

input [`DispDataInBits]     DispDataIn;
input [4:0]                 LinkWidth;

output [`DispDataOutBits]   DispDataOut;

`ifndef DISP_LINK_WIDE
wire [(NumOfLanes*10)-1:0]  Link;

 Serialiser                     ds (.SerClk           (ExtClk10), 
                                    .BitReverse       (1'b0),
                                    .ParInVev         (160'h0), 
                                    .SerOut           (), 
                                    .SerIn            (SerLink), 
                                    .ParOut           (Link)
                                    );
`endif

 PcieDispLinkCore #(NumOfLanes) dl (.ExtClk           (ExtClk), 
                                    .Link             (Link),
                                    .notReset         (notReset),
                                    .FwdName          (FwdName),
                                    .BckName          (BckName),
                                    .DispDataIn       (DispDataIn),
                                    .DispDataOut      (DispDataOut),
                                    .DispValIn        (DispValIn),
                                    .LinkWidth        (LinkWidth),
                                    .DisableScramble  (DisableScramble),
                                    .InvertTxPolarity (InvertTxPolarity),
                                    .NodeNum          (NodeNum)
                                    );

endmodule

//-------------------------------------------------------------
// PcieDispLinkCore
//-------------------------------------------------------------

module PcieDispLinkCore (ExtClk, Link, notReset, FwdName, BckName, DispDataIn, DispDataOut, DispValIn, LinkWidth, DisableScramble, InvertTxPolarity, NodeNum);

parameter NumOfLanes = 1;

input                       ExtClk; 
input                       DisableScramble;
input [15:0]                InvertTxPolarity;
input [(NumOfLanes*10)-1:0] Link;

input                       notReset;
input [7:0]                 FwdName;
input [7:0]                 BckName;
input [7:0]                 NodeNum;
input [`DispBits]           DispValIn;

input [`DispDataInBits]     DispDataIn;
input [4:0]                 LinkWidth;
output [`DispDataOutBits]   DispDataOut;

reg [7:0]                   Buf [0:(4095+28)];
reg [(4095+28):0]           BufCtrl;

reg [63:0]                  CompletionData [0:`CmplSaveDepth-1];
reg [`CmplSaveDepth-1:0]    ComplPending;
integer                     HighestCmplSlot;

reg                         TlpDispEn;
reg [7:0]                   TlpName;
reg                         DllpDispEn;
reg [7:0]                   DllpName;
reg [NumOfLanes-1:0]        SyncedLast;

integer                     idx, i, j, k;
integer                     NewLine, ByteCount, IsTlp;

reg [11:0]                  DllpParam1;
reg [7:0]                   DllpParam2;
reg [7:0]                   DllpType;
reg [31:0]                  TlWord;
reg [7:0]                   NewByte;
reg [`DispDataOutBits]      DispDataOut;
integer                     BufOffset;

reg [31:0]                  DlSeqNum, DllpWord, TlWord0, TlWord1, TlWord2, TlWord3;
reg [31:0]                  PayloadLen, EndAddr, StartAddr, HdrLength;

reg                         TlTD, TlED, TlBCM;
reg                         MemoryRequest, IORequest, ConfigRequest, MessageRequest, Completion;
reg [1:0]                   TlFmt, TlAttr;
reg [2:0]                   TlTC, TlCStatus, TlFuncNum;
reg [3:0]                   TlLastBE, TlFirstBE, TlExtRegNum;
reg [4:0]                   TlDevNum;
reg [5:0]                   TlRegNum;
reg [6:0]                   TlType, TlLowAddr;
reg [7:0]                   TlTAG, TlCTAG, TlBusNum;
reg [9:0]                   TlLength;
reg [11:0]                  TlByteCount;
reg [15:0]                  TlID, TlCRID;

task CalcHeader;
begin

    // Extract Transaction layer header information (PCIE words are big endian)
    DlSeqNum       = {16'h0000,Buf[1],  Buf[2]};
    DllpWord       = {Buf[1],  Buf[2],  Buf[3],  Buf[4]};
    TlWord0        = {Buf[3],  Buf[4],  Buf[5],  Buf[6]};
    TlWord1        = {Buf[7],  Buf[8],  Buf[9],  Buf[10]};
    TlWord2        = {Buf[11], Buf[12], Buf[13], Buf[14]};
    TlWord3        = {Buf[15], Buf[16], Buf[17], Buf[18]};

    // Decode the header information

    // TL header variables
    TlFmt          = TlWord0[`TL_FMTBITS]    & {2{(^TlWord0[`TL_FMTBITS]) !== 1'bx}}; // If not loaded Fmt bits yet, assume a 3DW header
    TlTD           = TlWord0[`TL_TDBITS]     & (TlWord0[`TL_TDBITS] !== 1'bx);
    TlLength       = TlWord0[`TL_LENGTHBITS] & {10{(^TlWord0[`TL_LENGTHBITS]) !== 1'bx}};
    TlType         = TlWord0[`TL_CODEBITS];
    TlTC           = TlWord0[`TL_TCBITS];
    TlED           = TlWord0[`TL_EPBITS];
    TlAttr         = TlWord0[`TL_ATTRBITS];

    // Mem, I/O and config value
    TlID           = TlWord1[31:16];
    TlTAG          = TlWord1[15:8];
    TlLastBE       = TlWord1[7:4];
    TlFirstBE      = TlWord1[3:0];
    TlBusNum       = TlWord2[31:24];
    TlDevNum       = TlWord2[23:19];
    TlFuncNum      = TlWord2[18:16];
    TlExtRegNum    = TlWord2[11:8];
    TlRegNum       = TlWord2[7:2];

    // Completer values
    TlCStatus      = TlWord1[15:13];
    TlBCM          = TlWord1[12];
    TlByteCount    = TlWord1[11:0];
    TlCRID         = TlWord2[31:16];
    TlCTAG         = TlWord2[15:8];
    TlLowAddr      = TlWord2[6:0];

    PayloadLen     = (TlFmt[1] == 1'b1) ? ({22'h000000, TlLength} + (~(|TlLength) ? 1024 : 0)): 0;
    HdrLength      = ({22'h000000, TlLength} + (~(|TlLength) ? 1024 : 0));
    StartAddr      = TlFmt[0] ? TlWord3 : TlWord2;
    EndAddr        = StartAddr + (HdrLength << 2) - 1;

    MemoryRequest  = ~(|TlType[4:1]);
    IORequest      = ~TlType[4] & ~TlType[3] &  TlType[1];
    ConfigRequest  = ~TlType[4] &  TlType[2];
    MessageRequest =  TlType[4] & ~TlType[3];
    Completion     = ~TlType[4] &  TlType[3];

end
endtask

// Flag TLP data sub-catagories
  
reg [15:0] DllpCrc;
reg [31:0] Ecrc, Lcrc;

task DispRoute;
input [2:0] route;
input Disp;
input [15:0] RID;
begin
    case (route)
    `TL_ROUTE_ROOT  : if (Disp) $display("(route to root complex)");
    `TL_ROUTE_ADDR  : if (Disp) $display("(route by address)");
    `TL_ROUTE_ID    : if (Disp) $display("(route by ID = 0x%h)", RID);
    `TL_ROUTE_BCAST : if (Disp) $display("(broadcast from root complex)");
    `TL_ROUTE_LOCAL : if (Disp) $display("(Local)");
    `TL_ROUTE_GATHER: if (Disp) $display("(Gather and route to root complex)");
    default         : prot_err(`TXN_2_12_7); //$display("(**illegal route**)");
    endcase
end
endtask

task DispComp;
input [2:0] status;
input Disp;
begin
    case (status)
    3'b000: if (Disp) $write("Successful ");
    3'b001: if (Disp) $write("Unsupported Request ");
    3'b010: if (Disp) $write("Config Req Retry Status ");
    3'b100: if (Disp) $write("Completer Abort ");
    default: prot_err(`TXN_2_21_7); //$write("**illegal completion status** ");
    endcase
    if(Disp) 
        $display("CID=%h BCM=%b Byte Count=%h RID=%h TAG=%h Lower Addr=%h", 
                  TlID, TlBCM, TlByteCount, TlCRID, TlCTAG, TlLowAddr);
end
endtask

task DispMsg;
input [31:0] word1, word2, word3;
input Disp;
begin
    case (word1[7:0])
    `TL_ASSERT_INTA        : if(Disp) $write("Assert INTA ");
    `TL_ASSERT_INTB        : if(Disp) $write("Assert INTB ");
    `TL_ASSERT_INTC        : if(Disp) $write("Assert INTC ");
    `TL_ASSERT_INTD        : if(Disp) $write("Assert INTD ");
    `TL_DEASSERT_INTA      : if(Disp) $write("Deassert INTA ");
    `TL_DEASSERT_INTB      : if(Disp) $write("Deassert INTB ");
    `TL_DEASSERT_INTC      : if(Disp) $write("Deassert INTC ");
    `TL_DEASSERT_INTD      : if(Disp) $write("Deassert INTD ");
    `TL_PM_ACTIVE_STATE_NAK: if(Disp) $write("PM Active state NAK ");
    `TL_PM_PME             : if(Disp) $write("PM Power Management Enable ");
    `TL_PM_TURN_OFF        : if(Disp) $write("PM Turn Off ");
    `TL_PM_TO_ACK          : if(Disp) $write("PM TO Ack ");
    `TL_ERR_COR            : if(Disp) $write("Error Correctable ");
    `TL_ERR_NON_FATAL      : if(Disp) $write("Error Non-fatal ");
    `TL_ERR_FATAL          : if(Disp) $write("Error Fatal ");
    `TL_UNLOCK             : if(Disp) $write("Unlock locked transaction ");
    `TL_SET_SLOT_PWR_LIM   : if(Disp) $write("Set slot power limit ");
    `TL_VENDOR_TYPE0       : if(Disp) $write("Vendor type 0 ");
    `TL_VENDOR_TYPE1       : if(Disp) $write("Vendor type 1 ");
    default                : prot_err(`TXN_2_12_2); //$write("**illegal Msg code** ");
    endcase

    if (Disp)
        $write("ID=%h TAG=%h ", word1[31:16], word1[15:8]);

    if (Disp && (word1[7:0] == `TL_VENDOR_TYPE0 || word1[7:0] == `TL_VENDOR_TYPE1))
        $write("Vendor ID=%h Vendor Data=%h ", word2[15:0], word3);
end
endtask

task DispVendor;
begin
end
endtask

task DispRawData;
input Control;
input [7:0] Byte;
begin

    if (Control === 1'bx || ^Byte === 1'bx)
        $write("  X ");
    else if (Control === 1'bz && Byte === 8'bzzzzzzzz)
        $write("  Z ");
    else if (!Control)
    begin
        if (Byte < 16)
            $write(" 0%0h ", Byte);
        else
            $write(" %0h ", Byte);
    end
    else
    begin
        case (Byte)
        `IDL: $write("IDL ");
        `COM: $write("COM ");
        `PAD: $write("PAD ");
        `FTS: $write("FTS ");
        `SKP: $write("SKP ");
        `SDP: $write("SDP ");
        `STP: $write("STP ");
        `END: $write("END ");
        `EDB: $write("EDB ");
        default:
            $write("??? ");
        endcase
    end
end
endtask

function [31:0] MungeCrc;
input [31:0] Crc;
begin
    MungeCrc = ~{Crc[24], Crc[25], Crc[26], Crc[27], Crc[28], Crc[29], Crc[30], Crc[31],
                 Crc[16], Crc[17], Crc[18], Crc[19], Crc[20], Crc[21], Crc[22], Crc[23],
                 Crc[08], Crc[09], Crc[10], Crc[11], Crc[12], Crc[13], Crc[14], Crc[15],
                 Crc[00], Crc[01], Crc[02], Crc[03], Crc[04], Crc[05], Crc[06], Crc[07]};
end
endfunction

function [15:0] MungeCrc16;
input [15:0] Crc;
begin
    MungeCrc16 = ~{Crc[08], Crc[09], Crc[10], Crc[11], Crc[12], Crc[13], Crc[14], Crc[15],
                   Crc[00], Crc[01], Crc[02], Crc[03], Crc[04], Crc[05], Crc[06], Crc[07]};
end
endfunction


// Extract a clock from lane0---which is always present
wire [NumOfLanes-1:0] LinkClk;

assign LinkClk = {NumOfLanes{ExtClk}};
wire Clk = LinkClk[0];
wire [NumOfLanes-1:0] Control, Synced;
wire [(8*NumOfLanes)-1:0] Byte, ByteRaw;
wire [`DispBits] DispVal = DispValIn | {`NoDispBits{DispValIn[`DispAll]}};


wire [159:0] LaneNums = {5'h1f, 5'h1e, 5'h1d, 5'h1c, 5'h1b, 5'h1a, 5'h19, 5'h18, 
                         5'h17, 5'h16, 5'h15, 5'h14, 5'h13, 5'h12, 5'h11, 5'h10, 
                         5'h0f, 5'h0e, 5'h0d, 5'h0c, 5'h0b, 5'h0a, 5'h09, 5'h08, 
                         5'h07, 5'h06, 5'h05, 5'h04, 5'h03, 5'h02, 5'h01, 5'h00};

wire [(NumOfLanes*5)-1:0] LaneNum = LaneNums[(NumOfLanes*5)-1:0];

wire [15:0] LaneEnable = ~(16'hffff << LinkWidth);

    // An array of instances of the Physical Lane logic
    PcieDispLinkLane dlpl[NumOfLanes-1:0] (
                         .Link             (Link),
                         .Clk              (LinkClk),
                         .RxByte           (Byte),
                         .RxByteRaw        (ByteRaw),
                         .RxControl        (Control),
                         .Synced           (Synced),
                         .notReset         (notReset),
                         .FwdName          (FwdName),
                         .DispVal          (DispVal),
                         .LaneNum          (LaneNum),
                         .Enable           (LaneEnable),
                         .DisableScramble  (DisableScramble),
                         .InvertTxPolarity (InvertTxPolarity),
                         .NodeNum          (NodeNum)
                         );

reg [127:0] ShiftByte;
reg [31:0] Tmp;

always @(posedge Clk)
begin
    //////////////////////
    // Termination
    //////////////////////

    if (DispValIn[`DispFinish])
    begin
      $finish;
    end

    if (DispValIn[`DispStop])
    begin
      $stop;
    end

    //////////////////////
    // Raw Data
    //////////////////////

    if (DispVal[`DispRawSym])
    begin
        $write("PCIE%c%0d: ", FwdName, NodeNum);
        for (idx=0; idx < {27'h0000000, LinkWidth}; idx = idx + 1)
        begin
`ifdef DESCRAMBLE_RAW_PCIE
            ShiftByte = Byte >> (idx*8);
`else
            ShiftByte = ByteRaw >> (idx*8);
`endif
            NewByte = ShiftByte[7:0];
            DispRawData(Control[idx], NewByte);
        end
        $display("");
    end

    #`RegDel
    for (idx=0; idx < {27'h0000000, LinkWidth}; idx = idx + 1)
    begin

        if(Synced[idx] && SyncedLast[idx] !== 1'b1)
        begin
            $display("PCIE%c%0d: Lane %0d has synchronised at time %0d", FwdName, NodeNum, idx, $time);
            SyncedLast[idx] = 1'b1;
        end
       
        ShiftByte          = Byte >> (idx*8);
        NewByte            = ShiftByte[7:0];
        Buf[ByteCount]     = NewByte;
        BufCtrl[ByteCount] = Control[idx];


        if(Control[idx])
        begin
            case (NewByte)
            `IDL, `COM, `PAD, `FTS, `SKP : ;
            `SDP : begin IsTlp = 1; ByteCount = ByteCount + 1; end
            `STP : begin IsTlp = 1; ByteCount = ByteCount + 1; end
            `END : begin IsTlp = 0; ByteCount = ByteCount + 1; end
            `EDB : begin IsTlp = 0; ByteCount = ByteCount + 1; end
            default: 
            begin 
                if (&Synced) 
                begin 
                    prot_err(`PHY_2_1_1); 
                end 
            end
            endcase
        end
        else
        begin
            if (NewByte !== 8'hxx && (IsTlp != 32'h00000000))
            begin
                ByteCount = ByteCount + 1;
            end
        end

        if (Control[idx] === 1'b1 && (NewByte === `END || NewByte === `EDB))
        begin
            CalcHeader;
            for (j = 0; j < ByteCount; j = j + 1)
            begin
                if (BufCtrl[j])
                begin
                    if (DispVal[`DispPL])
                    begin
                       if      (Buf[j] == `SDP) begin $display("PCIE%c%0d: {SDP ", FwdName, NodeNum); $write("PCIE%c%0d: ", FwdName, NodeNum); end
                       else if (Buf[j] == `STP) begin $display("PCIE%c%0d: {STP ", FwdName, NodeNum); $write("PCIE%c%0d: ", FwdName, NodeNum); end
                       else if (Buf[j] == `END) begin if ((NewLine == 0) ? 1'b1 : 1'b0) begin $display(""); $write("PCIE%c%0d: ", FwdName, NodeNum); end $display("END}"); end
                       else if (Buf[j] == `EDB) begin if ((NewLine == 0) ? 1'b1 : 1'b0) begin $display(""); $write("PCIE%c%0d: ", FwdName, NodeNum); end $display("EDB}"); end
                    end
                end
                else
                begin
                    NewLine = 0;
                    if (DispVal[`DispPL]) $write("%h ", Buf[j]);
                    if (j > 0 && ((j-1)%`LINEBREAK) == (`LINEBREAK-1))
                    begin
                        if (DispVal[`DispPL]) $display("");
                        if (DispVal[`DispPL]) $write("PCIE%c%0d: ", FwdName, NodeNum);
                        NewLine = 1;
                    end
                end
            end

            //////////////////////
            // Data link layer 
            //////////////////////

            if (BufCtrl[0] && Buf[0] == `SDP)
            begin
                DllpType   = Buf[1];
                Tmp        = {16'h0000, {Buf[3], Buf[4]} & 16'h0fff};
                DllpParam1 = Tmp[11:0];
                Tmp        = {16'h0000, ({Buf[2], Buf[3]} & 16'h3fc0) >> 6};
                DllpParam2 = Tmp[7:0];

                DllpName   = FwdName;
                DllpDispEn = DispVal[`DispDL];

                if (DllpDispEn) 
                begin
                     $write("PCIE%c%0d: ", DllpName, NodeNum);
                    if (DispVal[`DispPL]) 
                    begin
                        $write(`PADSTR);
                    end
                end

                case({DllpType[7:3], 3'b000})
                `DL_ACK         : if (DllpDispEn) $display("DL Ack seq %0d", DllpParam1);
                `DL_NAK         : if (DllpDispEn) $display("DL Nak seq %0d", DllpParam1);
                `DL_INITFC1_P   : if (DllpDispEn) $display("DL InitFC1-P    VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_INITFC1_NP  : if (DllpDispEn) $display("DL InitFC1-NP   VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_INITFC1_CPL : if (DllpDispEn) $display("DL InitFC1-Cpl  VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_INITFC2_P   : if (DllpDispEn) $display("DL InitFC2-P    VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_INITFC2_NP  : if (DllpDispEn) $display("DL InitFC2-NP   VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_INITFC2_CPL : if (DllpDispEn) $display("DL InitFC2-Cpl  VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_UPDATEFC_P  : if (DllpDispEn) $display("DL UpdateFC-P   VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_UPDATEFC_NP : if (DllpDispEn) $display("DL UpdateFC-NP  VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_UPDATEFC_CPL: if (DllpDispEn) $display("DL UpdateFC-Cpl VC%d  HdrFC=%0d DataFC=%0d", DllpType[2:0], DllpParam2, DllpParam1);
                `DL_VENDOR      : if (DllpDispEn) $display("DL Vendor Specific DLLP");
                `DL_PM_ENTER_L1 :
                begin
                    if (DllpType[2:0] == 0)
                    begin
                        if (DllpDispEn) $display("DL PM_Enter_L1");
                    end
                    else if (DllpType[2:0] == 1)
                    begin
                        if (DllpDispEn) $display("DL PM_Enter_L23");
                    end
                    else if (DllpType[2:0] == 2)
                    begin
                        if (DllpDispEn) $display("DL PM_Active_State_Request_L0s");
                    end
                    else if (DllpType[2:0] == 3)
                    begin
                        if (DllpDispEn) $display("DL PM_Active_State_Request_L1");
                    end
                    else if (DllpType[2:0] == 4)
                    begin
                        if (DllpDispEn) $display("DL PM_Request_Ack");
                    end
                end
                endcase

                if (DllpDispEn) 
                begin
                     $write("PCIE%c%0d: ", DllpName, NodeNum);
                    if (DispVal[`DispPL]) 
                    begin
                        $write(`PADSTR);
                    end
                end

                DllpCrc = 16'hffff;
                $pcicrc16({Buf[4],  Buf[3],  Buf[2],  Buf[1]}, DllpCrc);
                DllpCrc = MungeCrc16(DllpCrc);
 
                if (DllpCrc != {Buf[5], Buf[6]})
                begin
                    $display("DL Warning : **BAD PCIe DLLP CRC** (%x v %x)", DllpCrc, {Buf[5], Buf[6]});
                end
                else
                begin
                    if (DispVal[`DispDL]) 
                    begin
                        $display("DL Good DLLP CRC (%x)", DllpCrc);
                    end
                end
            end

            TlpDispEn  = DispVal[`DispTL];
            DllpDispEn = DispVal[`DispDL];
            TlpName    = FwdName;

            // Display the DL sequence number
            if (DllpDispEn && BufCtrl[0] && Buf[0] == `STP)
            begin
                $write("PCIE%c%0d: ", TlpName, NodeNum);
                if(DispVal[`DispPL]) 
                begin
                    $write(`PADSTR);
                end
                $display("DL Sequence number=%0d", DlSeqNum[11:0]);
            end

            //////////////////////
            // Transaction Layer
            //////////////////////

            if (BufCtrl[0] && Buf[0] == `STP)
            begin
                if (TlpDispEn && DispVal[`DispTL]) $write("PCIE%c%0d: ", TlpName, NodeNum);
                if (TlpDispEn && DispVal[`DispDL]) $write(`PADSTR);
                if (TlpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                if (TlpDispEn && DispVal[`DispTL]) $write("TL ");
                casex(TlType)
                `TL_MRD32    : if (TlpDispEn) $display("MEM read req Addr=%h (32) RID=%h TAG=%h FBE=%b LBE=%b Len=%h", TlWord2, TlID, TlTAG, TlFirstBE, TlLastBE, TlLength );
                `TL_MRD64    : if (TlpDispEn) $display("MEM read req Addr=%h (64) RID=%h TAG=%h FBE=%b LBE=%b Len=%h", {TlWord2, TlWord3}, TlID, TlTAG, TlFirstBE, TlLastBE, TlLength);
                `TL_MRDLCK32 : if (TlpDispEn) $display("MEM read req Addr=%h (32) LOCKED ID=%h TAG=%h FBE=%b LBE=%b Len=%h", TlWord2, TlID, TlTAG, TlFirstBE, TlLastBE, TlLength);
                `TL_MRDLCK64 : if (TlpDispEn) $display("MEM read req Addr=%h (64) LOCKED ID=%h TAG=%h FBE=%b LBE=%b Len=%h", {TlWord2, TlWord3}, TlID, TlTAG, TlFirstBE, TlLastBE, TlLength);
                `TL_MWR32    : if (TlpDispEn) $display("MEM write req Addr=%h (32) RID=%h TAG=%h FBE=%b LBE=%b", TlWord2, TlID, TlTAG, TlFirstBE, TlLastBE);
                `TL_MWR64    : if (TlpDispEn) $display("MEM write req Addr=%h (64) RID=%h TAG=%h FBE=%b LBE=%b", {TlWord2, TlWord3}, TlID, TlTAG, TlFirstBE, TlLastBE);
                `TL_IORD     : if (TlpDispEn) $display("IO read req Addr=%h (32) RID=%h TAG=%h FBE=%b LBE=%b", TlWord2, TlID, TlTAG, TlFirstBE, TlLastBE);
                `TL_IOWR     : if (TlpDispEn) $display("IO write req Addr=%h (32) RID=%h TAG=%h FBE=%b LBE=%b", TlWord2, TlID, TlTAG, TlFirstBE, TlLastBE);
                `TL_CFGRD0   : if (TlpDispEn) $display("Config read type 0 RID=%h TAG=%h FBE=%b LBE=%b Bus=%h Dev=%h Func=%h EReg=%h Reg=%h", TlID, TlTAG, TlFirstBE, TlLastBE, TlBusNum, TlDevNum, TlFuncNum, TlExtRegNum, TlRegNum);
                `TL_CFGWR0   : if (TlpDispEn) $display("Config write type 0 RID=%h TAG=%h FBE=%b LBE=%b Bus=%h Dev=%h Func=%h EReg=%h Reg=%h", TlID, TlTAG, TlFirstBE, TlLastBE, TlBusNum, TlDevNum, TlFuncNum, TlExtRegNum, TlRegNum);
                `TL_CFGRD1   : if (TlpDispEn) $display("Config read type 1 RID=%h TAG=%h FBE=%b LBE=%b Bus=%h Dev=%h Func=%h EReg=%h Reg=%h", TlID, TlTAG, TlFirstBE, TlLastBE, TlBusNum, TlDevNum, TlFuncNum, TlExtRegNum, TlRegNum);
                `TL_CFGWR1   : if (TlpDispEn) $display("Config write type 1 RID=%h TAG=%h FBE=%b LBE=%b Bus=%h Dev=%h Func=%h EReg=%h Reg=%h", TlID, TlTAG, TlFirstBE, TlLastBE, TlBusNum, TlDevNum, TlFuncNum, TlExtRegNum, TlRegNum);
                `TL_CPL      : begin if (TlpDispEn) $write("Completion "); DispComp(TlCStatus, TlpDispEn); end
                `TL_CPLD     : begin if (TlpDispEn) $write("Completion with Data "); DispComp(TlCStatus, TlpDispEn); end
                `TL_CPLLK    : begin if (TlpDispEn) $write("Completion LOCKED "); DispComp(TlCStatus, TlpDispEn); end
                `TL_CPLDLK   : begin if (TlpDispEn) $write("Completion with data LOCKED "); DispComp(TlCStatus, TlpDispEn); end
                `TL_MSG      : begin if (TlpDispEn) $write("Message req "); DispMsg(TlWord1, TlWord2, TlWord3, TlpDispEn); DispRoute(TlType[2:0], TlpDispEn, TlWord2[31:16]); end
                `TL_MSGD     : begin if (TlpDispEn) $write("Message req with Data "); DispMsg(TlWord1, TlWord2, TlWord3, TlpDispEn); DispRoute(TlType[2:0], TlpDispEn, TlWord2[31:16]); end
                default      : $display("**Error** : Illegal transaction type (%b)", TlType);
                endcase

                if (TlpDispEn)
                begin
                    $write("PCIE%c%0d: ", TlpName, NodeNum);
                    if (DispVal[`DispDL]) $write(`PADSTR);
                    if (DispVal[`DispPL]) $write(`PADSTR);
                    $write("Traffic Class=%0d", TlTC);
                end

                if (TlTD)
                    if (TlpDispEn) $write (", TLP Digest");
                if (TlED)
                    if (TlpDispEn) $write (", Poisoned");
                if (TlAttr[1])
                    if (TlpDispEn) $write (", Relaxed ordering (PCI-X)");
                else
                    if (TlpDispEn) $write (", Strong ordering (PCI)");
  
                if (TlAttr[0])
                    if (TlpDispEn) $write (", No Snoop");
  
                if (TlFmt[1])
                    if (TlpDispEn) $write(", Payload Length=0x%h DW", PayloadLen[9:0]);
 
                if (TlpDispEn) 
                    $display("");
 
                // Display the data payload (if any)
 
                // Calculate the offset into the byte buffer where the payload starts
                BufOffset = 15 + (TlFmt[0] ? 4 : 0);
                for (i = 0; i < PayloadLen; i = i + 1)
                begin
                    // Construct a header word
                    TlWord = {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]};
                    
                    if (i%(`DispLineBrk) == 0)
                    begin
                        if (TlpDispEn) $write("PCIE%c%0d: ", TlpName, NodeNum);
                        if (TlpDispEn && DispVal[`DispDL]) $write(`PADSTR);
                        if (TlpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                    end
 
                    if (TlpDispEn) $write("%x ", TlWord);
                    if (i%(`DispLineBrk) == (`DispLineBrk-1))
                        if (TlpDispEn) $display("");
                end 

                if (((i%`DispLineBrk) != 0) ? 1'b1 : 1'b0)
                    if (TlpDispEn) $display("");

                if (TlTD)
                begin
                    Ecrc = 32'hffffffff;
                    for (k = 3; k < (3 + (TlFmt[0] ? 16 : 12) + PayloadLen*4); k = k + 1)
                        $pcicrc32(Buf[k] | {1'b0, k==5 ? 1'b1 : 1'b0, 5'b00000, k==3 ? 1'b1 : 1'b0}, Ecrc, 8);
                    Ecrc = MungeCrc(Ecrc);
                    if (Ecrc == {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]})
                    begin
                        if (TlpDispEn) $write("PCIE%c%0d: ", TlpName, NodeNum);
                        if (TlpDispEn && DispVal[`DispDL]) $write(`PADSTR);
                        if (TlpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                        if (TlpDispEn) $display("TL Good ECRC (%x)", {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]});
                    end
                    else
                    begin
                        $write("PCIE%c%0d: ", TlpName, NodeNum);
                        if (TlpDispEn && DispVal[`DispDL]) $write(`PADSTR);
                        if (TlpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                        $display("TL **Bad ECRC** (%x v %x)", MungeCrc(Ecrc), {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]});
                    end
                    i = i + 1;
                end
                else
                begin
                    if (TlpDispEn) $write("PCIE%c%0d: ", TlpName, NodeNum);
                    if (TlpDispEn && DispVal[`DispDL]) $write(`PADSTR);
                    if (TlpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                    if (TlpDispEn) $display("TL No ECRC");
                end
            end
            Lcrc = 32'hffffffff;
            for (k = 1; k < (3 + (TlFmt[0] ? 16 : 12) + PayloadLen*4 + (TlTD ? 4 : 0)); k = k + 1)
                $pcicrc32(Buf[k], Lcrc, 8);
            Lcrc = MungeCrc(Lcrc);
            if (BufCtrl[0] && Buf[0] == `STP)
            begin
                if (Lcrc === {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]})
                begin
                   if (DllpDispEn) $write("PCIE%c%0d: ", TlpName, NodeNum);
                   if (DllpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                   if (DllpDispEn) $display("DL Good LCRC (%x)", {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]});
                end
                else
                begin
                    $write("PCIE%c%0d: ", TlpName, NodeNum);
                    if (DllpDispEn && DispVal[`DispPL]) $write(`PADSTR);
                    $display("DL **Bad LCRC** (%x v %x)", MungeCrc(Lcrc), {Buf[4*i+BufOffset], Buf[4*i+BufOffset+1], Buf[4*i+BufOffset+2], Buf[4*i+BufOffset+3]});
                end
            end

            CheckTransaction(ByteCount);

            ByteCount = 0;
            Buf[0] = `IDL;
        end
    end
end


initial
begin
    ComplPending    = 0;
    HighestCmplSlot = 0;
    DispDataOut     = 0;
    ByteCount       = 0;
    IsTlp           = 0;
    for (i = 0; i < 16; i = i + 1)
    begin
        Buf[i] = 0;
    end
end

reg firstProtErr;

initial
begin
    firstProtErr = 1;
end 

task prot_err;
input [31:0] error_type;
begin

`ifndef DISABLE_PCI_CHECKLIST

    if (firstProtErr)
    begin
        $display;
        $display("PCIE%c%0d: ************************** Protocol Error Report For Transaction Layer Packets **************************", FwdName, NodeNum);     
    end

    $write ("PCIE%c%0d: ", FwdName, NodeNum);

    case(error_type)
    `size_difference:
        $write("Packet of unexpected size:-");
      
//  `TXN_2_0_1: same as TXN.2.21#3
    `TXN_2_0_2:
        $write("TXN.2.0#2 - Reserved TLP bits should always be zero"); 

    `TXN_2_1_2:     
        $write("TXN.2.1#2 - Use of reserved Fmt/Type value");
    `TXN_2_1_4:
        $write("TXN.2.1#4 - Data is not four-byte aligned");
    `TXN_2_1_5:      
   //also TXN.2.2#2
        $write("TXN.2.1#5 - Length field should be zero if no data payload");
      
//  `TXN_2_2_1:; 
//  `TXN_2_2_2: see TXN_2_1_5
//  `TXN_2_2_3: setting of max_payload_size if PayloadLen > Max_Payload_Size prot_error(TXN_2_2_3); 
//  `TXN_2_2_4: setting of max_payload_size; 
    `TXN_2_2_5:
        $write("TXN.2.2#5 - Payload and Length do not match");
    `TXN_2_2_6:
//      also TXN.2.11#9
        $write("TXN.2.2#6 - Address/Length combination crosses a 4k boundary");

    `TXN_2_3_1:
        $write("TXN.2.3#1 - Either Corrupted TD field or the Payload and Length do not match"); 

    `TXN_2_4_1: 
        $write("TXN.2.4#1 - For addresses below 4GB 32 bit addressing format should be used");
    `TXN_2_4_2:
        $write("TXN.2.4#2 - IO Requests must use 32 bit format only"); 

    `TXN_2_6_1:
        $write("TXN.2.6#1 - For a payload of over 1DW first DW BE[3:0] must not be 0000b");
    `TXN_2_6_2:
        $write("TXN.2.6#2 - For a payload of 1DW last DW BE[3:0] must equal 0000b");
    `TXN_2_6_3:
        $write("TXN.2.6#3 - For a payload of more than 1DW last DW BE[3:0] must not equal 0000b");
    `TXN_2_6_9:
        $write("TXN.2.6#9 - Completion of a zero-length Memory Read Request must have a 1DW data payload");
    `TXN_2_7_2: 
        $write("TXN.2.7#2 - Upper three bits of the tag must be zero if the Extended Tag bit is not set");
    `TXN_2_8_1: 
        $write("TXN.2.8#1 - Relaxed Ordering attribute should be set to zero");
    `TXN_2_9_1: 
        $write("TXN.2.9#1 - No Snoop attribute should be set to zero");
    `TXN_2_11_4: 
        $write("TXN.2.11#4 - I/O request restrictions not met (TC, Attr, Length or Last DW BE)");
    `TXN_2_11_7: 
        $write("TXN.2.11#7 - Config request restrictions not met (TC, Attr, Length or Last DW BE)");
    `TXN_2_12_2:
        $write("TXN.2.12#2 - Use of reserved message type field");
    `TXN_2_12_7:
        $write("TXN.2.12#7 - Use of reserved message routing type");
    `TXN_2_21_3:
        $write("TXN.2.21#3 - Routing ID and RequesterID from corresponding request do not correspond"); 
    `TXN_2_21_7:
        $write("TXN.2.21#7 - Use of reserved completion status");
    `TXN_2_21_8:
        $write("TXN.2.21#8 - BCM - this bit must not be set by PCI Express Completers, and may only be set by PCI-X completers.");
    `TXN_2_21_19:
        $write("TXN.2.21#19 - RequesterID, Attributes & Traffic Class should be identical to the corresponding request"); 
    `TXN_2_21_13:
        $write("TXN.2.21#13 - Lower Address[6:0] should be zero for all NON memory read completions "); 
    `TXN_2_21_22:
        $write("TXN.2.21#22 - Byte Count must be 4 for all NON memory read completions"); 
    `TXN_3_2_3:
        $write("TXN.3.2#3 - Config or IO Reads must be completed with a single completion"); 
    `TXN_2_7_1:
        $write("TXN.2.7#1 - Re-use of an outstanding tag");
    `TXN_3_3_1:
        $write("TXN.3.3#1 & TXN.3.3#2 - Completion received for non-existant request must be discarded and reported as an error");
    `TXN_7_1_6:
        $write("TXN.7.1#6 - ECRC does not meet section 2.7.1 specification");

    `DLL_4_1_34:
        $write("DLL.4.1#34 - DLLP 16 bit CRC not applied correctly");
    `DLL_4_1_5_to_33:
        $write("DLL.4.1#(5 to 33) - Use of reserved DLLP type");
    `DLL_5_2_20:
        $write("DLL.5.2#20 - LCRC not applied correctly at TX");

    `PHY_2_1_1:
        $write("PHY.2.1#1 - Invalid symbol and/or bad disparity");
    endcase
    $display(" at time %0d", $time);
    firstProtErr = 0;
`ifndef DISABLE_DEAF_ON_PCI_CHECKLIST
    `deaf
`endif
`endif
end
endtask

///////////////////////////////////////////////////////////////////////////////////
// Task to check transactions seen on this link
//
task CheckTransaction;

input [31:0] TLPByteCount;

reg [31:0] TLPLastByteNum;
integer    FirstDataByte, LastDataByte;
integer    i1, CplSlot;
reg [31:0] TmpWord0, TmpWord1;    
reg [7:0]  TmpTag;
reg [15:0] TmpRID;
reg        CmplMatch;
begin

`ifndef DISABLE_PCI_CHECKLIST

    TLPLastByteNum = TLPByteCount - 2;   // Last byte before the END/EDP symbol

    if (BufCtrl[0] && Buf[0] == `STP)
    begin
        firstProtErr = 1'b1;
            
        // Assumes TlFmt[0] and TlTD are not corrupted 
        FirstDataByte = `SEQ_NUM_LEN + `MIN_HDR_LEN + (TlFmt[0] << 2);
        LastDataByte  = TLPLastByteNum - `LCRC_LEN - (TlTD << 2);
            
        // General Protocol Errors        

        if ((TLPLastByteNum) != `SEQ_NUM_LEN + `MIN_HDR_LEN + `LCRC_LEN + ((TlFmt[0] + PayloadLen + TlTD) << 2))
        begin   
            prot_err(`size_difference);
            $write("    TLP buffer = ");
            for(i = 1; i <= (TLPLastByteNum); i = i + 1)
                $write(" %h ", Buf[i]);
            $display("%m Expected: %d Actual: %d",(`SEQ_NUM_LEN + `MIN_HDR_LEN + ((TlFmt[0] + PayloadLen + TlTD) << 2)), (TLPLastByteNum+1)); 
        end
    
        if ((LastDataByte - FirstDataByte) != (PayloadLen << 2))
            prot_err(`TXN_2_3_1);
    
        if (|(TlWord0 & `RSVD_MASK))
            prot_err(`TXN_2_0_2);

        if (TlBCM && Completion)
            prot_err(`TXN_2_21_8);
   
        if (MessageRequest || Completion)
            if (~TlFmt[1] && |TlLength)
                prot_err(`TXN_2_1_5);
 
        // Export completion data to other displink
        DispDataOut[`DispComplHdr] = {TlWord0, TlWord1, TlWord2};

        // Check for already used RID/TAG
        CmplMatch = 1'b0;
        for (i1 = 0; i1 <= HighestCmplSlot; i1 = i1 + 1)
        begin
            {TmpWord0, TmpWord1} = CompletionData[i];
            TmpTag = TmpWord1[`TL_TAG];
            TmpRID = TmpWord1[`TL_REQID];
            CmplMatch = CmplMatch | (TmpRID == TlID && TmpTag == TlTAG && ComplPending[i]);
        end

        // Find a free completion save slot
        CplSlot = 0;
        while (ComplPending[CplSlot])
        begin
            CplSlot = CplSlot + 1;
            if (CplSlot == `CmplSaveDepth)
            begin
                $display("%m Ran out of free completion slots");
                `deaf
            end
        end

        if (CplSlot > HighestCmplSlot)
            HighestCmplSlot = CplSlot;
   
        // Transaction layer checking
        casex(TlType)
        `TL_MRD32    : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_MRD64    : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_MRDLCK32 : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_MRDLCK64 : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_MWR32    : ; 
        `TL_MWR64    : ;
        `TL_IORD     : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_IOWR     : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_CFGRD0   : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_CFGWR0   : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_CFGRD1   : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_CFGWR1   : begin if (CmplMatch) prot_err(`TXN_2_7_1); ComplPending[CplSlot] = 1'b1; CompletionData[CplSlot] = {TlWord0, TlWord1}; end
        `TL_CPL      : begin DispComp(TlCStatus, `CHECKONLY); DispDataOut[`DispCompl] = ~DispDataOut[`DispCompl]; end
        `TL_CPLD     : begin DispComp(TlCStatus, `CHECKONLY); DispDataOut[`DispCompl] = ~DispDataOut[`DispCompl]; end
        `TL_CPLLK    : begin DispComp(TlCStatus, `CHECKONLY); DispDataOut[`DispCompl] = ~DispDataOut[`DispCompl]; end
        `TL_CPLDLK   : begin DispComp(TlCStatus, `CHECKONLY); DispDataOut[`DispCompl] = ~DispDataOut[`DispCompl]; end
        `TL_MSG      : begin DispMsg(TlWord1, TlWord2, TlWord3, `CHECKONLY); DispRoute(TlType[2:0], `CHECKONLY, TlWord2[31:16]); end
        `TL_MSGD     : begin DispMsg(TlWord1, TlWord2, TlWord3, `CHECKONLY); DispRoute(TlType[2:0], `CHECKONLY, TlWord2[31:16]); end
        default      : prot_err(`TXN_2_1_2);
        endcase

        if (TlTD && Ecrc !== {Buf[TLPLastByteNum-7], Buf[TLPLastByteNum-6], Buf[TLPLastByteNum-5], Buf[TLPLastByteNum-4]})
            prot_err(`TXN_7_1_6);

        if (Lcrc !== {Buf[TLPLastByteNum-3], Buf[TLPLastByteNum-2], Buf[TLPLastByteNum-1], Buf[TLPLastByteNum-0]})
            prot_err(`DLL_5_2_20);

        // Errors specific to request types other than Completions
        if (~Completion)
        begin
            if ((PayloadLen > 32'h1) && (TlFirstBE == 4'b0000))
                prot_err(`TXN_2_6_1);
            if ((PayloadLen == 32'h1 && TlType != `TL_MSGD) && (TlLastBE != 4'b0000))
                prot_err(`TXN_2_6_2);
            if ((PayloadLen > 32'h1) && (TlLastBE == 4'b0000))
                prot_err(`TXN_2_6_3);
            if ((((MemoryRequest || IORequest) && ~TlFmt[0]) && |TlWord2[1:0]) || ((MemoryRequest && TlFmt[0]) && |TlWord3[1:0]))
                prot_err(`TXN_2_1_4);   
            if (MemoryRequest && (EndAddr[12] != StartAddr[12]))
            begin
                prot_err(`TXN_2_2_6);
                $display ("endaddr %h, TlWord2 %h, PayloadLen %h", EndAddr, TlWord2, PayloadLen);
            end
            if (MemoryRequest && (TlFmt[0] == 1'b1) && (TlWord2 == 32'h0))
                prot_err(`TXN_2_4_1);
            if (IORequest && (|{TlTC, TlAttr, TlLastBE} || (TlLength != 10'b1)))
                prot_err(`TXN_2_11_4);
            if (ConfigRequest && (|{TlTC, TlAttr, TlLastBE} || (TlLength != 10'b1))) 
                prot_err(`TXN_2_11_7);
            if (IORequest && TlFmt[0])
                prot_err(`TXN_2_4_2);
            if (`EXTENDED_TAG & |TlTAG[7:5]) 
                prot_err(`TXN_2_7_2);
            if (~MemoryRequest)
            begin
                if (TlAttr[1] == 1'b1)
                    prot_err(`TXN_2_8_1);
                if (TlAttr[0] == 1'b1)
                    prot_err(`TXN_2_9_1);
            end
        end
    end

    // Data Link Layer type checks
    if (BufCtrl[0] && Buf[0] == `SDP)
    begin
        case({DllpWord[31:27], 3'b000})
        `DL_ACK         , `DL_NAK       , `DL_INITFC1_P  , `DL_INITFC1_NP, `DL_INITFC1_CPL, 
        `DL_INITFC2_P   , `DL_INITFC2_NP, `DL_INITFC2_CPL, `DL_UPDATEFC_P, `DL_UPDATEFC_NP, 
        `DL_UPDATEFC_CPL, 
        `DL_VENDOR      : ; 
        `DL_PM_ENTER_L1 :
        begin
            // Check power management sub-type
            if (DllpWord[26:24] > 4)
            begin
               prot_err(`DLL_4_1_5_to_33);
            end
        end
        default:  prot_err(`DLL_4_1_5_to_33);
        endcase

        if (DllpCrc !== {Buf[5], Buf[6]})
            prot_err(`DLL_4_1_34); 
    end
`endif
end
endtask

`ifndef DISABLE_PCI_CHECKLIST

///////////////////////////////////////////////////////////////////////////////////
// Checking of completions seen on other DispLink against stored headers of
// matching transaction
//
always @(DispDataIn[`DispCompl])
begin : ProcessCompletions

reg [95:0] CmplWord;
reg [31:0] CmplWord0, CmplWord1, CmplWord2, TlStoredWord0, TlStoredWord1;
reg [7:0]  CmplCTAG;
reg [10:0] CmplLength;
reg [2:0]  CmplTC;
reg [1:0]  CmplAttr;
reg [12:0] CmplByteCount;
reg [2:0]  CmplStatus;
reg [15:0] CmplCRID;
reg [6:0]  CmplLowAddr;

reg [63:0] TlStoredHeader;
reg [1:0]  TlStoredFmt;
reg [6:0]  TlStoredType;
reg [2:0]  TlStoredTC;
reg [1:0]  TlStoredAttr;
reg [9:0]  TlStoredLength;
reg [15:0] TlStoredRID;
reg [3:0]  TlStoredLastBE;
reg [3:0]  TlStoredFirstBE;
reg [7:0]  TlStoredTAG;
reg TlStoredMemoryRequest, TlStoredIORequest, TlStoredConfigRdRequest, TlStoredMessageRequest;
integer i,  CplSlot;

    // Extract completion header data from the other Disps data
    CmplWord      = DispDataIn[`DispComplHdr];
    CmplWord0     = CmplWord[95:64];
    CmplWord1     = CmplWord[64:32];
    CmplWord2     = CmplWord[31:0];
    CmplCTAG      = CmplWord2[`TL_TAG];
    CmplLength    = {~|CmplWord0[`TL_LENGTHBITS], CmplWord0[`TL_LENGTHBITS]};
    CmplTC        = CmplWord0[`TL_TCBITS];
    CmplAttr      = CmplWord0[`TL_ATTRBITS];
    CmplStatus    = CmplWord1[`TL_COMPSTATUS];
    CmplByteCount = {~|CmplWord1[`TL_BYTECOUNT], CmplWord1[`TL_BYTECOUNT]};
    CmplCRID      = CmplWord2[`TL_REQID];
    CmplLowAddr   = CmplWord2[`TL_LWRADDR];

    // Search pending list for matching tag
    i = 0;
    while (i <= HighestCmplSlot)
    begin
        // Extract enough information to get RID and TAG
        TlStoredHeader          = CompletionData[i];
        TlStoredWord1           = TlStoredHeader[31:0];
        TlStoredRID             = TlStoredWord1[`TL_REQID];
        TlStoredTAG             = TlStoredWord1[`TL_TAG];
        
        // Check for a match
        if (TlStoredRID == CmplCRID && TlStoredTAG == CmplCTAG && ComplPending[i])
        begin
            CplSlot = i;
            i = `CmplSaveDepth;
        end
        // No more active slots, and didn't find a match
        else if (i > HighestCmplSlot)
        begin
            prot_err(`TXN_3_3_1);
        end
        else
            i = i + 1;
    end

    // Extract stored request header values for an associated completion
    //TlStoredHeader          = CompletionData[CplSlot];
    TlStoredWord0           = TlStoredHeader[63:32];
    //TlStoredWord1           = TlStoredHeader[31:0];
    TlStoredFmt             = TlStoredWord0[`TL_FMTBITS];
    TlStoredType            = TlStoredWord0[`TL_CODEBITS];
    TlStoredTC              = TlStoredWord0[`TL_TCBITS];
    TlStoredAttr            = TlStoredWord0[`TL_ATTRBITS];
    TlStoredLength          = TlStoredWord0[`TL_LENGTHBITS];
    //TlStoredRID             = TlStoredWord1[`TL_REQID];
    TlStoredLastBE          = TlStoredWord1[`TL_LASTBE];
    TlStoredFirstBE         = TlStoredWord1[`TL_FIRSTBE];
    TlStoredMemoryRequest   = ~(|TlStoredType[4:1]);
    TlStoredIORequest       = ~TlStoredType[4] & ~TlStoredType[3] &  TlStoredType[1];
    TlStoredConfigRdRequest = ~TlStoredType[6] & ~TlStoredType[4] &  TlStoredType[2];
    TlStoredMessageRequest  =  TlStoredType[4] & ~TlStoredType[3];

    if (Synced)
    begin
        if (ComplPending[CplSlot] == 1'b0)
            prot_err(`TXN_3_3_1);
        else
            // If the payload size (in bytes) is big enough to contain the
            // advertised completion byte count, then this must be the last
            // completion packet.
            if (CmplByteCount <= {CmplLength, 2'b00})
                ComplPending[CplSlot] = 1'b0;

        //Errors specific to Completions
        //$display("%m SearchTransID = %h StoredHeader = %h", SearchTransID, StoredHeader);
        if ((CmplStatus == `TL_COMPL_SC) && (TlStoredLength != CmplLength[9:0]) && (TlStoredConfigRdRequest || TlStoredIORequest && ~TlStoredFmt[1]))
            prot_err(`TXN_3_2_3);
        if (TlStoredRID !=  CmplCRID)
            prot_err(`TXN_2_21_3);
        if ({TlStoredTC, TlStoredAttr} != {CmplTC, CmplAttr})
            prot_err(`TXN_2_21_19);
        if (~(|{TlStoredType[6], TlStoredType[4:1]}) && (TlStoredLength == 10'h1) && ~|{TlStoredFirstBE, TlStoredLastBE} && CmplLength != 11'h001)
            prot_err(`TXN_2_6_9);
        if (|{TlStoredType[6], TlStoredType[4:1]}) //All requests except Memory Reads
        begin
            if (|CmplLowAddr) 
                prot_err(`TXN_2_21_13);
            if (CmplByteCount != 13'h4) begin
                $display ("TlStoredType = %h CmplByteCount =%0d", TlStoredType, CmplByteCount);
                prot_err(`TXN_2_21_22);
            end
        end
    end
end
`endif
endmodule

