-- =============================================================
--
--  Copyright (c) 2026 Simon Southwell. All rights reserved.
--
--  Date: 28th Feb 2026
--
--  This file is part of the pcieVHost package.
--
--  This file is free software: you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  The file is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this file. If not, see <http://www.gnu.org/licenses/>.
--
-- =============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.env.all;

entity clkmux is
port (
  aresetn           : in  std_logic;
  sel               : in  std_logic;
  clka              : in  std_logic;
  clkb              : in  std_logic;
  clkout            : out std_logic
);
end entity;

architecture rtl of clkmux is

signal clkagated    : std_logic;
signal clkbgated    : std_logic;

signal selclka      : std_logic := '0';
signal selclkb      : std_logic := '0';
signal selclka_p1   : std_logic := '0';
signal selclkb_p1   : std_logic := '0';

begin

-- -------------------------------------------------------------
-- Combinatorial Logic
-- -------------------------------------------------------------

  -- clks gated on synchronised selects
  clkagated <= clka and selclka;
  clkbgated <= clkb and selclkb;

  -- clkout, combined from selected clocks
  clkout    <= clkagated or clkbgated;

-- -------------------------------------------------------------
-- Synchronous Processes
-- -------------------------------------------------------------

  -- Phase 1 stage for clka select
  process (clka, aresetn)
  begin
    if aresetn = '0' then
      selclka_p1 <= '0';
    elsif clka'event and clka = '1' then
      selclka_p1 <=  sel and not selclkb;
    end if;
  end process;

  -- Output stage for clka select
  process (clka, aresetn)
  begin
      if aresetn = '0' then
        selclka  <= '0';
      elsif clka'event and clka = '0' then
        selclka  <= selclka_p1;
      end if;
  end process;

  -- Phase 1 stage for clkb select
  process (clkb, aresetn)
  begin
    if aresetn = '0' then
      selclkb_p1 <= '0';
    elsif clkb'event and clkb = '1' then
      selclkb_p1 <= not sel and not selclka;
    end if;
  end process;

  -- Output stage for clkb select
  process (clkb, aresetn)
  begin
    if aresetn = '0' then
      selclkb    <= '0';
    elsif clkb'event and clkb = '0' then
      selclkb    <= selclkb_p1;
    end if;
  end process;

end rtl;

-- ==============================================================
-- Local test
-- ==============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;
use std.env.all;

entity testclkmux is
generic (
  EN_TEST             : boolean := false;
  CLKA_PERIOD         : time    := 5000 ps;
  CLKB_PERIOD         : time    := 2000 ps
);
end entity testclkmux;

architecture behavioural of testclkmux is

signal clkout         : std_logic ;

signal aresetn        : std_logic := '0';
signal clka           : std_logic := '0';
signal clkb           : std_logic := '0';
signal sel            : std_logic := '0';

begin

-- -------------------------------------------------------------
-- Clock generation
-- -------------------------------------------------------------

  P_GENTEST : if EN_TEST generate
  begin
    P_CLKAGEN : process
    begin
      -- Generate a clock cycle
      loop
        clka                             <= '1';
        wait for CLKA_PERIOD/2;
        clka                             <= '0';
        wait for CLKA_PERIOD/2;
      end loop;
    end process;

    P_CLKBGEN : process
    begin
      -- Generate a clock cycle
      loop
        clkb                             <= '1';
        wait for CLKB_PERIOD/2;
        clkb                             <= '0';
        wait for CLKB_PERIOD/2;
      end loop;
    end process;

-- -------------------------------------------------------------
-- Clock Multiplexer Instantiation
-- -------------------------------------------------------------
    clkmux_i : entity work.clkmux
    port map (
      aresetn  => aresetn,
      sel      => sel,
      clka     => clka,
      clkb     => clkb,
      clkout   => clkout
    );

-- -------------------------------------------------------------
-- Test Sequence
-- -------------------------------------------------------------
    P_SEQ : process
    begin
      aresetn        <='0';
      wait for  5000 ps;
      aresetn <='1';
      wait for 11000 ps;
      sel     <='1';
      wait for 27000 ps;
      sel     <='0';
      wait for 43000 ps;
      sel     <='1';
      wait for 17000 ps;
      sel     <='0';
      wait for  1000 ps;
      stop;
    end process;

  end generate;

end behavioural;
