//=============================================================
//
// Copyright (c) 2024 Simon Southwell. All rights reserved.
//
// Date: 17th Dec 2024
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
//
// C++ wrapper class for PcieVHost API
//
//=============================================================

#include <cstdint>

extern "C" {
#include "pcie.h"
}

#ifndef _PCIEMODELCLASS_H_
#define _PCIEMODELCLASS_H_

class pcieModelClass
{
public:
               pcieModelClass       (const unsigned nodeIn) : node (nodeIn) {};

    // TLP generation
    pPktData_t memWrite             (const uint64_t addr, const PktData_t *data, const int length, const int tag,
                                     const uint32_t rid, const bool queue)
                                        {return MemWrite(addr, data, length, tag, rid, queue, node);};

    pPktData_t memRead              (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue)
                                        {return MemRead(addr, length, tag, rid, queue, node);};

    pPktData_t completion           (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length,
                                     const int tag, const uint32_t cid, const uint32_t rid, const bool queue)
                                        {return Completion(addr, data, status, fbe, lbe, length, tag, cid, rid, queue, node);};

    pPktData_t partCompletion       (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength,
                                     const int length, const int tag, const uint32_t cid, const uint32_t rid, const bool queue)
                                        {return PartCompletion(addr, data, status, fbe, lbe, rlength, length, tag, cid, rid, queue, node);};

    pPktData_t completionDelay      (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length,
                                     const int tag, const uint32_t cid, const uint32_t rid)
                                        {return CompletionDelay(addr, data, status, fbe, lbe, length, tag, cid, rid, node);};

    pPktData_t partCompletionDelay  (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength,
                                     const int length, const int tag, const uint32_t cid, const uint32_t rid, const bool digest, const bool delay,
                                     const bool queue)
                                        {return PartCompletionDelay(addr, data, status, fbe, lbe, rlength, length, tag, cid, rid, digest, delay, queue, node);};

    pPktData_t cfgWrite             (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool queue)
                                        {return CfgWrite(addr, data, length, tag, rid, queue, node);};

    pPktData_t cfgRead              (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue)
                                        {return CfgRead(addr, length, tag, rid, queue, node);};

    pPktData_t ioWrite              (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid,
                                     const bool queue)
                                        {return IoWrite(addr, data, length, tag, rid, queue, node);};

    pPktData_t ioRead               (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue)
                                        {return IoRead(addr, length, tag, rid, queue, node);};

    pPktData_t message              (const int code, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool queue)
                                        {return Message(code, data, length, tag, rid, queue, node);};

    // TLP variant with digest (ECRC) generation argument
    pPktData_t memWriteDigest       (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                     const bool queue)
                                        {return MemWriteDigest(addr, data, length, tag, rid, digest, queue, node);};

    pPktData_t memReadDigest        (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue)
                                        {return MemReadDigest(addr, length, tag, rid, digest, queue, node);};

    pPktData_t completionDigest     (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length,
                                     const int tag, const uint32_t cid, const uint32_t rid, const bool digest, const bool queue)
                                        {return CompletionDigest(addr, data, status, fbe, lbe, length, tag, cid, rid, digest, queue, node);};

    pPktData_t partCompletionDigest (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength,
                                     const int length, const int tag , const uint32_t cid, const uint32_t rid, const bool digest, const bool queue)
                                        {return PartCompletionDigest(addr, data, status, fbe, lbe, rlength, length, tag, cid, rid, digest, queue, node);};

    pPktData_t cfgWriteDigest       (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                     const bool queue)
                                        {return CfgWriteDigest(addr, data, length, tag, rid, digest, queue, node);};

    pPktData_t cfgReadDigest        (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue)
                                        {return CfgReadDigest(addr, length, tag, rid, digest, queue, node);};

    pPktData_t ioWriteDigest        (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                     const bool queue)
                                        {return IoWriteDigest(addr, data, length, tag, rid, digest, queue, node);};

    pPktData_t ioReadDigest         (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue)
                                        {return IoReadDigest(addr, length, tag, rid, digest, queue, node);};

    pPktData_t messageDigest        (const int code, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                                     const bool queue)
                                        {return MessageDigest(code, data, length, tag, rid, digest, queue, node);};

    // Link initialisation
    void       initFc               (void)                 {InitFc(node);};
    void       initLink             (const int linkwidth)  {InitLink(linkwidth, node);};

    // Queue flushing
    void       sendPacket           (void)                 {SendPacket(node);};

    // Dllps
    void       sendAck              (const int seq)        {SendAck(seq, node);};
    void       sendNak              (const int seq)        {SendNak(seq, node);};
    void       sendFC               (const int type,  const int vc,    const int hdrfc, const int datafc, const bool queue)
                                                           {SendFC(type, vc, hdrfc, datafc, queue, node);};
    void       sendPM               (const int type,  const bool queue)
                                                           {SendPM(type, queue, node);};
    void       sendVendor           (const bool queue)
                                                           {SendVendor(queue, node);};

    // Physical layer Ordered sets etc.
    void       sendIdle             (const int ticks)      {SendIdle(ticks, node);};
    void       sendOs               (const int type)       {SendOs(type, node);};
    void       sendTs               (const int identifier, const int lane_num, const int link_num, const int n_fts, const int control,
                                     const bool is_gen2)
                                                           {SendTs(identifier, lane_num, link_num, n_fts, control, is_gen2, node);};

    void       waitForCompletion    (void)                 {WaitForCompletion(node);};
    void       waitForCompletionN   (const uint32_t count) {WaitForCompletionN(count, node);};
    void       initialisePcie       (const callback_t    cb_func, void *usrptr)
                                                           {InitialisePcie(cb_func, usrptr, node);};
    void       registerOsCallback   (const os_callback_t cb_func)
                                                           {RegisterOsCallback(cb_func, node);};
    uint32_t   getCycleCount        (void)                 {return GetCycleCount(node);};
    void       configurePcie        (const int type, const int value)
                                        {ConfigurePcie(type, value, node);};
    void       configurePcie        (const int type)
                                        {ConfigurePcie(type, 0, node);};

    // Physical layer event routines
    int        resetEventCount      (const int type)       {return ResetEventCount(type, node);};
    int        readEventCount       (const int type, uint32_t *ts_data)
                                        {return ReadEventCount(type, ts_data, node);};
    TS_t       getTS                (const int lane)       {return GetTS(lane, node);};

    // Miscellaneous support routines
    uint32_t   pcieRand             (void)                 {return PcieRand(node);};
    void       pcieSeed             (const uint32_t seed)  {PcieSeed(seed, node);};
    void       setTxEnabled         (void)                 {SetTxEnabled(node);};
    void       setTxDisabled        (void)                 {SetTxDisabled(node);};
    void       getPcieVersionStr    (char* sbuf, const int bufsize)
                                                           {getPcieVersionString(sbuf, bufsize);};

    // Memory access
    void       writeRamByteBlock    (const uint64_t addr, const PktData_t* const data, const int fbe, const int lbe, const int length)
                                        {WriteRamByteBlock(addr, data, fbe, lbe, length, node);};
    int        readRamByteBlock     (const uint64_t addr, PktData_t* const data, const int length)
                                        {return ReadRamByteBlock(addr, data, length, node);};

    void       writeRamByte         (const uint64_t addr, const uint32_t data)                          {WriteRamByte(addr, data, node);};
    void       writeRamWord         (const uint64_t addr, const uint32_t data, const int little_endian) {WriteRamWord(addr, data, little_endian, node);};
    void       writeRamDWord        (const uint64_t addr, const uint64_t data, const int little_endian) {WriteRamDWord(addr, data, little_endian, node);};
    uint32_t   readRamByte          (const uint64_t addr)                                               {return ReadRamByte(addr, node);};
    uint32_t   readRamWord          (const uint64_t addr, const int little_endian)                      {return ReadRamWord(addr, little_endian, node);};
    uint64_t   readRamDWord         (const uint64_t addr, const int little_endian)                      {return ReadRamDWord(addr, little_endian, node);};

    void       writeConfigSpace     (const uint32_t addr, const uint32_t data)                          {WriteConfigSpace(addr, data, node);};
    uint32_t   readConfigSpace      (const uint32_t addr)                                               {return ReadConfigSpace(addr, node);};
    void       writeConfigSpaceMask (const uint32_t addr, const uint32_t data)                          {WriteConfigSpaceMask(addr, data, node);};
    uint32_t   readConfigSpaceMask  (const uint32_t addr)                                               {return ReadConfigSpaceMask(addr, node);};

private:

    unsigned node;

};

#endif