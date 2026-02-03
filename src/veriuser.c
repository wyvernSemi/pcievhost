//=====================================================================
//
// veriuser.c                                         Date: 2002/07/10
//
// Copyright (c) 2002-2010 Simon Southwell.
//
// This file is part of pcieVHost.
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
// along with VProc. If not, see <http://www.gnu.org/licenses/>.
//
//=====================================================================
//
// VPI task/function registration table for plitasks
//
//=====================================================================

#ifndef VPROC_VHDL 

#include "VSched_pli.h"
#include "mem_model.h"

extern int PciCrc32(char* userdata);
extern int PciCrc16(char* userdata);

#ifndef MEM_MODEL_VPI_TBL
#define MEM_MODEL_VPI_TBL
#endif

// -------------------------------------------------------------------------
// register_vpi_tasks()
//
// Registers the VProc system tasks for VPI
// -------------------------------------------------------------------------

static void register_vpi_tasks()
{
    s_vpi_systf_data data[] =
      {
          VPROC_VPI_TBL
#if defined(ENABLE_MEM_PLI)
          MEM_MODEL_VPI_TBL,
#endif
          {vpiSysTask, 0, "$pcicrc16",   PciCrc16,   0, 0, 0},
          {vpiSysTask, 0, "$pcicrc32",   PciCrc32,   0, 0, 0},
      };


    for (int idx= 0; idx < sizeof(data)/sizeof(s_vpi_systf_data); idx++)
    {
        debug_io_printf("registering %s\n", data[idx].tfname);
        vpi_register_systf(&data[idx]);
    }
}

// -------------------------------------------------------------------------
// Contains a zero-terminated list of functions that have
// to be called at startup
// -------------------------------------------------------------------------

void (*vlog_startup_routines[])() =
{
    register_vpi_tasks,
    0
};

#endif

