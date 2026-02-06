-- =============================================================
--
--  Copyright (c) 2026 Simon Southwell. All rights reserved.
--
--  Date: 3rd February 2026
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

package test_pkg is

  -- Component declaration for the SystemVerilog wrapper for the Altera PCIe IP model
  component pcie1epavmm is
  generic (
    DISABLE_INT_RESET_SEQ : integer := 0
  );
  port (
    RefClk               : in    std_logic;
    PipeClk              : in    std_logic;
    nReset               : in    std_logic;

    coreclkout           : out   std_logic;

    Bar0Address          : out   std_logic_vector (31 downto 0);
    Bar0Read             : out   std_logic ;
    Bar0WaitRequest      : in    std_logic ;
    Bar0Write            : out   std_logic ;
    Bar0ReadDataValid    : in    std_logic ;
    Bar0ReadData         : in    std_logic_vector (31 downto 0);
    Bar0WriteData        : out   std_logic_vector (31 downto 0);
    Bar0ByteEnable       : out   std_logic_vector  (3 downto 0);
    Bar0BurstCount       : out   std_logic_vector  (6 downto 0);

    TxData               : out   std_logic_vector  (7 downto 0);
    TxDataK              : out   std_logic ;

    TxDetectRx           : out   std_logic ;
    TxElecIdle           : out   std_logic ;
    TxCompliance         : out   std_logic ;
    RxPolarity           : out   std_logic ;
    PowerDown            : out   std_logic_vector  (1 downto 0);
    Rate                 : out   std_logic_vector  (1 downto 0);
    TxDemph              : out   std_logic ;
    TxMargin             : out   std_logic_vector  (2 downto 0);
    TxSwing              : out   std_logic ;

    RxData               : in    std_logic_vector  (7 downto 0);
    RxDataK              : in    std_logic ;
    RxValid              : in    std_logic ;

    RxElecIdle           : in    std_logic ;
    RxStatus             : in    std_logic_vector  (2 downto 0);
    PhyStatus            : in    std_logic ;

    LtssmState           : out   std_logic_vector  (4 downto 0);
    EidleInferSel        : out   std_logic_vector  (2 downto 0)

   ) ;
  end component pcie1epavmm ;

end package test_pkg;