-- =============================================================
--
--  Copyright (c) 2025 Simon Southwell. All rights reserved.
--
--  Date: 23rd July 2025
--
--  This file is part of the pcieVHost package.
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

library std;
use std.env.all;

use work.pcieVHost_pkg.all;

entity Serialiser is
  generic (
    Width                              : integer := MAXLINKWIDTH
  );
  port (
    SerClk                             : in  std_logic;
    BitReverse                         : in  std_logic := '0';

    ParIn                              : in  link_array_t(0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0) ;
    SerOut                             : out std_logic_vector (MAXLINKWIDTH-1 downto 0);
    SerIn                              : in  std_logic_vector (MAXLINKWIDTH-1 downto 0);
    ParOut                             : out link_array_t(0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0)
  );
end entity;

architecture behavioural of Serialiser is

signal SerialShift                     : link_array_t(0 to Width-1)(LANEWIDTH-1 downto 0);
signal DeserialShift                   : link_array_t(0 to Width-1)(LANEWIDTH-1 downto 0);
signal DeserialReg                     : link_array_t(0 to Width-1)(LANEWIDTH-1 downto 0);

signal Synced                          : std_logic := '0';

signal SerialCount                     : integer   := 0;
signal DeserialCount                   : integer   := 0;

signal ParInInt                        : link_array_t(0 to Width-1)(LANEWIDTH-1 downto 0);

begin


  g_GENDATA:  for i in 0 to Width-1 generate
    ParInInt (i)        <= BitRev10(ParIn(i)) when BitReverse = '1' else ParIn(i);
    ParOut(i)           <= (others => 'Z') when Synced = '0' else BitRev10(DeserialReg(i)) when BitReverse = '1' else  DeserialReg(i);
    SerOut(i)           <= SerialShift(i)(0);
  end generate g_GENDATA;

  process (SerClk)
  begin
    if SerClk'event and SerClk = '1' then

      if SerialCount = 0 then
        SerialCount <= LANEWIDTH-1;
        for idx in 0 to Width-1 loop
          SerialShift(idx) <= ParIn(idx);
        end loop;
      else
        SerialCount <=  SerialCount - 1;
        for idx in 0 to Width-1 loop
          SerialShift(idx) <= '0' &  SerialShift(idx)(LANEWIDTH-1 downto 1);
        end loop;
      end if;

      for idx in 0 to Width-1 loop
        DeserialShift(idx) <= SerIn(idx) & DeserialShift(idx)(LANEWIDTH-1 downto 1);
      end loop;

      -- Maintain sync
      if Synced = '1' then
        if SerIn(0) = 'Z' or SerIn(0) = 'X' then
          Synced <= '0';
        end if;
      else
        if DeserialShift(0) = NCOMMA or DeserialShift(0) = PCOMMA then
          Synced <= '1';
        end if;
      end if;

      if Synced = '1' then
        DeserialCount <=  (DeserialCount + 1) mod LANEWIDTH;
      end if;

      if DeserialCount = (LANEWIDTH-1) or (Synced = '0' and (DeserialShift(0) = NCOMMA or DeserialShift(0) = PCOMMA)) then
        for idx in 0 to Width-1 loop
          DeserialReg(idx) <= DeserialShift(idx);
        end loop;
      end if;
    end if;

  end process;

end behavioural;