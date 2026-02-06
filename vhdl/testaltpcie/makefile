###################################################################
# Makefile for Virtual PCIe Host test code in Questa
#
# Copyright (c) 2026 Simon Southwell.
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

# Flags for pcie library compilation
EXTFLAGS      = -DVPROC_VHDL -DMODELSIM

# Altera PCIe simulation model definitions
ALTQSYSNAME   = pcie1_ep_avmm.qsys
ALTSRCDIR     = pcie1_ep_avmm
ALTMODELLIB   = pcie_cv_hip_avmm_0
ALTSIMDIR     = $(ALTSRCDIR)/simulation
ALTLIBINFO    = libraries/$(ALTMODELLIB)/_info
ALTCOMPSCRIPT = $(ALTSIMDIR)/mentor/msim_setup.tcl
ALTLIBARGS    = -L work       -L $(ALTMODELLIB)                                               \
                -L altera_ver -L altera_mf_ver -L altera_lnsim_ver  -L lpm_ver                \
                -L sgate_ver  -L cyclonev_ver  -L cyclonev_hssi_ver -L cyclonev_pcie_hip_ver  \
                -L altera     -L altera_mf     -L altera_lnsim      -L lpm                    \
                -L sgate      -L cyclonev      -L cyclonev_hssi                               \
                -suppress 2241 -suppress 2685    -suppress 2718     -suppress 2958            \
                -suppress 3015

# Include main make file rules
include makefile.common

#
# Flags for Questa
#
PCIE_TOP      = test
FILELIST      = files_vhdl.tcl

VCOMFLAGS     = -quiet -2008 -f $(FILELIST) -work $(WORKDIR)
VSIMFLAGS     = $(USRSIMFLAGS) -t 1ps $(SIMELABOPT) -do warnings.do $(PCIE_TOP) $(ALTLIBARGS)

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the vhdl by default
#
all: hdl

.PHONY: hdl, clean, maplib, all

# Analyse HDL files
hdl: $(PLI_SO) $(ALTLIBINFO) maplib
	@if [ ! -d $(WORKDIR) ]; then                                \
	    vlib $(WORKDIR);                                         \
	fi
	@vcom $(VCOMFLAGS)
	@vlog -quiet -sv -l $(ALTMODELLIB) svlog/pcie1epavmm.sv -work $(WORKDIR)

# Generate Altera PCIe simulation source from QSYS file
$(ALTCOMPSCRIPT): ./qsys/$(ALTQSYSNAME)
	@qsys-generate --simulation=VERILOG --output-directory=./$(ALTSRCDIR) ./qsys/$(ALTQSYSNAME)

# Compile the Altera PCIe IP simulation source to the pcie_cv_hip_avmm_0 library
$(ALTLIBINFO): $(ALTCOMPSCRIPT)
	@vsim -c -do "set QSYS_SIMDIR "[pwd]/$(ALTSIMDIR)";          \
	              source $(ALTCOMPSCRIPT);                       \
	              dev_com;                                       \
	              com;                                           \
	              quit"
# Map the Altera PCIe IP library to a logical library name
maplib:
	@vmap $(ALTMODELLIB) $(CURDIR)/libraries/$(ALTMODELLIB)

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

sim: all
	@vsim -c $(VSIMFLAGS)

run: all
	@vsim -c $(VSIMFLAGS) -do "run -all" -do "quit"

rungui: all
	@if [ -e wave.do ]; then                                     \
	   vsim -gui -do wave.do $(VSIMFLAGS) -do "run -all";        \
	else                                                         \
	   vsim -gui $(VSIMFLAGS);                                   \
	fi

gui: rungui

help:
	@echo "make help          Display this message"
	@echo "make               Build C/C++ code without running simulation"
	@echo "make sim           Build and run command line interactive (sim not started)"
	@echo "make run           Build and run batch simulation"
	@echo "make rungui/gui    Build and run GUI simulation"
	@echo "make clean         clean previous build artefacts"
	@echo "make cleanlib      Clean previous $(ALTMODELLIB) library build artefacts"

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean: $(VPROC_TOP)
	@if [ -d "$(WORKDIR)" ]; then                                \
	  vdel -all;                                                 \
	fi
	@$(MAKE) --no-print-directory -C $(PCIE_CLIB_DIR)  clean
	@$(MAKE) --no-print-directory -C $(VPROC_TOP)/test TESTDIR=$(CURDIR) clean
	@rm -rf $(PLI_SO) work transcript *.wlf modelsim.ini $(ALTMODELLIB)


cleanlibs:
	@rm -rf ./libraries $(ALTSIMDIR)/modelsim.ini $(ALTSRCDIR)

