--=============================================================
--
-- Copyright (c) 2025 Simon Southwell. All rights reserved.
--
-- Date: 23rd July 2025
--
-- This file is part of the pcieVHost package.
--
-- This code is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- The code is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this code. If not, see <http://www.gnu.org/licenses/>.
--
--=============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.env.all;

use work.pcieVHost_pkg.all;
use work.test_pkg.all;

-- -------------------------------------------------------------
--  Top level test bench for usbModel
-- -------------------------------------------------------------

entity test is
generic    (CLK_FREQ_MHZ   : integer   := 250;
            TIMEOUT_US     : integer   := 500;
            GUI_RUN        : integer   := 0
);

end entity ;

architecture behavioural of test is

-- Derive some useful local constants
constant CLK_PERIOD        : time      := 1 us / CLK_FREQ_MHZ;
constant TIMEOUT_COUNT     : integer   := CLK_FREQ_MHZ * TIMEOUT_US;
constant RESET_COUNT       : integer   := 50;

signal   clk               : std_logic := '1';
signal   refclk            : std_logic := '1';
signal   coreclkout        : std_logic;
signal   serclk            : std_logic := '1';
signal   nreset            : std_logic := '0';
signal   count             : integer   := 0;

signal   LinkOutRc         : link_array_t (0 to 0)(LANEWIDTH-1 downto 0);
signal   LinkInRc          : link_array_t (0 to 0)(LANEWIDTH-1 downto 0);

signal   AvAddress         : std_logic_vector (31 downto 0);
signal   AvRead            : std_logic;
signal   AvWaitRequest     : std_logic := '0';
signal   AvWrite           : std_logic;
signal   AvReadDataValid   : std_logic;
signal   AvReadData        : std_logic_vector (31 downto 0);
signal   AvWriteData       : std_logic_vector (31 downto 0);
signal   AvByteEnable      : std_logic_vector ( 3 downto 0);
signal   AvBurstCount      : std_logic_vector ( 6 downto 0);

signal   LtssmState        : std_logic_vector ( 4 downto 0);
signal   EidleInferSel     : std_logic_vector ( 2 downto 0);

begin

-- Generate an active low reset
nreset                     <= '1' when count >= RESET_COUNT else '0';

LinkInRc(0)(9)             <= '0';

-- -----------------------------------------------
-- Clock Generation and timing
-- -----------------------------------------------

  P_CLKGEN : process
  begin
    -- Generate a clock cycle
    loop
      clk                              <= '1';
      wait for CLK_PERIOD/2.0;
      clk                              <= '0';
      wait for CLK_PERIOD/2.0;
    end loop;
  end process;

  P_REFCLKGEN : process
  begin
    -- Generate a clock cycle
    loop
      refclk                              <= '1';
      wait for CLK_PERIOD;
      refclk                              <= '0';
      wait for CLK_PERIOD;
    end loop;
  end process;

  -- -----------------------------------------------
  -- Keep a clock count and monitor for a timeout
  -- -----------------------------------------------
  P_CLKCOUNT : process (clk)
  begin
    if clk'event and clk = '1' then

      count <= count + 1;

      if count >= TIMEOUT_COUNT then
        report "***ERROR: simulation timed out" severity error;

        if GUI_RUN = 1 then
          stop;
        else
          finish;
        end if;
      end if;
    end if;
  end process;

-- -----------------------------------------------
-- Instantiate pcieVHost as a root complex
-- -----------------------------------------------

  rc_i : entity work.pcieVHost
    generic map (
      LinkWidth                        => 1,
      NodeNum                          => 0,
      EndPoint                         => 0
    )
    port map (

      Clk                              => clk,
      notReset                         => nreset,

      LinkOut0                         => LinkOutRc(0),
      LinkIn0                          => LinkInRc(0),
      
      -- Unused inputs
      LinkIn1                          => (others => 'Z'),
      LinkIn2                          => (others => 'Z'),
      LinkIn3                          => (others => 'Z'),
      LinkIn4                          => (others => 'Z'),
      LinkIn5                          => (others => 'Z'),
      LinkIn6                          => (others => 'Z'),
      LinkIn7                          => (others => 'Z'),
      LinkIn8                          => (others => 'Z'),
      LinkIn9                          => (others => 'Z'),
      LinkIn10                         => (others => 'Z'),
      LinkIn11                         => (others => 'Z'),
      LinkIn12                         => (others => 'Z'),
      LinkIn13                         => (others => 'Z'),
      LinkIn14                         => (others => 'Z'),
      LinkIn15                         => (others => 'Z')
    );

-- -----------------------------------------------
-- Instantiate pcie1epavmm as an endpoint
-- -----------------------------------------------

  ep_i : component pcie1epavmm
  generic map (
    DISABLE_INT_RESET_SEQ              => 0
  )
  port map (

    RefClk                             => refclk,
    PipeClk                            => clk,
    nReset                             => nreset,

    coreclkout                         => coreclkout,

    Bar0Address                        => AvAddress,
    Bar0Read                           => AvRead,
    Bar0WaitRequest                    => AvWaitRequest,
    Bar0Write                          => AvWrite,
    Bar0ReadDataValid                  => AvReadDataValid,
    Bar0ReadData                       => AvReadData,
    Bar0WriteData                      => AvWriteData,
    Bar0ByteEnable                     => AvByteEnable,
    Bar0BurstCount                     => AvBurstCount,

    TxData                             => LinkInRc(0)(7 downto 0),
    TxDataK                            => LinkInRc(0)(8),

    TxDetectRx                         => open,
    TxElecIdle                         => open,
    TxCompliance                       => open,
    RxPolarity                         => open,
    PowerDown                          => open,
    Rate                               => open,
    TxDemph                            => open,
    TxMargin                           => open,
    TxSwing                            => open,

    RxData                             => LinkOutRc(0)(7 downto 0),
    RxDataK                            => LinkOutRc(0)(8),
    RxValid                            => 'Z',                       -- driven internally when DISABLE_INT_RESET_SEQ = 0

    RxElecIdle                         => 'Z',                       -- unused when DISABLE_INT_RESET_SEQ = 0
    RxStatus                           => "ZZZ",                     -- unused when DISABLE_INT_RESET_SEQ = 0
    PhyStatus                          => 'Z',                       -- unused when DISABLE_INT_RESET_SEQ = 0

    LtssmState                         => LtssmState,
    EidleInferSel                      => EidleInferSel
  );

  -- -------------------------------------------------------------
  --  Memory model
  -- -------------------------------------------------------------
  mem : entity work.mem_model
  port map
  (
    clk                                => coreclkout,
    rst_n                              => nreset,
                                       
    address                            => AvAddress(31 downto 2) &  "00",
    byteenable                         => AvByteEnable,
    write                              => AvWrite,
    writedata                          => AvWriteData,
    read                               => AvRead,
    readdata                           => AvReadData,
    readdatavalid                      => AvReadDataValid,
                                  
    -- Unused ports                  
    rx_waitrequest                     => open,
    rx_burstcount                      => 12x"000",
    rx_address                         => 32x"00000000",
    rx_read                            => '0',
    rx_readdata                        => open,
    rx_readdatavalid                   => open,
                                     
    tx_waitrequest                     => open,
    tx_burstcount                      => 12x"000",
    tx_address                         => 32x"00000000",
    tx_write                           => '0',
    tx_writedata                       => 32x"00000000",
                                   
    wr_port_valid                      => '0',
    wr_port_data                       => 32x"00000000",
    wr_port_addr                       => 32x"00000000"
  );

end behavioural;