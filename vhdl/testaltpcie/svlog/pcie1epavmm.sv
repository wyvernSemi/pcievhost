//=============================================================
//
// Copyright (c) 2026 Simon Southwell. All rights reserved.
//
// Date: 2nd Feb 2026
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

module pcie1epavmm
#(parameter DISABLE_INT_RESET_SEQ  = 0
)
(

  // Clock inputs and POR
  input                RefClk,
  input                PipeClk,
  input                nReset,

  // Output clock to drive logic attached to Avalon bus
  output               coreclkout,

  // BAR0 Avalon memory mapped master
  output [31:0]        Bar0Address,
  output               Bar0Read,
  input                Bar0WaitRequest,
  output               Bar0Write,
  input                Bar0ReadDataValid,
  input  [31:0]        Bar0ReadData,
  output [31:0]        Bar0WriteData,
  output  [3:0]        Bar0ByteEnable,
  output  [6:0]        Bar0BurstCount,

  // PIPE interface for lane 0
  output  [7:0]        TxData,
  output               TxDataK,
  
  output               TxDetectRx,
  output               TxElecIdle,
  output               TxCompliance,
  output               RxPolarity,
  output  [1:0]        PowerDown,
  output  [1:0]        Rate,
  output               TxDemph,
  output  [2:0]        TxMargin,
  output               TxSwing,

  input   [7:0]        RxData,
  input                RxDataK,
  input                RxValid,
  
  input                RxElecIdle,
  input   [2:0]        RxStatus,
  input                PhyStatus,

  // Useful debug signals
  output  [4:0]        LtssmState,
  output  [2:0]        EidleInferSel

);

wire [31:0] Bar0AddressAligned;
wire [63:0] Bar0WriteDataWide;
wire [63:0] Bar0ReadDataWide;
wire  [7:0] Bar0ByteEnableWide;

wire [31:0] WordAddrAdj;
wire        LowWordAddress;

wire        RxElecIdleInt;
wire [2:0]  RxStatusInt;
wire        PhyStatusInt;

// ---------------------------------------------------------
// Combinatorial logic for conversion from 64- to 32-bit
// Avalon interface
// ---------------------------------------------------------

// Flag if 64-bit access is in the low word space
assign LowWordAddress   = Bar0ByteEnableWide[3:0] != 4'b0000 ? 1'b1 : 1'b0;

// Calculate the low address adjustment from the byte enables (unary to binary)
assign WordAddrAdj      = Bar0ByteEnableWide[0] ? 32'h00000000 :
                          Bar0ByteEnableWide[1] ? 32'h00000001 :
                          Bar0ByteEnableWide[2] ? 32'h00000002 :
                          Bar0ByteEnableWide[3] ? 32'h00000003 :
                          Bar0ByteEnableWide[4] ? 32'h00000004 :
                          Bar0ByteEnableWide[5] ? 32'h00000005 :
                          Bar0ByteEnableWide[6] ? 32'h00000006 :
                                                  32'h00000007 ;

// Output address is 64-bit aligned address plus the adjustment
assign Bar0Address      = Bar0AddressAligned + WordAddrAdj;

// Mux the wide write data to the 32-bit output based on low address flag
assign Bar0WriteData    = (LowWordAddress == 1'b1) ? Bar0WriteDataWide[31:0] : Bar0WriteDataWide[63:32];
assign Bar0ByteEnable   = (LowWordAddress == 1'b1) ? Bar0ByteEnableWide[3:0] : Bar0ByteEnableWide[7:4];
assign Bar0ReadDataWide = {Bar0ReadData, Bar0ReadData};

// ---------------------------------------------------------
// Synchronous PHY reset sequence process
// ---------------------------------------------------------

generate
if (DISABLE_INT_RESET_SEQ == 0)
begin

  integer     TxDetectTime;
  integer     Count;

  reg         RxElecIdleReg;
  reg [2:0]   RxStatusReg;
  reg         PhyStatusReg;

  initial
  begin
    Count                = 0;
    TxDetectTime         = 0;
    RxElecIdleReg        = 1'b1;
    RxStatusReg          = 3'b100;
    PhyStatusReg         = 1'b1 ;
  end

  // Generate the PHY reset sequence
  always @(posedge PipeClk)
  begin

    // Keep a cycle count
    Count               <= Count + 1;

    // Capture the count that the TxDetectRx was asserted for timings
    // relative to this
    if ((TxDetectTime == 0) && (TxDetectRx == 1'b1))
    begin
      TxDetectTime      = Count;
    end

    // Generate PhyStatus pulse
    if ((Count == 54) || ((TxDetectTime != 0 && (Count - TxDetectTime) == 5)) || (TxDetectTime != 0 && ((Count - TxDetectTime) == 6)))
    begin
      PhyStatusReg     <= ~PhyStatusReg;
    end

    if (TxDetectTime != 0)
    begin
        // Generate RxStatus and RxElecIdle
        if (((Count - TxDetectTime) == 2) || ((Count - TxDetectTime) ==  6))
        begin
          RxStatusReg      <= 3'b000;
        end
        else if ((Count - TxDetectTime) == 5)
        begin
          RxStatusReg      <= 3'b011;
        end
        else if ((Count - TxDetectTime) == 8)
        begin
          RxStatusReg      <= 3'b100;
        end
        else if ((Count - TxDetectTime) == 25)
        begin
          RxStatusReg      <= "000" ;
          RxElecIdleReg    <= 1'b0 ;
        end
    end
  end
  
  assign PhyStatusInt  = PhyStatusReg;
  assign RxStatusInt   = RxStatusReg;
  assign RxElecIdleInt = RxElecIdleReg;
end  
else
begin

  // When using external PHY reset, just route through the input ports
  assign PhyStatusInt  = PhyStatus;
  assign RxStatusInt   = RxStatus;
  assign RxElecIdleInt = RxElecIdle;

end
endgenerate

  // ---------------------------------------------------------
  // Instantiate the Altera Cyclone V Hard IP for PCI Express
  // simulation model
  // ---------------------------------------------------------

  altpcie_cv_hip_avmm_hwtcl # (

    // Define general specification
    .port_type_hwtcl                                   ("Native endpoint"),    // DEFAULT "Native endpoint"
    .gen123_lane_rate_mode_hwtcl                       ("gen1"),               // DEFAULT "gen1"
    .pll_refclk_freq_hwtcl                             ("125 MHz"),            // DEFAULT "100 MHz"
    .lane_mask_hwtcl                                   ("x1"),                 // default "x4"

    // Set fields in configuration space
    .vendor_id_hwtcl                                   (5372),                 // default 4466
    .device_id_hwtcl                                   (1),                    // default 57345
    .revision_id_hwtcl                                 (1),                    // default 1
    .class_code_hwtcl                                  (163841),               // default 16711680
    .subsystem_vendor_id_hwtcl                         (0),                    // default 4466
    .subsystem_device_id_hwtcl                         (0),                    // default 57345
    .max_payload_size_hwtcl                            (128),                  // default 256
    .bar0_io_space_hwtcl                               ("Disabled"),           // DEFAULT "Disabled"
    .bar0_64bit_mem_space_hwtcl                        ("Disabled"),           // DEFAULT "Disabled"
    .bar0_prefetchable_hwtcl                           ("Disabled"),           // DEFAULT "Disabled"
    .bar0_size_mask_hwtcl                              (12),                   // default "256 MBytes - 28 bits"

    // Define flow control parameters
    .vc0_rx_flow_ctrl_posted_header_hwtcl              (16),                   // default 50
    .vc0_rx_flow_ctrl_posted_data_hwtcl                (16),                   // default 360
    .vc0_rx_flow_ctrl_nonposted_header_hwtcl           (16),                   // default 54
    .vc0_rx_flow_ctrl_nonposted_data_hwtcl             (0),                    // DEFAULT 0
    .vc0_rx_flow_ctrl_compl_header_hwtcl               (0),                    // default 112
    .vc0_rx_flow_ctrl_compl_data_hwtcl                 (0),                    // default 448
    .cpl_spc_header_hwtcl                              (67),                   // default 195
    .cpl_spc_data_hwtcl                                (269),                  // default 781

    // A single receive detector for the 1 lane
    .single_rx_detect_hwtcl                            (1),                    // default 0

    // We don't need a configuration space Avalon access port
    .CG_IMPL_CRA_AV_SLAVE_PORT                         (0),                    // default 1

    // Some required parameters for Avalon to PCIe bridge settings
    .CB_A2P_ADDR_MAP_NUM_ENTRIES                       (2),                    // default 1
    .a2p_pass_thru_bits                                (12)                    // default 24

  ) altpcie_i  (

    // Reset signals
    .pin_perst                                         (nReset),               // input
    .npor                                              (nReset),               // input

    // Clocks
    .refclk                                            (RefClk),               // input
    .coreclkout                                        (coreclkout),           // output

    // PIPE signals for simulation only
    .simu_mode_pipe                                    (1'b1),                 // input
    .sim_pipe_rate                                     (Rate),                 // output [1:0]
    .sim_pipe_pclk_in                                  (PipeClk),              // input
    .sim_ltssmstate                                    (LtssmState),           // output [4:0]

    // Input Pipe interface
    .phystatus0                                        (PhyStatusInt),         // input
    .rxdata0                                           (RxData),               // input  [7:0]
    .rxdatak0                                          (RxDataK),              // input
    .rxelecidle0                                       (RxElecIdleInt),        // input
    .rxstatus0                                         (RxStatusInt),          // input  [2:0]
    .rxvalid0                                          (~RxElecIdleInt),       // input

    // Output Pipe interface
    .eidleinfersel0                                    (EidleInferSel),        // output [2:0]
    .powerdown0                                        (PowerDown),            // output [1:0]
    .rxpolarity0                                       (RxPolarity),           // output
    .txcompl0                                          (TxCompliance),         // output
    .txdata0                                           (TxData),               // output [7:0]
    .txdatak0                                          (TxDataK),              // output
    .txdetectrx0                                       (TxDetectRx),           // output
    .txelecidle0                                       (TxElecIdle),           // output
    .txmargin0                                         (TxMargin),             // output [2:0]
    .txdeemph0                                         (TxDemph),              // output
    .txswing0                                          (TxSwing),              // output

    // Avalon Rx Master BAR0 interface
    .RxmWrite_0_o                                      (Bar0Write),            // output
    .RxmAddress_0_o                                    (Bar0AddressAligned),   // output [AVALON_ADDR_WIDTH-1:0]
    .RxmWriteData_0_o                                  (Bar0WriteDataWide),    // output [avmm_width_hwtcl-1:0]
    .RxmByteEnable_0_o                                 (Bar0ByteEnableWide),   // output [(avmm_width_hwtcl/8)-1:0]
    .RxmBurstCount_0_o                                 (Bar0BurstCount),       // output [avmm_burst_width_hwtcl-1:0]
    .RxmWaitRequest_0_i                                (Bar0WaitRequest),      // input
    .RxmRead_0_o                                       (Bar0Read),             // output
    .RxmReadData_0_i                                   (Bar0ReadDataWide),     // input  [avmm_width_hwtcl-1:0]
    .RxmReadDataValid_0_i                              (Bar0ReadDataValid),    // input

     // Tie off unused Avalon bus inputs else Avalon data transfers corrupted
    .RxmWaitRequest_1_i                                (1'b0),                 // input
    .RxmReadDataValid_1_i                              (1'b0),                 // input
    .RxmWaitRequest_2_i                                (1'b0),                 // input
    .RxmReadDataValid_2_i                              (1'b0),                 // input
    .RxmWaitRequest_3_i                                (1'b0),                 // input
    .RxmReadDataValid_3_i                              (1'b0),                 // input
    .RxmWaitRequest_4_i                                (1'b0),                 // input
    .RxmReadDataValid_4_i                              (1'b0),                 // input
    .RxmWaitRequest_5_i                                (1'b0),                 // input
    .RxmReadDataValid_5_i                              (1'b0),                 // input
    .RxmWaitRequest_6_i                                (1'b0),                 // input
    .RxmReadDataValid_6_i                              (1'b0)                  // input
  );

endmodule