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
--  Top level test bench for usbModel
-- -------------------------------------------------------------

entity test is
generic    (CLK_FREQ_MHZ   : integer   := 500;
            TIMEOUT_US     : integer   := 5000;
            GUI_RUN        : integer   := 0;
            DEBUG_STOP     : integer   := 0
);

end entity ;

architecture behavioural of test is

-- Derive some useful local constants
constant CLK_PERIOD        : time      := 1 us / CLK_FREQ_MHZ;
constant TIMEOUT_COUNT     : integer   := CLK_FREQ_MHZ * TIMEOUT_US;

signal   clk               : std_logic := '1';
signal   serclk            : std_logic := '1';
signal   nreset            : std_logic := '0';
signal   count             : integer   := 0;

signal   UpData            : std_logic_vector (PIPEDATAWIDTH-1 downto 0);
signal   UpDataK           : std_logic;
signal   DownData          : std_logic_vector (PIPEDATAWIDTH-1 downto 0);
signal   DownDataK         : std_logic;

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

-- -----------------------------------------------
-- Instantiate pcieVHost as a root complex
-- -----------------------------------------------

  rc_i : entity work.pcieVHostPipex1
    generic map (
      NodeNum                          => 0,
      EndPoint                         => 0
    )
    port map (

      pclk                             => clk,
      nreset                           => nreset,

      TxData                           => DownData,
      TxDataK                          => DownDataK,

      RxData                           => UpData,
      RxDataK                          => UpDataK
    );

-- -----------------------------------------------
-- Instantiate pcieVHost as an endpoint
-- -----------------------------------------------

  ep_i : entity work.pcieVHostPipex1
    generic map (
      NodeNum                          => 1,
      EndPoint                         => 1
    )
    port map (

      pclk                             => clk,
      nreset                           => nreset,

      TxData                           => UpData,
      TxDataK                          => UpDataK,

      RxData                           => DownData,
      RxDataK                          => DownDataK
    );



end behavioural;