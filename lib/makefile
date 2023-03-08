###################################################################
# Makefile for Virtual PCIe Host C code in Modelsim
#
# Copyright (c) 2023 Simon Southwell.
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

SIM       = RivieraPro

SRCDIR    = ../src
OBJDIR    = obj

ifeq ("${SIM}", "ActiveHDL")
  ALDECDIR    =  /c/Aldec/Active-HDL-13-x64
  ARCHFLAGS   = -m64 -I${ALDECDIR}/pli/Include
else
  ALDECDIR    =  /c/Aldec/Riviera-PRO-2022.10-x64
  ARCHFLAGS   = -m64 -I${ALDECDIR}/interfaces/include
endif

C_FILES = codec.c mem.c pcie.c pcie_utils.c ltssm.c pcie_dpi.c pcicrc32.c
CFLAGS  = -c ${ARCHFLAGS} -DLTSSM_ABBREVIATED -I../sysverilog/test

CC      = gcc

LIB     = libpcievhost.a

OBJS    = ${addprefix ${OBJDIR}/, ${C_FILES:%.c=%.o}}

${OBJDIR}/%.o: ${SRCDIR}/%.c
	@${CC} ${CFLAGS} $< -o $@

all: ${LIB}

${LIB} : ${OBJS}
	@ar cr ${LIB} ${OBJS}

${OBJS}: | ${OBJDIR}

${OBJDIR}:
	@mkdir ${OBJDIR}


clean:
	@rm -rf ${LIB} ${OBJS}
