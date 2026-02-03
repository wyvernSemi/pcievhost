###################################################################
# Makefile for Virtual PCIe Host C code in Modelsim
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

EXTFLAGS  = 

SRCDIR    = ../src
OBJDIR    = obj
VPROC_TOP = ../../VProc

ARCHFLAG  = -m32

C_FILES   = codec.c mem.c mem_model.c pcie.c pcie_utils.c pcicrc32.c ltssm.c displink.c veriuser.c
CFLAGS    = -c -fPIC $(ARCHFLAG) -I$(MODEL_TECH)/../include -I $(VPROC_TOP)/code -DLTSSM_ABBREVIATED $(EXTFLAGS)

CC        = gcc

LIB       = libpcievhost.a

OBJS      = $(addprefix $(OBJDIR)/, $(C_FILES:%.c=%.o))

${OBJDIR}/%.o: ${SRCDIR}/%.c
	@$(CC) $(CFLAGS) $< -o $@

all: $(LIB)

${LIB} : $(OBJS)
	@ar cr $(LIB) $(OBJS)

${OBJS}: | $(OBJDIR)

${OBJDIR}:
	@mkdir $(OBJDIR)


clean:
	@rm -rf $(LIB) $(OBJS)
