//=============================================================
//
// Copyright (c) 2021-2026 Simon Southwell. All rights reserved.
//
// Date: 1st Aug 2021
//
// This file is part of the pcieVHost package.
//
// pcieVHost is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pcieVHost is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pcieVHost. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

#ifndef _MEM_MODEL_H_
#define _MEM_MODEL_H_

#if defined(MEM_MODEL_VHDL) && !defined(VPROC_VHDL)
#define VPROC_VHDL
#endif

#if defined(MEM_MODEL_SV) && !defined(VPROC_SV)
#define VPROC_SV
#endif

#if !defined(VPROC_VHDL)
# include "string.h"
# if !defined(VPROC_SV)
# include "veriuser.h"
# include "vpi_user.h"
# endif
#endif

#include "mem.h"

#define MEM_MODEL_TF_TBL_SIZE 2

#define MEM_MODEL_ADDR_ARG          1
#define MEM_MODEL_DATA_ARG          2
#define MEM_MODEL_BE_ARG            3

#define MEM_MODEL_DEFAULT_NODE      0

#define MEM_MODEL_BE                0
#define MEM_MODEL_LE                1

#ifndef MEM_MODEL_DEFAULT_ENDIAN
#define MEM_MODEL_DEFAULT_ENDIAN    MEM_MODEL_LE
#endif

# if defined (VPROC_VHDL) || defined(VPROC_SV)

#define MEM_MODEL_TF_TBL

#define MEM_READ_PARAMS    const int  address,       int* data, const int be
#define MEM_WRITE_PARAMS   const int  address, const int  data, const int be

#define MEM_RTN_TYPE       void

# else

#define MEM_MODEL_VPI_TBL \
  {vpiSysTask, 0, "$memread",     MemModelRead,     0, 0, 0}, \
  {vpiSysTask, 0, "$memwrite",    MemModelWrite,    0, 0, 0}

#define MEM_MODEL_VPI_TBL_SIZE 2

#define MEM_READ_PARAMS   char* userdata
#define MEM_WRITE_PARAMS  char* userdata

#define MEM_RTN_TYPE int

# endif

extern MEM_RTN_TYPE MemModelRead     (MEM_READ_PARAMS);
extern MEM_RTN_TYPE MemModelWrite    (MEM_WRITE_PARAMS);

#endif
