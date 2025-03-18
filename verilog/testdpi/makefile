###################################################################
# Makefile for Virtual PCIe Host test code in Vivado Xsim
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

USRSIMFLAGS   =

#
# PCIE VHost test defintions
#
SRCDIR        = ../../src
USRCDIR       = usercode

USER_C        = VUserMain0.cpp VUserMain1.c
PCIE_MAP_C    = pcie_vhost_map.h
PCIE_C        = codec.c                       \
                ltssm.c                       \
                mem.c                         \
                pcicrc32.c                    \
                pcie.c                        \
                pcie_dpi.c                    \
                pcie_utils.c



# Separate C and C++ source files
USER_CPP_BASE = $(notdir $(filter %cpp, $(USER_C)))
USER_C_BASE   = $(notdir $(filter %c,   $(USER_C)))
ALLSRC        = $(PCIE_C:%.c=$(SRCDIR)/%.c) $(USER_C_BASE:%.c=$(USRCDIR)/%.c) $(USER_CPP_BASE:%.cpp=$(USRCDIR)/%.cpp)

CFLAGS        = -Wno-attributes               \
                -DVPROC_SV                    \
                -DPCIEDPI                     \
                -DVIVADO                      \
                -DLTSSM_ABBREVIATED           \
                -I$(USRCDIR)                  \
                -I$(SRCDIR)

#------------------------------------------------------
# Flags for simulator
#------------------------------------------------------

PRJFILE       = files.prj
HDRDIR        = ../headers

PCIE_MAP_V    = $(PCIE_MAP_C:%.h=$(HDRDIR)/%.v)

SIMTOP        = test

# Flags for xsim
ANALYSEFLAGS  = --prj $(PRJFILE) -d DISP_LINK_WIDE -d VPROC_SV -d VIVADO -i $(HDRDIR)
ELABFLAGS     = --debug typical $(SIMTOP)
SIMFLAGS      = $(SIMTOP)

DPILIB        = xsim.dir/work/xsc/dpi.a

#------------------------------------------------------
# BUILD RULES
#------------------------------------------------------

#
# Build the verilog and the PLI shared object by default
#
all: sysverilog

#
# Phony targets
#
.PHONY: help, sim, run, rungui, gui, sysverilog, clean, all

#
# Create Verilog pcieVHost VProc register mappings from C header
#
$(PCIE_MAP_V) : $(SRCDIR)/$(PCIE_MAP_C)
	@sed "s/0x/32\'h/;s/#/\`/;s/DEFINES/AUTO GENERATED\. DO NOT EDIT\!/;" < $< > $@

#
# Build the verilog/systemverilog and the DPI library
#
$(DPILIB): $(ALLSRC) $(SRCDIR)/*.h
	@xsc --gcc_compile_options "$(CFLAGS)" $(ALLSRC)

# Analyse HDL files
sysverilog: $(PCIE_MAP_V) $(DPILIB)
	@xvlog $(ANALYSEFLAGS)
	@xelab $(USRSIMFLAGS) $(ELABFLAGS)

#------------------------------------------------------
# EXECUTION RULES
#------------------------------------------------------

sim: all
	@xsim $(SIMFLAGS)

run: all
	@xsim -R $(SIMFLAGS)

rungui: all
	@xsim -g --autoloadwcfg $(SIMFLAGS)

gui: rungui

help:
	@$(info make help          Display this message)
	@$(info make               Build C/C++ and HDL code without running simulation)
	@$(info make sim           Build and run command line interactive (sim not started))
	@$(info make run           Build and run batch simulation)
	@$(info make rungui/gui    Build and run GUI simulation (sim not started))
	@$(info make clean         clean previous build artefacts)

#------------------------------------------------------
# CLEANING RULES
#------------------------------------------------------

clean: $(VPROC_TOP)
	@rm -rf $(PCIE_MAP_V) xsim* xvlog* xelab* xsc* *.wdb work


