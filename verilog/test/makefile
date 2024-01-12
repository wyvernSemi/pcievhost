###################################################################
# Makefile for Virtual PCIe Host test code in Modelsim
#
# Copyright (c) 2005-2024 Simon Southwell.
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

# Location of VProc directory. Assumes in same directory as pcieVHost.
# Update if relocated.
VPROC_TOP     = ../../../vproc

# Define the github repository URL for the VProc virtual processor 
VPROC_REPO    = https://github.com/wyvernSemi/vproc.git

#
# PCIE VHost test defintions
#
PCIE_CLIB_DIR = ../../lib
SRCDIR        = ../../src
USRCDIR       = usercode
WORKDIR       = work

PLI_SO        = VProc.so

USER_C        = VUserMain0.c VUserMain1.c
PCIE_MAP_C    = pcie_vhost_map.h

#
# Flags for ModelSim
#
PCIE_TOP      = test
TOP_VC        = test.vc
HDRDIR        = ../headers

VSIMFLAGS     = -pli ${PLI_SO} ${PCIE_TOP}
VLOGFLAGS     = -quiet +define+DISP_LINK_WIDE -incr +incdir+${VPROC_TOP} -f ${TOP_VC}

PCIE_MAP_V    = ${PCIE_MAP_C:%.h=${HDRDIR}/%.v}

# Get OS type
OSTYPE:=$(shell uname)

# If run from a place where MODEL_TECH is not defined, construct from path to PLI library
ifeq ("${MODEL_TECH}", "")
  ifeq (${OSTYPE}, Linux)
    PLILIB    = libmtipli.so
  else
    PLILIB    = mtipli.dll
  endif
  
  VSIMPATH    = $(shell which vsim)
  SIMROOT     = $(shell dirname ${VSIMPATH})/..
  PLILIBPATH  = $(shell find ${SIMROOT} -name "${PLILIB}")
  MODEL_TECH  = $(shell dirname ${PLILIBPATH})
endif

#
# Flags for C compiler
#
CC            = gcc
C++           = g++
ARCHFLAG      = -m32
CFLAGS        = ${ARCHFLAG} -fPIC                           \
                -Wno-write-strings                          \
                -I${SRCDIR}                                 \
                -I${USRCDIR}                                \
                -I${VPROC_TOP}/code                         \
                -I${MODEL_TECH}/../include                  \
                -DMODELSIM                                  \
                -D_REENTRANT

# Set OS specific variables between Linux and Windows (MinGW)
ifeq (${OSTYPE}, Linux)
  CFLAGS_SO   = -shared -lpthread -lrt -rdynamic
  CPPSTD      = -std=c++11
else
  CFLAGS_SO   = -shared -Wl,-export-all-symbols
  CPPSTD      =
endif

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the verilog and the PLI shared object by default
#
all: ${PLI_SO} verilog

#
# Verilog. Phony target, as ModelSim does incremental compile
# and decides what the dependencies are; so always build.
#
.PHONY: verilog

verilog: ${PCIE_MAP_V}
	@if [ ! -d ${WORKDIR} ]; then                           \
	  vlib ${WORKDIR};                                      \
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
${PLI_SO}: vproc_lib pcie_c_lib ${USER_C:%.c=${USRCDIR}/%.c}
	@${CC} ${CFLAGS_SO} ${CFLAGS}                           \
	       -Wl,-whole-archive                               \
 	       ${USER_C:%.c=${USRCDIR}/%.c}                     \
           -lpthread                                        \
           -L${MODEL_TECH}                                  \
           -lmtipli                                         \
	       -L${PCIE_CLIB_DIR} -lpcievhost                   \
	       -L${VPROC_TOP}     -lvproc                       \
	       -Wl,-no-whole-archive                            \
	       -o $@

#
# PCIe model library
#
pcie_c_lib:
	@${MAKE} -C ${PCIE_CLIB_DIR} MODEL_TECH=${MODEL_TECH} ARCHFLAG=${ARCHFLAG} VPROC_TOP=${PWD}/${VPROC_TOP}

#
# VProc library.
#
vproc_lib: ${VPROC_TOP}
	@${MAKE} -C ${VPROC_TOP} MODEL_TECH=${MODEL_TECH} ARCHFLAG=${ARCHFLAG} MAX_NUM_VPROC=${NUM_VPROC} USER_C=""

#
# Checkout VProc from github if it doesn't exist at the specified location
#
${VPROC_TOP}:
	git clone ${VPROC_REPO} ${VPROC_TOP} --recursive

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

sim: vproc_lib pcie_c_lib verilog ${PLI_SO}
	@vsim -c ${VSIMFLAGS}

run: vproc_lib pcie_c_lib verilog ${PLI_SO}
	@vsim -c ${VSIMFLAGS} -do "run -all" -do "quit"

rungui: vproc_lib pcie_c_lib verilog ${PLI_SO}
	@if [ -e wave.do ]; then                                \
         vsim -gui -do wave.do ${VSIMFLAGS} -do "run -all"; \
     else                                                   \
         vsim -gui ${VSIMFLAGS};                            \
     fi

gui: rungui

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean:
	@if [ -d "${WORKDIR}" ]; then                           \
	  vdel -all;                                            \
     fi
	@${MAKE} -C ${PCIE_CLIB_DIR} clean
	@${MAKE} -C ${VPROC_TOP}     clean
	@rm -rf ${PLI_SO} ${PCIE_MAP_V}


