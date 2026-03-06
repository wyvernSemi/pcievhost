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
--  PcieSwDispLink
-- -------------------------------------------------------------

entity PcieSwDispLink is
  generic (
    LinkWidth          : integer := 16;
    NodeNum            : integer := 8;
    EP                 : integer := 0;
    DisableScrambling  : integer := 0;
    Disable8b10b       : integer := 0
  );
  port (
    Clk                : in  std_logic;
    notReset           : in  std_logic;

    LinkIn0            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn1            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn2            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn3            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn4            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn5            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn6            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn7            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn8            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn9            : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn10           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn11           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn12           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn13           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn14           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn15           : in  std_logic_vector (LANEWIDTH-1 downto 0)
  );

end entity;

architecture behavioural of PcieSwDispLink is
begin

  pcievhost_i : component PcieVhost
  generic map (
    LinkWidth          => LinkWidth,
    NodeNum            => NodeNum,
    EndPoint           => EP,
    DisableScrambling  => DisableScrambling,
    Disable8b10b       => Disable8b10b,
    Gen2Clk            => false
  )
  port map
  (
    Clk                => Clk,
    notReset           => notReset,

    LinkIn0            => LinkIn0,
    LinkIn1            => LinkIn1,
    LinkIn2            => LinkIn2,
    LinkIn3            => LinkIn3,
    LinkIn4            => LinkIn4,
    LinkIn5            => LinkIn5,
    LinkIn6            => LinkIn6,
    LinkIn7            => LinkIn7,
    LinkIn8            => LinkIn8,
    LinkIn9            => LinkIn9,
    LinkIn10           => LinkIn10,
    LinkIn11           => LinkIn11,
    LinkIn12           => LinkIn12,
    LinkIn13           => LinkIn13,
    LinkIn14           => LinkIn14,
    LinkIn15           => LinkIn15
  );

end behavioural;

