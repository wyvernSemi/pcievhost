-- =============================================================
--
--  Copyright (c) 2025 Simon Southwell. All rights reserved.
--
--  Date: 23rd July 2025
--
--  This code is part of the pcieVHost package.
--
--  This code is free software: you can redistribute it and/or modify
--  it under the terms of the GNU General Public License as published by
--  the Free Software Foundation, either version 3 of the License, or
--  (at your option) any later version.
--
--  The code is distributed in the hope that it will be useful,
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

package pcieVHost_pkg is

  type link_array_t is array (natural range <>) of std_logic_vector;

  constant ELECIDLE        : std_logic_vector (9 downto 0)  := 10b"ZZZZZZZZZZ";
  constant MAXLINKWIDTH    : integer                        := 16;
  constant LANEWIDTH       : integer                        := 10;

  constant LINKADDR0       : std_logic_vector( 31 downto 0) := 32d"0";
  constant LINKADDR1       : std_logic_vector( 31 downto 0) := 32d"1";
  constant LINKADDR2       : std_logic_vector( 31 downto 0) := 32d"2";
  constant LINKADDR3       : std_logic_vector( 31 downto 0) := 32d"3";
  constant LINKADDR4       : std_logic_vector( 31 downto 0) := 32d"4";
  constant LINKADDR5       : std_logic_vector( 31 downto 0) := 32d"5";
  constant LINKADDR6       : std_logic_vector( 31 downto 0) := 32d"6";
  constant LINKADDR7       : std_logic_vector( 31 downto 0) := 32d"7";
  constant LINKADDR8       : std_logic_vector( 31 downto 0) := 32d"8";
  constant LINKADDR9       : std_logic_vector( 31 downto 0) := 32d"9";
  constant LINKADDR10      : std_logic_vector( 31 downto 0) := 32d"10";
  constant LINKADDR11      : std_logic_vector( 31 downto 0) := 32d"11";
  constant LINKADDR12      : std_logic_vector( 31 downto 0) := 32d"12";
  constant LINKADDR13      : std_logic_vector( 31 downto 0) := 32d"13";
  constant LINKADDR14      : std_logic_vector( 31 downto 0) := 32d"14";
  constant LINKADDR15      : std_logic_vector( 31 downto 0) := 32d"15";

  constant NODENUMADDR     : std_logic_vector( 31 downto 0) := 32d"200";
  constant LANESADDR       : std_logic_vector( 31 downto 0) := 32d"201";
  constant PVH_INVERT      : std_logic_vector( 31 downto 0) := 32d"202";
  constant EP_ADDR         : std_logic_vector( 31 downto 0) := 32d"203";
  constant CLK_COUNT       : std_logic_vector( 31 downto 0) := 32d"204";
  constant LINK_STATE      : std_logic_vector( 31 downto 0) := 32d"205";
  constant RESET_STATE     : std_logic_vector( 31 downto 0) := 32d"206";

  constant PVH_STOP        : std_logic_vector( 31 downto 0) := 32x"fffffffd";
  constant PVH_FINISH      : std_logic_vector( 31 downto 0) := 32x"fffffffe";
  constant PVH_FATAL       : std_logic_vector( 31 downto 0) := 32x"ffffffff";
  
  -- 10 bit COMMA codes
  constant PCOMMA          : std_logic_vector (LANEWIDTH-1 downto 0) := 10b"1010000011";   -- 0x283
  constant NCOMMA          : std_logic_vector (LANEWIDTH-1 downto 0) := 10b"0101111100";   -- 0x17c

  function has_an_x (vec   : std_logic_vector) return boolean;
  function BitRev10 (InVal : in std_logic_vector(9 downto 0)) return std_logic_vector;

end;

package body pcievhost_pkg is

  function has_an_x (vec : std_logic_vector) return boolean is
  begin

    for idx in vec'range loop
      case vec(idx) is
        when 'U' | 'X' | 'Z' | 'W' | '-' => return true;
        when others                      => null;
      end case;
    end loop;

    return false;

  end function has_an_x;


  function BitRev10 (InVal : in std_logic_vector(9 downto 0)) return std_logic_vector is
  begin
      return InVal(0) & InVal(1) & InVal(2) & InVal(3) & InVal(4) & InVal(5) & InVal(6) & InVal(7) & InVal(8) & InVal(9);
  end function BitRev10;

end;