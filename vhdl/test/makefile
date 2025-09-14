###################################################################
# Makefile for Virtual PCIe Host test code in Questa
#
# Copyright (c) 2025 Simon Southwell.
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

# User overridable variables
NUM_VPROC     = 2
ARCHFLAG      = -m64
USRSIMFLAGS   =

SERIALISER    =

ifneq ("$(SERIALISER)", "")
 SIMELABOPT   = -GSERIALISER=1
endif

EXTFLAGS      = -DVPROC_VHDL -DMODELSIM -DVPROC_NO_PLI

include makefile.common

#
# Flags for Questa
#
PCIE_TOP      = test
FILELIST      = files_vhdl.tcl

VSIMFLAGS     = $(USRSIMFLAGS) -t 1ps $(SIMELABOPT) -do warnings.do $(PCIE_TOP)
VCOMFLAGS     = -quiet -2008 -f $(FILELIST) -work $(WORKDIR)

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the vhdl by default
#
all: hdl

.PHONY: hdl, pcie_c_lib, vproc_lib, clean, all

# Analyse HDL files
hdl: $(PLI_SO)  
	@if [ ! -d $(WORKDIR) ]; then                          \
	    vlib $(WORKDIR);                                   \
	fi
	@vcom $(VCOMFLAGS)

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

sim: all
	@vsim -c $(VSIMFLAGS)

run: all
	vsim -c $(VSIMFLAGS) -do "run -all" -do "quit"

rungui: all
	@if [ -e wave.do ]; then                               \
	   vsim -gui -do wave.do $(VSIMFLAGS) -do "run -all";  \
	else                                                   \
	   vsim -gui $(VSIMFLAGS);                             \
	fi

gui: rungui

help:
	@echo "make help          Display this message"
	@echo "make               Build C/C++ code without running simulation"
	@echo "make sim           Build and run command line interactive (sim not started)"
	@echo "make run           Build and run batch simulation"
	@echo "make rungui/gui    Build and run GUI simulation"
	@echo "make clean         clean previous build artefacts"

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean: $(VPROC_TOP)
	@if [ -d "$(WORKDIR)" ]; then                          \
	    vdel -all;                                         \
	fi
	@$(MAKE) --no-print-directory -C $(PCIE_CLIB_DIR)  clean
	@$(MAKE) --no-print-directory -C $(VPROC_TOP)/test TESTDIR=$(CURDIR) clean
	@rm -rf $(PLI_SO) work transcript *.wlf


