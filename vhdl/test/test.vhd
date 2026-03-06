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

-- -------------------------------------------------------------
--  Top level test bench for pcieVhost
-- -------------------------------------------------------------

entity test is
generic    (CLK_FREQ_MHZ   : integer   := 250; -- 250MHz GEN1, 500MHz GEN2
            TIMEOUT_US     : integer   := 5000;
            GUI_RUN        : integer   := 0;
            DEBUG_STOP     : integer   := 0;
            SERIALISER     : integer   := 0
);

end entity ;

architecture behavioural of test is

-- Derive some useful local constants
constant CLK_PERIOD        : time      := 1 us / CLK_FREQ_MHZ;
constant TIMEOUT_COUNT     : integer   := CLK_FREQ_MHZ * TIMEOUT_US;
constant GEN2_CLK          : boolean   := CLK_FREQ_MHZ = 500;

signal   clk               : std_logic := '1';
signal   serclk            : std_logic := '1';
signal   nreset            : std_logic := '0';
signal   count             : integer   := 0;

signal   LinkOutRc         : link_array_t (0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);
signal   LinkOutEp         : link_array_t (0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);

signal   LinkOutRcSer      : std_logic_vector (MAXLINKWIDTH-1 downto 0);
signal   LinkOutEpSer      : std_logic_vector (MAXLINKWIDTH-1 downto 0);

begin

-- Generate an active low reset
nreset                     <= '1' when count >= 10 else '0';

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

  -- Keep a clock count and monitor for a timeout
  process (clk)
  begin
    if clk'event and clk = '1' then

      if count = 0 and DEBUG_STOP /= 0 then
        report "Simulation stopped for debug attachment. Waiting for restart...";
        stop;
      end if;

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

  g_GENSERIAL : if SERIALISER /= 0 generate

    P_CLKGENSER : process
    begin
      -- Generate a clock cycle
      loop
        serclk                           <= '1';
        wait for CLK_PERIOD/20.0;
        serclk                           <= '0';
        wait for CLK_PERIOD/20.0;
      end loop;
    end process;

  -- -----------------------------------------------
-- Instantiate pcieVHostSerial as a root complex
-- -----------------------------------------------

  rc_i : entity work.pcieVHostSerial
    generic map (
      LinkWidth                        => MAXLINKWIDTH,
      NodeNum                          => 0,
      EndPoint                         => 0,
      Gen2Clk                          => GEN2_CLK
    )
    port map (

      Clk                              => clk,
      SerClk                           => serclk,
      notReset                         => nreset,

      LinkOut0                         => LinkOutRcSer(0),
      LinkOut1                         => LinkOutRcSer(1),
      LinkOut2                         => LinkOutRcSer(2),
      LinkOut3                         => LinkOutRcSer(3),
      LinkOut4                         => LinkOutRcSer(4),
      LinkOut5                         => LinkOutRcSer(5),
      LinkOut6                         => LinkOutRcSer(6),
      LinkOut7                         => LinkOutRcSer(7),
      LinkOut8                         => LinkOutRcSer(8),
      LinkOut9                         => LinkOutRcSer(9),
      LinkOut10                        => LinkOutRcSer(10),
      LinkOut11                        => LinkOutRcSer(11),
      LinkOut12                        => LinkOutRcSer(12),
      LinkOut13                        => LinkOutRcSer(13),
      LinkOut14                        => LinkOutRcSer(14),
      LinkOut15                        => LinkOutRcSer(15),

      LinkIn0                          => LinkOutEpSer(0),
      LinkIn1                          => LinkOutEpSer(1),
      LinkIn2                          => LinkOutEpSer(2),
      LinkIn3                          => LinkOutEpSer(3),
      LinkIn4                          => LinkOutEpSer(4),
      LinkIn5                          => LinkOutEpSer(5),
      LinkIn6                          => LinkOutEpSer(6),
      LinkIn7                          => LinkOutEpSer(7),
      LinkIn8                          => LinkOutEpSer(8),
      LinkIn9                          => LinkOutEpSer(9),
      LinkIn10                         => LinkOutEpSer(10),
      LinkIn11                         => LinkOutEpSer(11),
      LinkIn12                         => LinkOutEpSer(12),
      LinkIn13                         => LinkOutEpSer(13),
      LinkIn14                         => LinkOutEpSer(14),
      LinkIn15                         => LinkOutEpSer(15)
    );

-- -----------------------------------------------
-- Instantiate pcieVHostSerial as an endpoint
-- -----------------------------------------------

  ep_i : entity work.pcieVHostSerial
    generic map (
      LinkWidth                        => MAXLINKWIDTH,
      NodeNum                          => 1,
      EndPoint                         => 1,
      Gen2Clk                          => GEN2_CLK
    )
    port map (

      Clk                              => clk,
      SerClk                           => serclk,
      notReset                         => nreset,

      LinkOut0                         => LinkOutEpSer(0),
      LinkOut1                         => LinkOutEpSer(1),
      LinkOut2                         => LinkOutEpSer(2),
      LinkOut3                         => LinkOutEpSer(3),
      LinkOut4                         => LinkOutEpSer(4),
      LinkOut5                         => LinkOutEpSer(5),
      LinkOut6                         => LinkOutEpSer(6),
      LinkOut7                         => LinkOutEpSer(7),
      LinkOut8                         => LinkOutEpSer(8),
      LinkOut9                         => LinkOutEpSer(9),
      LinkOut10                        => LinkOutEpSer(10),
      LinkOut11                        => LinkOutEpSer(11),
      LinkOut12                        => LinkOutEpSer(12),
      LinkOut13                        => LinkOutEpSer(13),
      LinkOut14                        => LinkOutEpSer(14),
      LinkOut15                        => LinkOutEpSer(15),

      LinkIn0                          => LinkOutRcSer(0),
      LinkIn1                          => LinkOutRcSer(1),
      LinkIn2                          => LinkOutRcSer(2),
      LinkIn3                          => LinkOutRcSer(3),
      LinkIn4                          => LinkOutRcSer(4),
      LinkIn5                          => LinkOutRcSer(5),
      LinkIn6                          => LinkOutRcSer(6),
      LinkIn7                          => LinkOutRcSer(7),
      LinkIn8                          => LinkOutRcSer(8),
      LinkIn9                          => LinkOutRcSer(9),
      LinkIn10                         => LinkOutRcSer(10),
      LinkIn11                         => LinkOutRcSer(11),
      LinkIn12                         => LinkOutRcSer(12),
      LinkIn13                         => LinkOutRcSer(13),
      LinkIn14                         => LinkOutRcSer(14),
      LinkIn15                         => LinkOutRcSer(15)
    );

  else generate
  
  -- -----------------------------------------------
-- Instantiate pcieVHost as a root complex
-- -----------------------------------------------

  rc_i : entity work.pcieVHost
    generic map (
      LinkWidth                        => MAXLINKWIDTH,
      NodeNum                          => 0,
      EndPoint                         => 0,
      Gen2Clk                          => GEN2_CLK
    )
    port map (

      Clk                              => clk,
      notReset                         => nreset,

      LinkOut0                         => LinkOutRc(0),
      LinkOut1                         => LinkOutRc(1),
      LinkOut2                         => LinkOutRc(2),
      LinkOut3                         => LinkOutRc(3),
      LinkOut4                         => LinkOutRc(4),
      LinkOut5                         => LinkOutRc(5),
      LinkOut6                         => LinkOutRc(6),
      LinkOut7                         => LinkOutRc(7),
      LinkOut8                         => LinkOutRc(8),
      LinkOut9                         => LinkOutRc(9),
      LinkOut10                        => LinkOutRc(10),
      LinkOut11                        => LinkOutRc(11),
      LinkOut12                        => LinkOutRc(12),
      LinkOut13                        => LinkOutRc(13),
      LinkOut14                        => LinkOutRc(14),
      LinkOut15                        => LinkOutRc(15),

      LinkIn0                          => LinkOutEp(0),
      LinkIn1                          => LinkOutEp(1),
      LinkIn2                          => LinkOutEp(2),
      LinkIn3                          => LinkOutEp(3),
      LinkIn4                          => LinkOutEp(4),
      LinkIn5                          => LinkOutEp(5),
      LinkIn6                          => LinkOutEp(6),
      LinkIn7                          => LinkOutEp(7),
      LinkIn8                          => LinkOutEp(8),
      LinkIn9                          => LinkOutEp(9),
      LinkIn10                         => LinkOutEp(10),
      LinkIn11                         => LinkOutEp(11),
      LinkIn12                         => LinkOutEp(12),
      LinkIn13                         => LinkOutEp(13),
      LinkIn14                         => LinkOutEp(14),
      LinkIn15                         => LinkOutEp(15)
    );

-- -----------------------------------------------
-- Instantiate pcieVHost as an endpoint
-- -----------------------------------------------

  ep_i : entity work.pcieVHost
    generic map (
      LinkWidth                        => MAXLINKWIDTH,
      NodeNum                          => 1,
      EndPoint                         => 1,
      Gen2Clk                          => GEN2_CLK
    )
    port map (

      Clk                              => clk,
      notReset                         => nreset,

      LinkOut0                         => LinkOutEp(0),
      LinkOut1                         => LinkOutEp(1),
      LinkOut2                         => LinkOutEp(2),
      LinkOut3                         => LinkOutEp(3),
      LinkOut4                         => LinkOutEp(4),
      LinkOut5                         => LinkOutEp(5),
      LinkOut6                         => LinkOutEp(6),
      LinkOut7                         => LinkOutEp(7),
      LinkOut8                         => LinkOutEp(8),
      LinkOut9                         => LinkOutEp(9),
      LinkOut10                        => LinkOutEp(10),
      LinkOut11                        => LinkOutEp(11),
      LinkOut12                        => LinkOutEp(12),
      LinkOut13                        => LinkOutEp(13),
      LinkOut14                        => LinkOutEp(14),
      LinkOut15                        => LinkOutEp(15),

      LinkIn0                          => LinkOutRc(0),
      LinkIn1                          => LinkOutRc(1),
      LinkIn2                          => LinkOutRc(2),
      LinkIn3                          => LinkOutRc(3),
      LinkIn4                          => LinkOutRc(4),
      LinkIn5                          => LinkOutRc(5),
      LinkIn6                          => LinkOutRc(6),
      LinkIn7                          => LinkOutRc(7),
      LinkIn8                          => LinkOutRc(8),
      LinkIn9                          => LinkOutRc(9),
      LinkIn10                         => LinkOutRc(10),
      LinkIn11                         => LinkOutRc(11),
      LinkIn12                         => LinkOutRc(12),
      LinkIn13                         => LinkOutRc(13),
      LinkIn14                         => LinkOutRc(14),
      LinkIn15                         => LinkOutRc(15)
    );
 
 
 end generate;

end behavioural;