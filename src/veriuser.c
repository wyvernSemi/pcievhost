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
// $Id: veriuser.c,v 1.1 2016/10/04 15:47:34 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/veriuser.c,v $
//
//=====================================================================
// 
// PLI task/function registration table for plitask
// 
//=====================================================================

#include "VSched_pli.h"

extern int PciCrc32(void);
extern int PciCrc16(void);

char *veriuser_version_str = "Virtual Processor PLI V0.1 Copyright (c) 2005 Simon Southwell.";

s_tfcell veriusertfs[VPROC_TF_TBL_SIZE+3] =
{
    VPROC_TF_TBL,
    {usertask, 0, NULL, 0, PciCrc32, NULL,  "$pcicrc32", 1},
    {usertask, 0, NULL, 0, PciCrc16, NULL,  "$pcicrc16", 1},
    {0} 
};

p_tfcell bootstrap ()
{
    return veriusertfs;
}

#ifdef ICARUS
static void veriusertfs_register(void)
{
    veriusertfs_register_table(veriusertfs);
}

void (*vlog_startup_routines[])() = { &veriusertfs_register, 0 };
#endif
