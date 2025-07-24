-- =============================================================
--
--  Copyright (c) 2025 Simon Southwell. All rights reserved.
--
--  Date: 22nd July 2025
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

library std;
use std.env.all;

use work.pcieVHost_pkg.all;

-- -------------------------------------------------------------
--  PcieVhost
-- -------------------------------------------------------------

entity PcieVHost is
  generic (
    LinkWidth                         : integer := 0;
    NodeNum                           : integer := 8;
    EndPoint                          : integer := 0
  );

  port (
    Clk                               : in  std_logic;
    notReset                          : in  std_logic;

    LinkIn0                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn1                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn2                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn3                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn4                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn5                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn6                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn7                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn8                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn9                           : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn10                          : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn11                          : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn12                          : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn13                          : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn14                          : in  std_logic_vector (LANEWIDTH-1 downto 0);
    LinkIn15                          : in  std_logic_vector (LANEWIDTH-1 downto 0);

    LinkOut0                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut1                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut2                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut3                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut4                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut5                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut6                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut7                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut8                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut9                          : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut10                         : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut11                         : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut12                         : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut13                         : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut14                         : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z');
    LinkOut15                         : out std_logic_vector (LANEWIDTH-1 downto 0) := (others => 'Z')

  );
end entity;

architecture behavioural of PcieVHost is

signal notResetLast                    : std_logic := '0';

signal InvertIn                        : std_logic := '0';
signal InvertOut                       : std_logic := '0';
signal InvertOutVec                    : std_logic_vector(LANEWIDTH-1 downto 0)  := (others => '0');
signal InvertInVec                     : std_logic_vector(LANEWIDTH-1 downto 0)  := (others => '0');
signal ReverseIn                       : std_logic := '0';
signal ReverseOut                      : std_logic := '0';
signal ElecIdleIn                      : std_logic_vector(MAXLINKWIDTH-1 downto 0);
signal ElecIdleOut                     : std_logic_vector(MAXLINKWIDTH-1 downto 0) := (others => '1');
signal RxDetect                        : std_logic_vector(MAXLINKWIDTH-1 downto 0);

signal Addr                            : std_logic_vector(31 downto 0);
signal DataOut                         : std_logic_vector(31 downto 0);
signal WE                              : std_logic;
signal RD                              : std_logic;
signal Update                          : std_logic;
signal UpdateResponse                  : std_logic                                 := '1';
signal DataIn                          : std_logic_vector(31 downto 0)             := (others => '0');
signal Interrupt                       : std_logic_vector( 2 downto 0)             := (others => '0');

signal ClkCount                        : integer                                   := 0;

signal LinkInVec                       : link_array_t (0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);
signal LinkOutVec                      : link_array_t (0 to MAXLINKWIDTH-1)(LANEWIDTH-1 downto 0);

begin

  -----------------------------------------
  -- Synchronous process
  -----------------------------------------

  process(Clk)
  begin
    if Clk'event and Clk = '1' then
      ClkCount                         <= ClkCount + 1;
      notResetLast                     <= notReset;
    end if;
  end process;

  -----------------------------------------
  -- VProc instantiation
  -----------------------------------------

  vproc_inst : entity work.VProc
  port map (
    Clk                                => Clk,
    Addr                               => Addr,
    WE                                 => WE,
    RD                                 => RD,
    DataOut                            => DataOut,
    DataIn                             => DataIn,
    WRAck                              => '1',
    RDAck                              => '1',
    Interrupt                          => Interrupt,
    Update                             => update,
    UpdateResponse                     => updateResponse,
    Node                               => std_logic_vector(to_unsigned(NodeNum, 4))
  );

  -----------------------------------------
  -- Delta-cycle update process
  -----------------------------------------

  process (Update)
  begin
    if Update'event then
      DataIn <= (others => '0');

      if WE = '1' or RD = '1' then

        case Addr is

          when NODENUMADDR => DataIn <= std_logic_vector(to_unsigned(NodeNum,   32));
          when LANESADDR   => DataIn <= std_logic_vector(to_unsigned(LinkWidth, 32));
          when EP_ADDR     => DataIn <= std_logic_vector(to_unsigned(EndPoint,  32));
          when CLK_COUNT   => DataIn <= std_logic_vector(to_unsigned(ClkCount,  32));
          when RESET_STATE => DataIn <= 31x"00000000" & not notReset;

          when LINKADDR0  | LINKADDR1  | LINKADDR2  | LINKADDR3  |
               LINKADDR4  | LINKADDR5  | LINKADDR6  | LINKADDR7  |
               LINKADDR8  | LINKADDR9  | LINKADDR10 | LINKADDR11 |
               LINKADDR12 | LINKADDR13 | LINKADDR14 | LINKADDR15 =>

              if WE = '1' then
                  LinkOutVec(to_integer(unsigned(Addr(3 downto 0)))) <= DataOut(LANEWIDTH-1 downto 0) xor InvertOutVec;
              end if;

              DataIn <= 22x"000000" & (LinkInVec(to_integer(unsigned(Addr(3 downto 0)))) xor InvertInVec);

          when LINK_STATE  =>
              if WE = '1' then
                ElecIdleOut <= DataOut(MAXLINKWIDTH-1 downto 0);
              end if;
              DataIn <= RxDetect & ElecIdleIn;

          when PVH_INVERT =>
              if WE = '1' then
                ReverseOut   <= DataOut(3) ;
                ReverseIn    <= DataOut(2) ;
                InvertOut    <= DataOut(1) ;
                InvertIn     <= DataOut(0) ;
              end if;

              DataIn <= 28x"0000000" & ReverseOut & ReverseIn & InvertOut & InvertIn;

          when PVH_STOP   => if WE = '1' then stop; end if;
          when PVH_FINISH => if WE = '1' then finish; end if;
          when PVH_FATAL  => if WE = '1' then report "Fatal issued by VProc" severity error; end if;
          when others     => report "***Error. PcieVHost---access to invalid address from VProc" severity error;

        end case;
      end if;

      -- Finished processing, so flag to VProc
      UpdateResponse <= not UpdateResponse;

    end if;
  end process;

  -----------------------------------------
  -- Combinatorial logic
  -----------------------------------------

  Interrupt     <= (notReset and not notResetLast) & 2b"00";

  ElecIdleIn(0)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(1)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(2)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(3)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(4)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(5)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(6)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(7)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(8)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(9)  <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(10) <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(11) <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(12) <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(13) <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(14) <= '1' when LinkIn0 = ELECIDLE  else '0';
  ElecIdleIn(15) <= '1' when LinkIn0 = ELECIDLE  else '0';

  RxDetect(0)    <= '1' when has_an_x(LinkOut0)  else '0';
  RxDetect(1)    <= '1' when has_an_x(LinkOut1)  else '0';
  RxDetect(2)    <= '1' when has_an_x(LinkOut2)  else '0';
  RxDetect(3)    <= '1' when has_an_x(LinkOut3)  else '0';
  RxDetect(4)    <= '1' when has_an_x(LinkOut4)  else '0';
  RxDetect(5)    <= '1' when has_an_x(LinkOut5)  else '0';
  RxDetect(6)    <= '1' when has_an_x(LinkOut6)  else '0';
  RxDetect(7)    <= '1' when has_an_x(LinkOut7)  else '0';
  RxDetect(8)    <= '1' when has_an_x(LinkOut8)  else '0';
  RxDetect(9)    <= '1' when has_an_x(LinkOut9)  else '0';
  RxDetect(10)   <= '1' when has_an_x(LinkOut10) else '0';
  RxDetect(11)   <= '1' when has_an_x(LinkOut11) else '0';
  RxDetect(12)   <= '1' when has_an_x(LinkOut12) else '0';
  RxDetect(13)   <= '1' when has_an_x(LinkOut13) else '0';
  RxDetect(14)   <= '1' when has_an_x(LinkOut14) else '0';
  RxDetect(15)   <= '1' when has_an_x(LinkOut15) else '0';

  InvertInVec    <= (others => InvertIn);

  LinkInVec(0)   <= LinkIn15 when ReverseIn = '1' else LinkIn0  after 1 ps;
  LinkInVec(1)   <= LinkIn14 when ReverseIn = '1' else LinkIn1  after 1 ps;
  LinkInVec(2)   <= LinkIn13 when ReverseIn = '1' else LinkIn2  after 1 ps;
  LinkInVec(3)   <= LinkIn12 when ReverseIn = '1' else LinkIn3  after 1 ps;
  LinkInVec(4)   <= LinkIn11 when ReverseIn = '1' else LinkIn4  after 1 ps;
  LinkInVec(5)   <= LinkIn10 when ReverseIn = '1' else LinkIn5  after 1 ps;
  LinkInVec(6)   <= LinkIn9  when ReverseIn = '1' else LinkIn6  after 1 ps;
  LinkInVec(7)   <= LinkIn8  when ReverseIn = '1' else LinkIn7  after 1 ps;
  LinkInVec(8)   <= LinkIn7  when ReverseIn = '1' else LinkIn8  after 1 ps;
  LinkInVec(9)   <= LinkIn6  when ReverseIn = '1' else LinkIn9  after 1 ps;
  LinkInVec(10)  <= LinkIn5  when ReverseIn = '1' else LinkIn10 after 1 ps;
  LinkInVec(11)  <= LinkIn4  when ReverseIn = '1' else LinkIn11 after 1 ps;
  LinkInVec(12)  <= LinkIn3  when ReverseIn = '1' else LinkIn12 after 1 ps;
  LinkInVec(13)  <= LinkIn2  when ReverseIn = '1' else LinkIn13 after 1 ps;
  LinkInVec(14)  <= LinkIn1  when ReverseIn = '1' else LinkIn14 after 1 ps;
  LinkInVec(15)  <= LinkIn0  when ReverseIn = '1' else LinkIn15 after 1 ps;

  InvertOutVec   <= (others => InvertOut);

  LinkOut0       <= ELECIDLE when ElecIdleOut(0)  = '1' else LinkOutVec(15) when ReverseOut else LinkOutVec( 0);
  LinkOut1       <= ELECIDLE when ElecIdleOut(1)  = '1' else LinkOutVec(14) when ReverseOut else LinkOutVec( 1);
  LinkOut2       <= ELECIDLE when ElecIdleOut(2)  = '1' else LinkOutVec(13) when ReverseOut else LinkOutVec( 2);
  LinkOut3       <= ELECIDLE when ElecIdleOut(3)  = '1' else LinkOutVec(12) when ReverseOut else LinkOutVec( 3);
  LinkOut4       <= ELECIDLE when ElecIdleOut(4)  = '1' else LinkOutVec(11) when ReverseOut else LinkOutVec( 4);
  LinkOut5       <= ELECIDLE when ElecIdleOut(5)  = '1' else LinkOutVec(10) when ReverseOut else LinkOutVec( 5);
  LinkOut6       <= ELECIDLE when ElecIdleOut(6)  = '1' else LinkOutVec(9)  when ReverseOut else LinkOutVec( 6);
  LinkOut7       <= ELECIDLE when ElecIdleOut(7)  = '1' else LinkOutVec(8)  when ReverseOut else LinkOutVec( 7);
  LinkOut8       <= ELECIDLE when ElecIdleOut(8)  = '1' else LinkOutVec(7)  when ReverseOut else LinkOutVec( 8);
  LinkOut9       <= ELECIDLE when ElecIdleOut(9)  = '1' else LinkOutVec(6)  when ReverseOut else LinkOutVec( 9);
  LinkOut10      <= ELECIDLE when ElecIdleOut(10) = '1' else LinkOutVec(5)  when ReverseOut else LinkOutVec(10);
  LinkOut11      <= ELECIDLE when ElecIdleOut(11) = '1' else LinkOutVec(4)  when ReverseOut else LinkOutVec(11);
  LinkOut12      <= ELECIDLE when ElecIdleOut(12) = '1' else LinkOutVec(3)  when ReverseOut else LinkOutVec(12);
  LinkOut13      <= ELECIDLE when ElecIdleOut(13) = '1' else LinkOutVec(2)  when ReverseOut else LinkOutVec(13);
  LinkOut14      <= ELECIDLE when ElecIdleOut(14) = '1' else LinkOutVec(1)  when ReverseOut else LinkOutVec(14);
  LinkOut15      <= ELECIDLE when ElecIdleOut(15) = '1' else LinkOutVec(0)  when ReverseOut else LinkOutVec(15);

end behavioural;
