-- =============================================================
--
--  Copyright (c) 2025 Simon Southwell. All rights reserved.
--
--  Date: 26th July 2025
--
--  This file is part of the pcieVHost package.
--
--  The code is free software: you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  This code is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with this code. If not, see <http://www.gnu.org/licenses/>.
--
-- =============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library std;
use std.env.all;

use work.pcieVHost_pkg.all;

entity pcieVHostPipex1 is
generic (
  NodeNum                    : integer := 8;
  EndPoint                   : integer := 1
);
port (
  pclk                       : in  std_logic;
  nreset                     : in  std_logic;

  RxData                     : in  std_logic_vector (PIPEDATAWIDTH-1 downto 0);
  RxDataK                    : in  std_logic;

  TxData                     : out std_logic_vector (PIPEDATAWIDTH-1 downto 0);
  TxDataK                    : out std_logic
);

end entity;

architecture behavioural of pcieVHostPipex1 is

constant LINKWIDTH           : integer := 1;
signal   LinkOut             : std_logic_vector (LANEWIDTH-1 downto 0);

begin

  TxData                     <= LinkOut(PIPEDATAWIDTH-1 downto 0);
  TxDataK                    <= LinkOut(PIPEKBIT);

  -- pcievhost configured with x1 link
  pcievh_i : entity work.PcieVhost
  generic map (
    LinkWidth                => LINKWIDTH,
    NodeNum                  => NodeNum,
    EndPoint                 => EndPoint
  )
  port map (
    Clk                      => pclk,
    notReset                 => nreset,

     -- Link lane input
    LinkIn0                  => '0' & RxDataK & RxData,

    -- Unused inputs
    LinkIn1                  => (others => 'Z'), LinkIn2  => (others => 'Z'), LinkIn3  => (others => 'Z'),
    LinkIn4                  => (others => 'Z'), LinkIn5  => (others => 'Z'), LinkIn6  => (others => 'Z'),
    LinkIn7                  => (others => 'Z'), LinkIn8  => (others => 'Z'), LinkIn9  => (others => 'Z'),
    LinkIn10                 => (others => 'Z'), LinkIn11 => (others => 'Z'), LinkIn12 => (others => 'Z'),
    LinkIn13                 => (others => 'Z'), LinkIn14 => (others => 'Z'), LinkIn15 => (others => 'Z'),

    -- Link lane output
    LinkOut0                 => LinkOut,

    -- Unused outputs
    LinkOut1                 => open, LinkOut2  => open, LinkOut3  => open,
    LinkOut4                 => open, LinkOut5  => open, LinkOut6  => open,
    LinkOut7                 => open, LinkOut8  => open, LinkOut9  => open,
    LinkOut10                => open, LinkOut11 => open, LinkOut12 => open,
    LinkOut13                => open, LinkOut14 => open, LinkOut15 => open

  );

end behavioural;