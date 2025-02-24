REM ##################################################################
REM  Compile script for DPI only Virtual PCIe Host test code in
REM  Vivado XSIM
REM
REM  Copyright (c) 2025 Simon Southwell.
REM
REM  This file is part of pcieVHost.
REM
REM  pcieVHost is free software: you can redistribute it and/or modify
REM  it under the terms of the GNU General Public License as published by
REM  the Free Software Foundation, either version 3 of the License, or
REM  (at your option) any later version.
REM
REM  pcieVHost is distributed in the hope that it will be useful,
REM  but WITHOUT ANY WARRANTY; without even the implied warranty of
REM  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
REM  GNU General Public License for more details.
REM
REM  You should have received a copy of the GNU General Public License
REM  along with pcieVHost. If not, see <http://www.gnu.org/licenses/>.
REM
REM ##################################################################

@echo off
cls

REM compile the DPI C++ code, elaborate the Verilog/SystemVerilog
REM and run the simulation in batch mode

xsc --gcc_compile_options -Wno-attributes                                            ^
    --gcc_compile_options -DVPROC_SV                                                 ^
    --gcc_compile_options -DPCIEDPI                                                  ^
    --gcc_compile_options -DVIVADO                                                   ^
    --gcc_compile_options -DLTSSM_ABBREVIATED                                        ^
    --gcc_compile_options -Iusercode                                                 ^
    --gcc_compile_options -I..\..\src                                                ^
    ..\..\src\codec.c                                                                ^
    ..\..\src\ltssm.c                                                                ^
    ..\..\src\mem.c                                                                  ^
    ..\..\src\pcicrc32.c                                                             ^
    ..\..\src\pcie.c                                                                 ^
    ..\..\src\pcie_dpi.c                                                             ^
    ..\..\src\pcie_utils.c                                                           ^
    .\usercode\VUserMain0.cpp                                                        ^
    .\usercode\VUserMain1.c                                                          ^
&& xvlog --prj files.prj -d DISP_LINK_WIDE -d VPROC_SV -d VIVADO -i ..\headers -i .\ ^
&& xelab   --debug typical test                                                      ^
&& xsim    -R test
