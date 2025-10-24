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
  EndPoint                   : integer := 1;
  DataWidth                  : integer := 8
);
port (
  pcieclk                    : in  std_logic;
  pclk                       : in  std_logic;
  nreset                     : in  std_logic;

  RxData                     : in  std_logic_vector (DataWidth-1 downto 0);
  RxDataK                    : in  std_logic_vector (DataWidth/8-1 downto 0);

  TxData                     : out std_logic_vector (DataWidth-1 downto 0);
  TxDataK                    : out std_logic_vector (DataWidth/8-1 downto 0)
);

end entity;

architecture behavioural of pcieVHostPipex1 is

constant LINKWIDTH           : integer := 1;
signal   LinkOut             : std_logic_vector (LANEWIDTH-1 downto 0);
signal   LinkIn              : std_logic_vector (LANEWIDTH-1 downto 0);
signal   TxSampleClk         : std_logic;

signal   RxDataSampleShift   : std_logic_vector (DataWidth-1   downto 0);
signal   RxDataKSampleShift  : std_logic_vector (DataWidth/8-1 downto 0);
signal   TxDataPipeShift     : std_logic_vector (DataWidth-1   downto 0);
signal   TxDataKPipeShift    : std_logic_vector (DataWidth/8-1 downto 0);

signal   RxDataSample        : std_logic_vector (DataWidth-1   downto 0);
signal   RxDataKSample       : std_logic_vector (DataWidth/8-1 downto 0);
signal   TxDataPipe          : std_logic_vector (DataWidth-1   downto 0);
signal   TxDataKPipe         : std_logic_vector (DataWidth/8-1 downto 0);

signal   rxcount             : integer := 0;

begin

-- If DataWidth is 8 use the pcieclk and make the pclk unused
-- (for backward compatibility)
g_SHIFT : if DataWidth = 8 generate

  TxSampleClk                <= pcieclk;
  RxDataSampleShift          <= RxData;
  RxDataKSampleShift         <= RxDataK;
  TxDataPipeShift            <= LinkOut(7 downto 0);
  TxDataKPipeShift(0)        <= LinkOut(8);

else generate

  TxSampleClk                <= pclk;
  RxDataSampleShift          <= 8x"00" & RxDataSample(DataWidth-1 downto 8);
  RxDataKSampleShift         <= '0' & RxDataKSample(DataWidth/8-1 downto 1);
  TxDataPipeShift            <= LinkOut(7 downto 0) &  TxDataPipe(DataWidth-1 downto 8);
  TxDataKPipeShift           <= LinkOut(8) & TxDataKPipe(DataWidth/8-1 downto 1);

end generate g_SHIFT;

-- Link input is the lower bits of the sampled RX input
LinkIn                       <='0' & RxDataKSample(0) & RxDataSample(7 downto 0);

-- Process PIPE interface signals
process (pcieclk, nreset)
begin
  if nreset = '0' then

    rxcount                  <= 0;

  elsif pcieclk'event and pcieclk = '1' then

    -- Sample the RX input every PIPE width bytes
    if rxcount = 0 then

      RxDataSample           <= RxData;
      RxDataKSample          <= RxDataK;

      -- After sampling, set count to PIPE bytes minus 1
      rxcount                <= (DataWidth/8)-1;

     --  When rxcount non-zero, shift right the sample registers and decrement the count
    else

      RxDataSample           <= RxDataSampleShift;
      RxDataKSample          <= RxDataKSampleShift;

      rxcount                <= rxcount - 1;
    end if;

    -- At each cycle, shift right TX PIPE registers, adding link output to top
    TxDataPipe               <= TxDataPipeShift;
    TxDataKPipe              <= TxDataKPipeShift;
  end if;

end process;

-- At the PIPE clock rate, update TX outputs with TX PIPE data
process (TxSampleClk)
begin
  if TxSampleClk'event and TxSampleClk = '1' then
    TxData                   <= TxDataPipe;
    TxDataK                  <= TxDataKPipe;
  end if;
end process;

  -- pcievhost configured with x1 link
  pcievh_i : entity work.PcieVhost
  generic map (
    LinkWidth                => LINKWIDTH,
    NodeNum                  => NodeNum,
    EndPoint                 => EndPoint
  )
  port map (
    Clk                      => pcieclk,
    notReset                 => nreset,

     -- Link lane input
    LinkIn0                  => LinkIn,

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