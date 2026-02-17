//=============================================================
//
// Copyright (c) 2026 Simon Southwell. All rights reserved.
//
// Date: 11th Feb 2026
//
// This file is part of the pcieVHost package.
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The file is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this file. If not, see <http://www.gnu.org/licenses/>.
//
//=============================================================

//=============================================================
// VUserMain.cpp
//=============================================================

#include <stdio.h>
#include <stdlib.h>

#include "pcieModelClass.h"

//-------------------------------------------------------------
// DispLinkCommon()
//
// Program to set up a pcieVHost as a standalone displink
//
//-------------------------------------------------------------

void DispLinkCommon (int node)
{
    // Create an API object for this node
    pcieModelClass* pcie = new pcieModelClass(node);

    // Initialise PCIe VHost, with input callback function and no user pointer.
    pcie->initialisePcie(NULL, NULL);

    // Speed up model by turning nearly everything off
    pcie->configurePcie(CONFIG_DISABLE_FC);
    pcie->configurePcie(CONFIG_DISABLE_ACK);
    pcie->configurePcie(CONFIG_DISABLE_MEM);
    pcie->configurePcie(CONFIG_DISABLE_SKIPS);
    pcie->configurePcie(CONFIG_DISABLE_ECRC_CMPL);
    pcie->configurePcie(CONFIG_DISABLE_CRC_CHK);
    pcie->configurePcie(CONFIG_DISABLE_UR_CPL);

    // Send out idles forever. For display link, this is done
    // so that the link lane inputs are continuously read
    do
    {
        pcie->sendIdle(0x7fffffff);
    }
    while (true);

    // Halt the simulation (should not reach here)
    VWrite(PVH_FINISH, 0, 0, node);
}

// Support up to 64 displink instantiations

#ifdef DISPLINK0
extern "C" void VUserMain0 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK1
extern "C" void VUserMain1 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK2
extern "C" void VUserMain2 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK3
extern "C" void VUserMain3 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK4
extern "C" void VUserMain4 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK5
extern "C" void VUserMain5 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK6
extern "C" void VUserMain6 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK7
extern "C" void VUserMain7 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK8
extern "C" void VUserMain8 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK9
extern "C" void VUserMain9 (int node) {DispLinkCommon(node);}
#endif

#ifdef DISPLINK10
extern "C" void VUserMain10 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK11
extern "C" void VUserMain11 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK12
extern "C" void VUserMain12 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK13
extern "C" void VUserMain13 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK14
extern "C" void VUserMain14 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK15
extern "C" void VUserMain15 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK16
extern "C" void VUserMain16 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK17
extern "C" void VUserMain17 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK18
extern "C" void VUserMain18 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK19
extern "C" void VUserMain19 (int node) {DispLinkCommon(node);}
#endif

#ifdef DISPLINK20
extern "C" void VUserMain20 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK21
extern "C" void VUserMain21 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK22
extern "C" void VUserMain22 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK23
extern "C" void VUserMain23 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK24
extern "C" void VUserMain24 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK25
extern "C" void VUserMain25 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK26
extern "C" void VUserMain26 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK27
extern "C" void VUserMain27 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK28
extern "C" void VUserMain28 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK29
extern "C" void VUserMain29 (int node) {DispLinkCommon(node);}
#endif

#ifdef DISPLINK30
extern "C" void VUserMain30 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK31
extern "C" void VUserMain31 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK32
extern "C" void VUserMain32 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK33
extern "C" void VUserMain33 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK34
extern "C" void VUserMain34 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK35
extern "C" void VUserMain35 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK36
extern "C" void VUserMain36 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK37
extern "C" void VUserMain37 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK38
extern "C" void VUserMain38 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK39
extern "C" void VUserMain39 (int node) {DispLinkCommon(node);}
#endif

#ifdef DISPLINK40
extern "C" void VUserMain40 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK41
extern "C" void VUserMain41 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK42
extern "C" void VUserMain42 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK43
extern "C" void VUserMain43 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK44
extern "C" void VUserMain44 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK45
extern "C" void VUserMain45 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK46
extern "C" void VUserMain46 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK47
extern "C" void VUserMain47 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK48
extern "C" void VUserMain48 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK49
extern "C" void VUserMain49 (int node) {DispLinkCommon(node);}
#endif

#ifdef DISPLINK50
extern "C" void VUserMain50 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK51
extern "C" void VUserMain51 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK52
extern "C" void VUserMain52 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK53
extern "C" void VUserMain53 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK54
extern "C" void VUserMain54 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK55
extern "C" void VUserMain55 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK56
extern "C" void VUserMain56 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK57
extern "C" void VUserMain57 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK58
extern "C" void VUserMain58 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK59
extern "C" void VUserMain59 (int node) {DispLinkCommon(node);}
#endif

#ifdef DISPLINK60
extern "C" void VUserMain60 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK61
extern "C" void VUserMain61 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK62
extern "C" void VUserMain62 (int node) {DispLinkCommon(node);}
#endif
#ifdef DISPLINK63
extern "C" void VUserMain63 (int node) {DispLinkCommon(node);}
#endif





