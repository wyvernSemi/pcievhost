###################################################################
# Makefile for Virtual PCIe Host test code
#
# Copyright (c) 2005-2023 Simon Southwell.
#
# Date: 8th Mar 2023
#
# This file is part of pcieVHost.
#
# pcieVHost is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# pcieVHost is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with pcieVHost. If not, see <http://www.gnu.org/licenses/>.
#
###################################################################

NUM_VPROC     = 2
SIM           = RivieraPro

#
# PCIE PcieHost test defintions
#
PCIE_CLIB_DIR = ../lib
SRCDIR        = ../src
USRCDIR       = usercode
WORKDIR       = work

PLI_SO        = VProc.so

USER_C        = VUserMain0.c VUserMain1.c
PCIE_MAP_C    = pcie_vhost_map.h

PCIE_TOP      = test
HDRDIR        = ../sysverilog/headers

VSIMFLAGS     = +access +r -interceptcoutput -sv_lib ${PLI_SO} ${PCIE_TOP}
VLOGFLAGS     = -quiet -sv2k5 -msg 0 +define+DISP_LINK_WIDE -incr +incdir+${VPROC_TOP} -f test.vc

PCIE_MAP_V    = ${PCIE_MAP_C:%.h=${HDRDIR}/%.v}

ifeq ("${SIM}", "ActiveHDL")
  ALDECDIR    =  /c/Aldec/Active-HDL-13-x64
  ARCHFLAGS   = -m64 -I${ALDECDIR}/pli/Include -L${ALDECDIR}/pli/Lib -l:svdpi_exp.lib
else
  ALDECDIR    =  /c/Aldec/Riviera-PRO-2022.10-x64
  ARCHFLAGS   = -m64 -I${ALDECDIR}/interfaces/include -L${ALDECDIR}/interfaces/lib -l:svdpi_exp.lib
endif

#
# Flags for C compiler
#
CC            = gcc
C++           = g++
CFLAGS        = ${ARCHFLAGS}                     \
                -Wno-write-strings               \
                -I${SRCDIR}                      \
                -I${USRCDIR}                     \
                -I${VPROC_TOP}/code              \
                -I.                              \
                -D_REENTRANT

# Get OS type
OSTYPE:=$(shell uname)

# Set OS specific variables between Linux and Windows (MinGW)
ifeq (${OSTYPE}, Linux)
  CFLAGS_SO   = -shared -lpthread -lrt -rdynamic
else
  CFLAGS_SO   = -shared -Wl,-export-all-symbols
endif

SIMLOGFILE    = sim.log

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the verilog and the PLI shared object by default
#
all: ${PLI_SO}

verilog: ${PCIE_MAP_V}
	@if [ ! -d ${WORKDIR} ]; then                \
	  vlib ${WORKDIR};                           \
    fi
	@vlog ${VLOGFLAGS}

#
# Create Verilog pcieVHost VProc register mappings from C header
#
${PCIE_MAP_V} : ${SRCDIR}/${PCIE_MAP_C}
	@sed "s/0x/32\'h/;s/#/\`/;s/DEFINES/AUTO GENERATED\. DO NOT EDIT\!/;" < $< > $@

#
# PLI shared object
#
${PLI_SO}: verilog pcie_c_lib ${USER_C:%.c=${USRCDIR}/%.c}
	@${CC} ${CFLAGS_SO}                          \
	       -Wl,-whole-archive                    \
           ${CFLAGS}                             \
 	       ${USER_C:%.c=${USRCDIR}/%.c}          \
	       -L${PCIE_CLIB_DIR} -lpcievhost        \
	       -L${VPROC_TOP}     -lvproc            \
	       -Wl,-no-whole-archive                 \
	       -o $@

#
# PCIe model library
#
pcie_c_lib:
	@${MAKE} -C ${PCIE_CLIB_DIR}

#
# VProc library.
#
vproc_lib: ${VPROC_TOP}
	@${MAKE} -C ${VPROC_TOP} MAX_NUM_VPROC=${NUM_VPROC} USER_C=""

#
# Checkout VProc from github if it doesn't exist at the specified location
#
${VPROC_TOP}:
	git clone ${VPROC_REPO} ${VPROC_TOP} --recursive

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

run: verilog ${PLI_SO}
	@vsim -c ${VSIMFLAGS} -l ${SIMLOGFILE} -do run.do

rungui: verilog ${PLI_SO}
	@if [ -e wave.do ]; then                                      \
         vsim -gui -l ${SIMLOGFILE} -do wave.do ${VSIMFLAGS};     \
     else                                                         \
         vsim -gui -l ${SIMLOGFILE} ${VSIMFLAGS};                 \
     fi

gui: rungui

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean:
	@${MAKE} -C ${PCIE_CLIB_DIR} clean
	@rm -rf ${PLI_SO} ${PCIE_MAP_V}
	@rm -rf work
	@rm -rf obj
	@rm -rf dataset.asdb
	@rm -rf library.cfg
	@rm -rf sim.log
	@rm -rf dataset*
	@rm -rf compile
	@rm -rf dpi_header.h


