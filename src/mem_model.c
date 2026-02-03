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

#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include "mem_model.h"

/////////////////////////////////////////////////////////////
// PLI access function for $memread.
//   Argument 1 is word address
//   Argument 2 is 32 bit return data
MEM_RTN_TYPE MemModelRead (MEM_READ_PARAMS)
{
    uint32_t data_int, addr;

#if !defined(VPROC_VHDL) && !defined(VPROC_SV)

    uint32_t           address, be;
    vpiHandle          taskHdl;
    int                args[10];

    // Obtain a handle to the argument list
    taskHdl            = vpi_handle(vpiSysTfCall, NULL);

    getArgs(taskHdl, &args[1]);

    address   = args[MEM_MODEL_ADDR_ARG];
    be        = args[MEM_MODEL_BE_ARG];

#endif

    // Get data  from memory model
    if (be == 0x1 || be == 0x2 || be == 0x4 || be == 0x8)
    {
        // Ensure address is 32 bit aligned, then add bottom bits based on byte enables
        addr = (address & ~0x3UL) | ((be == 0x01) ? 0 : (be == 0x02) ? 1 : (be == 0x04) ? 2 : 3);

        // Get the byte from the memory model
        data_int = ReadRamByte(addr, MEM_MODEL_DEFAULT_NODE);

        // Place in the correct lane
        data_int <<= (addr & 0x3) * 8;
    }
    else if (be == 0x3 || be == 0xc)
    {
        // Ensure address is 32 bit aligned, then add bottom bits based on byte enables
        addr = (address & ~0x3UL) | ((be == 0x03) ? 0 : 2);

        // Get the half word from the model
        data_int = ReadRamHWord(addr, MEM_MODEL_DEFAULT_ENDIAN, MEM_MODEL_DEFAULT_NODE);

        // Place in the correct lane
        data_int <<= (addr & 0x3) * 8;
    }
    else
        data_int = ReadRamWord(address, MEM_MODEL_DEFAULT_ENDIAN, MEM_MODEL_DEFAULT_NODE);

#if defined(VPROC_VHDL) || defined(VPROC_SV)
    *data = data_int;
#else

    args[MEM_MODEL_DATA_ARG] = data_int;
    updateArgs(taskHdl, &args[1]);

#endif
}

/////////////////////////////////////////////////////////////
// PLI access function for $memwrite.
//   Argument 1 is word address
//   Argument 2 is 32 bit data
MEM_RTN_TYPE MemModelWrite (MEM_WRITE_PARAMS)
{
    uint32_t addr;

#if !defined(VPROC_VHDL) && !defined(VPROC_SV)
    uint32_t           address, data, be;
    vpiHandle          taskHdl;
    int                args[10];

    // Obtain a handle to the argument list
    taskHdl            = vpi_handle(vpiSysTfCall, NULL);

    getArgs(taskHdl, &args[1]);

    address   = args[MEM_MODEL_ADDR_ARG];
    data      = args[MEM_MODEL_DATA_ARG];
    be        = args[MEM_MODEL_BE_ARG];

#endif

    // Update data in memory model
    if (be == 0x1 || be == 0x2 || be == 0x4 || be == 0x8)
    {
        // Ensure address is 32 bit aligned, then add bottom bits based on byte enables
        addr = (address & ~0x3UL) | ((be == 0x01) ? 0 : (be == 0x02) ? 1 : (be == 0x04) ? 2 : 3);

        WriteRamByte(addr, data >> ((addr & 0x3)*8), MEM_MODEL_DEFAULT_NODE);
    }
    else if (be == 0x3 || be == 0xc)
    {
        // Ensure address is 32 bit aligned, then add bottom bits based on byte enables
        addr = (address & ~0x3UL) | ((be == 0x03) ? 0 : 2);

        uint32_t d = data >> ((addr & 0x3ULL)*8);

        WriteRamHWord(addr, d, MEM_MODEL_DEFAULT_ENDIAN, MEM_MODEL_DEFAULT_NODE);
    }
    else
    {
        WriteRamWord(address, data, MEM_MODEL_DEFAULT_ENDIAN, MEM_MODEL_DEFAULT_NODE);
    }

}
