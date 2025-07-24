-- =============================================================
--
--  Copyright (c) 2025 Simon Southwell. All rights reserved.
--
--  Date: 23rd July 2025
--
--  This code is part of the pcieVHost package.
--
--  pcieVHost is free software: you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  pcieVHost is distributed in the hope that it will be useful,
--  but WITHOUT ANY WARRANTY; without even the implied warranty of
--  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--  GNU General Public License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with pcieVHost. If not, see <http://www.gnu.org/licenses/>.
--
-- =============================================================

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.pcieVHost_pkg.all;

-- -------------------------------------------------------------
--  PcieVhostSerial
-- -------------------------------------------------------------

entity PcieVHostSerial is

  generic (
    LinkWidth             : integer := 16;
    NodeNum               : integer := 8;
    EndPoint              : integer := 0
  );
  port (
    Clk                   : in  std_logic;
    SerClk                : in  std_logic;
    notReset              : in  std_logic;

    LinkIn0               : in  std_logic;
    LinkIn1               : in  std_logic;
    LinkIn2               : in  std_logic;
    LinkIn3               : in  std_logic;
    LinkIn4               : in  std_logic;
    LinkIn5               : in  std_logic;
    LinkIn6               : in  std_logic;
    LinkIn7               : in  std_logic;
    LinkIn8               : in  std_logic;
    LinkIn9               : in  std_logic;
    LinkIn10              : in  std_logic;
    LinkIn11              : in  std_logic;
    LinkIn12              : in  std_logic;
    LinkIn13              : in  std_logic;
    LinkIn14              : in  std_logic;
    LinkIn15              : in  std_logic;

    LinkOut0              : out std_logic;
    LinkOut1              : out std_logic;
    LinkOut2              : out std_logic;
    LinkOut3              : out std_logic;
    LinkOut4              : out std_logic;
    LinkOut5              : out std_logic;
    LinkOut6              : out std_logic;
    LinkOut7              : out std_logic;
    LinkOut8              : out std_logic;
    LinkOut9              : out std_logic;
    LinkOut10             : out std_logic;
    LinkOut11             : out std_logic;
    LinkOut12             : out std_logic;
    LinkOut13             : out std_logic;
    LinkOut14             : out std_logic;
    LinkOut15             : out std_logic
  );

end entity;

architecture behavioural of PcieVHostSerial is

signal PLinkIn            : link_array_t(0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);
signal PLinkOut           : link_array_t(0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);
signal LinkOutVec         : std_logic_vector (MAXLINKWIDTH-1 downto 0);
signal LinkInVec          : std_logic_vector (MAXLINKWIDTH-1 downto 0);

begin

  pvh : entity work.PcieVHost
  generic map (
    LinkWidth                          => LinkWidth,
    NodeNum                            => NodeNum,
    EndPoint                           => EndPoint
  )
  port map (
    Clk                                => Clk,
    notReset                           => notReset,

    LinkIn0                            => PLinkIn(0),
    LinkIn1                            => PLinkIn(1),
    LinkIn2                            => PLinkIn(2),
    LinkIn3                            => PLinkIn(3),
    LinkIn4                            => PLinkIn(4),
    LinkIn5                            => PLinkIn(5),
    LinkIn6                            => PLinkIn(6),
    LinkIn7                            => PLinkIn(7),
    LinkIn8                            => PLinkIn(8),
    LinkIn9                            => PLinkIn(9),
    LinkIn10                           => PLinkIn(10),
    LinkIn11                           => PLinkIn(11),
    LinkIn12                           => PLinkIn(12),
    LinkIn13                           => PLinkIn(13),
    LinkIn14                           => PLinkIn(14),
    LinkIn15                           => PLinkIn(15),

    LinkOut0                           => PLinkOut(0),
    LinkOut1                           => PLinkOut(1),
    LinkOut2                           => PLinkOut(2),
    LinkOut3                           => PLinkOut(3),
    LinkOut4                           => PLinkOut(4),
    LinkOut5                           => PLinkOut(5),
    LinkOut6                           => PLinkOut(6),
    LinkOut7                           => PLinkOut(7),
    LinkOut8                           => PLinkOut(8),
    LinkOut9                           => PLinkOut(9),
    LinkOut10                          => PLinkOut(10),
    LinkOut11                          => PLinkOut(11),
    LinkOut12                          => PLinkOut(12),
    LinkOut13                          => PLinkOut(13),
    LinkOut14                          => PLinkOut(14),
    LinkOut15                          => PLinkOut(15)
  );

  serdes : entity work.Serialiser
  port map (
    SerClk                             => SerClk,
    BitReverse                         => '0',

    ParIn                              => PLinkIn,
    SerOut                             => LinkOutVec,
    SerIn                              => LinkInVec,
    ParOut                             => PLinkOut
  );

LinkOut0                               <= LinkOutVec(0);
LinkOut1                               <= LinkOutVec(1);
LinkOut2                               <= LinkOutVec(2);
LinkOut3                               <= LinkOutVec(3);
LinkOut4                               <= LinkOutVec(4);
LinkOut5                               <= LinkOutVec(5);
LinkOut6                               <= LinkOutVec(6);
LinkOut7                               <= LinkOutVec(7);
LinkOut8                               <= LinkOutVec(8);
LinkOut9                               <= LinkOutVec(9);
LinkOut10                              <= LinkOutVec(10);
LinkOut11                              <= LinkOutVec(11);
LinkOut12                              <= LinkOutVec(12);
LinkOut13                              <= LinkOutVec(13);
LinkOut14                              <= LinkOutVec(14);
LinkOut15                              <= LinkOutVec(15);

LinkInVec(0)                           <= LinkOut0;
LinkInVec(1)                           <= LinkOut1;
LinkInVec(2)                           <= LinkOut2;
LinkInVec(3)                           <= LinkOut3;
LinkInVec(4)                           <= LinkOut4;
LinkInVec(5)                           <= LinkOut5;
LinkInVec(6)                           <= LinkOut6;
LinkInVec(7)                           <= LinkOut7;
LinkInVec(8)                           <= LinkOut8;
LinkInVec(9)                           <= LinkOut9;
LinkInVec(10)                          <= LinkOut10;
LinkInVec(11)                          <= LinkOut11;
LinkInVec(12)                          <= LinkOut12;
LinkInVec(13)                          <= LinkOut13;
LinkInVec(14)                          <= LinkOut14;
LinkInVec(15)                          <= LinkOut15;


end behavioural;