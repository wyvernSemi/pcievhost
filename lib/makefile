###################################################################
# Makefile for Virtual PCIe Host C code in Modelsim
#
# Copyright (c) 2005-2010 Simon Southwell.
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
# $Id: makefile,v 1.4 2021/05/06 07:59:51 simon Exp $
# $Source: /home/simon/CVS/src/HDL/pcieVHost/lib/makefile,v $
#
###################################################################

# $MODELSIM and MODEL_TECH environment variables must be set
# for pointing to modelsim.ini file and binary dierctory respectively

SRCDIR    = ../src
OBJDIR    = obj
VPROC_TOP = ../../VProc

C_FILES = codec.c mem.c pcie.c pcie_utils.c pcicrc32.c ltssm.c veriuser.c
CFLAGS  = -c -m32 -I${MODEL_TECH}/../include -I ${VPROC_TOP}/code -DMODELSIM -DLTSSM_ABBREVIATED 

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
