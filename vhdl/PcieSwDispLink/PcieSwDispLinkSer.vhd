-- =============================================================
--
--  Copyright (c) 2026 Simon Southwell. All rights reserved.
--
--  Date: 4th Mar 2026
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

use work.pcieVHost_pkg.all;

-- -------------------------------------------------------------
--  PcieSwDispLinkSer
-- -------------------------------------------------------------

entity PcieSwDispLinkSer is
generic (
  LinkWidth    : integer := 16;
  NodeNum      : integer := 8;
  EndPoint     : integer := 0;
  Gen2Clk      : boolean := false
);
port (
  Clk          : in std_logic;
  SerClk       : in std_logic;
  notReset     : in std_logic;
  LinkIn       : in std_logic_vector (15 downto 0)
);
end entity PcieSwDispLinkSer;

architecture behavioural of PcieSwDispLinkSer is

signal PLinkVec      : link_array_t(0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);

begin

-- -------------------------------------------------------------
--  Instantiate (wide) software display link component
-- -------------------------------------------------------------

  psdl_i : entity work.PcieSwDispLink
  generic map
  (
    LinkWidth         => LinkWidth,
    NodeNum           => NodeNum,
    EP                => EndPoint
  )
  port map
  (
    Clk               => Clk,
    notReset          => notReset,

    LinkIn0           => PLinkVec(0),
    LinkIn1           => PLinkVec(1),
    LinkIn2           => PLinkVec(2),
    LinkIn3           => PLinkVec(3),
    LinkIn4           => PLinkVec(4),
    LinkIn5           => PLinkVec(5),
    LinkIn6           => PLinkVec(6),
    LinkIn7           => PLinkVec(7),
    LinkIn8           => PLinkVec(8),
    LinkIn9           => PLinkVec(9),
    LinkIn10          => PLinkVec(10),
    LinkIn11          => PLinkVec(11),
    LinkIn12          => PLinkVec(12),
    LinkIn13          => PLinkVec(13),
    LinkIn14          => PLinkVec(14),
    LinkIn15          => PLinkVec(15)
  );

 serdes : entity work.Serialiser
 port map
 (
    SerClk            => SerClk,
    BitReverse        => '0',

    ParIn             => (others => (others => '0')),
    SerOut            => open,
    SerIn             => LinkIn,
    ParOut            => PLinkVec
  );

end behavioural;