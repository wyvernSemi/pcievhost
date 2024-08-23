//=============================================================
// 
// Copyright (c) 2016 Simon Southwell. All rights reserved.
//
// Date: 20th Sep 2016
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
// $Id: pcie.c,v 1.4 2016/10/17 11:47:01 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/pcie.c,v $
//
//=============================================================
//
// A PCIe model of a host/endpoint which use the Virtual
// Processor (VProc) PLI package to generate and receive PCIe 
// traffic over a 16 lane link in a Verilog simulation.
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include "pcie.h"
#include "pcie_utils.h"
#include "codec.h"
#include "ltssm.h"

// -------------------------------------------------------------------------
// STATICS
// -------------------------------------------------------------------------

// Pointer to model's private internal state for each instantiation
static pPcieModelState_t *pms = NULL;

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

#define this pms[node]

// -------------------------------------------------------------------------
// SendPacket()
//
// Processes a linked list of raw packets, until either
// exhausted (if num_of_packets == 0) or count ==
// num_of_packets.
//
// -------------------------------------------------------------------------

void SendPacket(const int node)
{
    int lanes = 0, idx = 0;
    pPkt_t tmp_p;
    sPkt_t AckHolder, NakHolder;
    uint32_t code;
    uint32_t LinkIn [MAX_LINK_WIDTH];
    int i;

    PktData_t AckDataHolder[MAX_DLLP_BYTES], NakDataHolder[MAX_DLLP_BYTES];

    pUserConfig_t usrconf    = &(this->usrconf);
    bool          padding    = false;
    bool          sdp_output = false;

    DebugVPrint("** Entering SendPacket (send_p=%p)\n", this->send_p);

    if (this->draining_queue)
    {
        VPrint("MemWrite: ***Error --- Re-entered SendPacket() at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    this->draining_queue = true;
    
    // If we have outstanding Acks, clear the head of the queue for the
    // acknowledged packets
    if (!usrconf->DisableAck)
    {
        // Service received  acknowledges

        if (this->curr_ack != NULLACK)
        {
            while ((this->head_p != NULL && this->head_p != this->send_p) && 
                   ((this->head_p->seq != DLLP_SEQ_ID && this->head_p->seq <= this->curr_ack) || 
                    (this->head_p->seq == DLLP_SEQ_ID)))
            {
                tmp_p = this->head_p;
                this->head_p = this->head_p->NextPkt;
                CheckFree(tmp_p->data);
                CheckFree(tmp_p);
            }
            if (this->head_p == NULL)
            {
                this->head_p = this->end_p = this->send_p;
            }
            this->curr_ack = NULLACK;
        }

        // If we have received a NAK, reset the send pointer to the NAK'ed packet
        if (this->curr_nak != NULLACK)
        {
            this->send_p = this->head_p;
            while (this->send_p->seq > this->curr_nak)
            {
                this->send_p = this->send_p->NextPkt;
            }
            this->curr_nak = NULLACK;
        }

        // If we've ACk/NAKs to send, insert to front of send list
        if (this->nak_to_send_p != NULL && ((GetCycleCount(node) - this->nak_to_send_p->TimeStamp) > usrconf->AckRate)) 
        {
            // Snap shot the Nak to send (it may get overwritten by new input)
            NakHolder = *(this->nak_to_send_p);
            NakHolder.data = NakDataHolder;
            for (i = 0; i < MAX_DLLP_BYTES; i++)
            {
                NakDataHolder[i] = this->nak_to_send_p->data[i];
            }
            NakHolder.NextPkt = this->send_p;
            this->send_p = &NakHolder;

            // Free up current NAK memory
            CheckFree(this->nak_to_send_p->data);
            CheckFree(this->nak_to_send_p);

            // Mark as sent
            this->nak_to_send_p = NULL;
        }
        if (this->ack_to_send_p != NULL && ((GetCycleCount(node) - this->ack_to_send_p->TimeStamp) > usrconf->AckRate))
        {
            // Snap shot the Ack to send (it may get overwritten by new input)
            AckHolder = *(this->ack_to_send_p);
            AckHolder.data = AckDataHolder;
            for (i = 0; i < MAX_DLLP_BYTES; i++)
            {
                AckDataHolder[i] = this->ack_to_send_p->data[i];
            }
            AckHolder.NextPkt = this->send_p;
            this->send_p = &AckHolder;

            // Free up current ACK memory
            CheckFree(this->ack_to_send_p->data);
            CheckFree(this->ack_to_send_p);

            // Mark as sent
            this->ack_to_send_p = NULL;
        }
    }

    // Send out any accumulated skips
    while (this->SkipScheduled)
    {        
        SendOs(SKP, node);
    }

    // Main output packet loop
    do
    {
        // Loop through lanes
        for (lanes = 0; lanes < this->LinkWidth; lanes++)
        {
            // Flag when an SDP is output for this cycle (sticky)
            if (!sdp_output && !padding && this->send_p)
            {
                sdp_output = (this->send_p->data[idx] == SDP);
            }

            // Whilst in padding mode, encode to end of lanes with PAD, else encode data
            if (padding)
            {
                code = Encode(PAD, false, lanes, this->LinkWidth, node);
            }

            // If nothing to send (and not padding), output IDLE
            else if (!this->send_p)
            {
                code = Encode(0, false, lanes, this->LinkWidth, node);
            }
            else
            {
                code = Encode(this->send_p->data[idx++], false, lanes, this->LinkWidth, node);
            }

            // Output codes to current lanes and read input
            LinkIn[lanes] = (uint32_t)VWrite(LINKADDR0+lanes, code, lanes != this->LinkWidth-1, node);

            // last lane, so process input values 
            if (lanes == (this->LinkWidth-1))
            {
                ExtractPhyInput(this, LinkIn);
            }

            // Clear any padding status on last lane
            if (lanes == this->LinkWidth-1)
            {
                padding = 0;
                sdp_output = false;
            } 
            else
            {
                // If not already padding, monitor for padding status; i.e. Output DLLP and
                // next to send is DLLP, or nothing to send.
                if (!padding)
                {
                    padding = (this->send_p && (this->send_p->data[idx] == PKT_TERMINATION) && sdp_output && 
                              (this->send_p->NextPkt != NULL) && (this->send_p->NextPkt->seq == DLLP_SEQ_ID));
                }
            }

            // At end of packet move to next unless padding
            if (this->send_p)
            {
                if (!padding && this->send_p->data[idx] == PKT_TERMINATION)
                {
                    this->send_p->Retry += 1;
                    this->send_p = this->send_p->NextPkt;
                    idx = 0;
                    padding = (this->send_p == NULL);
                }
            }
        }
    } 
    while (this->send_p != NULL);

    // If acknowledges disabled, then delete all the packets
    // on the queue---the user code is responsible for retries
    if (usrconf->DisableAck)
    {
        while (this->head_p != NULL)
        {
            tmp_p = this->head_p;
            this->head_p = this->head_p->NextPkt;
            CheckFree(tmp_p->data);
            CheckFree(tmp_p);
        }
        this->head_p = this->end_p = this->send_p = NULL;
    }

    // If we've terminated midway through the lanes, then flush with PADs
    this->draining_queue = false;
    
    DebugVPrint("** Exiting SendPacket (send_p=%p)\n", this->send_p);
}

// -------------------------------------------------------------------------
// MemWrite()
//
// User request to send a memory write packet. Address, length (in bytes), tag
// and requestor ID supplied by user. The data is passed in as a pointer to
// packet data (*not* char, though each element of the array is a byte). The
// 'queue' input (true | false) indicates whether to simply add to the queue
// (true) or add and then send (false). With queue true, mutiple packets can
// be added before sending allowing contiguous packets on the link (i.e. not
// all padded and starting from lane 0).
//
// -------------------------------------------------------------------------

pPktData_t MemWrite (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool queue, const int node)
{
    return MemWriteDigest (addr, data, length, tag, rid, true, queue, node);
}

pPktData_t MemWriteDigest (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                           const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("MemWrite(): ***Error --- invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("MemWrite: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (length < 0 || length > MAX_PAYLOAD_BYTES)
    {
        VPrint( "MemWrite: ***Error --- invalid payload length (%d) at node %d\n", length, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if (length && (((addr + (uint64_t)(length-1)) & ~(MASK_4K_BITS)) > (addr & ~(MASK_4K_BITS))))
    {
        VPrint( "MemWrite: ***Error --- address + length crosses 4K boundary at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Create a template for a mem write
    if ((pkt_p = CreateTlpTemplate (TL_MWR64, addr, length, digest, &data_p)) == NULL)
    {
        VPrint( "MemWrite: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_TLP_TAG(tag, pkt_p);
    SET_TLP_RID(rid, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    for (i = 0; i < length; i++)
    {
        data_p[i] = (PktData_t)data[i];
    }

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = (pPkt_t)calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "MemWrite: ***Error --- memory allocation failed\n");
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;

    this->seq++;

    // Check there's enough space. If not call SendPacket to force out 
    // any packets on queue (that do have space), or idle if queue empty
    if (!this->usrconf.DisableFc) 
    {        
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_POST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_POST], 
                             this->flwcntl.TxHdrCredits[0][FC_POST], 
                             this->flwcntl.TxDataCredits[0][FC_POST], 
                             GET_TLP_LENGTH_ADJ(packet->data)))
        {
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_POST]++;
        this->flwcntl.TxDataCredits[0][FC_POST] += GET_TLP_LENGTH_ADJ(packet->data)/4 + ((GET_TLP_LENGTH_ADJ(packet->data)%4) ? 1 : 0);
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    } 
    else
    {        
        return packet->data;
    }
}

// -------------------------------------------------------------------------
// MemRead()
//
// User request to send a memory read packet. Address, length (in bytes), tag
// and requestor ID supplied by user. 'queue' is as for MemWrite above.
// MemRead returns immediately the packet is added to the queue and
// sent (if requested). The returned data arrives via the registered
// user callback function at some future time. The user code must manage
// the consolidation of read requests and returned completions, as well
// as the management of tags. The WaitForCompletion() funtion is used
// to stall for data if required.
//
// -------------------------------------------------------------------------

pPktData_t MemRead (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue, const int node)
{
    return MemReadDigest (addr, length, tag, rid, true, queue, node);
}

pPktData_t MemReadDigest (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, 
                          const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("MemRead: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("MemRead: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (length < 0 || length > MAX_PAYLOAD_BYTES)
    {
        VPrint( "MemRead: ***Error --- invalid payload (%d) at node %d\n", length, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if (length && (((addr + (uint64_t)(length-1)) & ~(MASK_4K_BITS)) > (addr & ~(MASK_4K_BITS))))
    {
        VPrint( "MemRead: ***Error --- address + length crosses 4K boundary at node %d, addr 0x%lx, length 0x%x\n", node, addr, length);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Create a template for a mem read
    if ((pkt_p = CreateTlpTemplate (TL_MRD64, addr, length, digest, &data_p)) == NULL)
    {
        VPrint( "MemRead: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_TLP_TAG(tag, pkt_p);
    SET_TLP_RID(rid, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "MemRead: ***Error --- memory allocation failed\n");
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;

    this->seq++;
    this->OutstandingCompletions++;

    if (!this->usrconf.DisableFc)
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_NONPOST], 
                             this->flwcntl.TxHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.TxDataCredits[0][FC_NONPOST], 
                             0))
        {                     
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_NONPOST]++;
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    } 
    else
    {
        return packet->data;
    }
}

// -------------------------------------------------------------------------
// Completion()
//
// Used to generate a completion packet and add to the send queue.
// Normally only called from within ProcessInput() for read requests,
// it is made available to the user for potentially implementing
// IO reads etc. at a higher level.
//
// -------------------------------------------------------------------------

pPktData_t Completion (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int length, 
                       const int tag, const uint32_t cid, const uint32_t rid, const bool queue, const int node)
{
    return CompletionDigest(addr, data, status, fbe, lbe, length, tag, cid, rid, true, queue, node);
}

pPktData_t CompletionDigest (uint64_t addr, const PktData_t *data, int status, int fbe, int lbe, int length, int tag,
                             uint32_t cid, uint32_t rid, bool digest, bool queue, int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("Completion: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("Completion: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (length < 0 || length > (MAX_PAYLOAD_BYTES/4))
    {
        VPrint( "Completion: ***Error --- invalid payload (%d) at node %d\n", length, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 

    // A full completion uses the PartCompletion function, where a remaining length
    // is equal to the actual length indicating no further completions.
    return PartCompletionDigest(addr, data, status, fbe, lbe, length, length, tag, cid, rid, digest, queue, node);
}

pPktData_t CompletionDelay (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, 
                            const int length, const int tag, const uint32_t cid, const uint32_t rid, const int node)
{
     return PartCompletionDelay (addr, data, status, fbe, lbe, length, length, tag, cid, rid, true, true, true, node);
}

// -------------------------------------------------------------------------
// PartCompletion()
//
// Same as Completion(), but remaining rlength can be
// more than length to indicate that the completion is
// not final for given request (tag). Equal length and
// rlength indicates whole or final completion packet.
//
// -------------------------------------------------------------------------

pPktData_t PartCompletion (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, 
                           const int rlength, const int length, const int tag, const uint32_t cid, const uint32_t rid,
                           const bool queue, const int node)
{
    return PartCompletionDigest (addr, data, status, fbe, lbe, rlength, length, tag, cid, rid, true, queue, node);
}

pPktData_t PartCompletionDelay (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, 
                                const int rlength, const int length, const int tag, const uint32_t cid, const uint32_t rid,
                                const bool digest, const bool delay, const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("PCompletion: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("PCompletion: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (length < 0 || length > (MAX_PAYLOAD_BYTES/4))
    {
        VPrint( "PCompletion: ***Error --- invalid payload (%d) at node %d\n", length, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 

    // Create a template for a mem read completiom
    if ((pkt_p = CreateTlpTemplate (length ? TL_CPLD : TL_CPL, addr, length*4, digest, &data_p)) == NULL)
    {
        VPrint( "PCompletion: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_CPL_TAG(tag, pkt_p);
    SET_CPL_CID(cid, pkt_p);
    SET_CPL_RID(rid, pkt_p);
    SET_CPL_STATUS(status, pkt_p);
    SET_CPL_BYTE_COUNT(status ? 4 : CalcByteCount(rlength, fbe, lbe), pkt_p);
    SET_CPL_LOW_ADDR(status ? 0 : (addr | CalcLoAddr(fbe)) & 0x7f, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    for (i = 0; i < (length*4); i++)
    {
        data_p[i] = data[i] & 0xff;
    }

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "PCompletion: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;
    packet->TimeStamp = this->TicksSinceReset;

    if (!this->usrconf.DisableFc) 
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_CMPL], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_CMPL], 
                             this->flwcntl.TxHdrCredits[0][FC_CMPL], 
                             this->flwcntl.TxDataCredits[0][FC_CMPL], 
                             GET_TLP_LENGTH_ADJ(packet->data)))
        {
            SendPacket(node);
        }
    }

    if (delay)
    {
        AddPktToQueueDelay(this, packet);
    }
    else
    {
        AddPktToQueue(this, packet);
    }

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_CMPL]++;
        this->flwcntl.TxDataCredits[0][FC_CMPL] += GET_TLP_LENGTH_ADJ(packet->data)/4 + ((GET_TLP_LENGTH_ADJ(packet->data)%4) ? 1 : 0);
    }

    this->seq++;

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    }
    else
    {
        return packet->data;
    }
}

pPktData_t PartCompletionDigest (const uint64_t addr, const PktData_t *data, const int status, const int fbe, const int lbe, const int rlength, 
                                 const int length, const int tag , const uint32_t cid, const uint32_t rid, const bool digest, const bool queue, const int node)
{
    return PartCompletionDelay (addr, data, status, fbe, lbe, rlength, length, tag, cid, rid, digest, false, queue, node);
}

// -------------------------------------------------------------------------
// IoWrite()
//
// Similar to MemWrite, but data packet limited to 1 DW.
//
// -------------------------------------------------------------------------

pPktData_t IoWrite (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool queue, const int node)
{
    return IoWriteDigest(addr, data, length, tag, rid, true, queue, node);
}

pPktData_t IoWriteDigest (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest, 
                          const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("IoWrite: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("IoWrite: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (((addr & ADDR_DW_OFFSET_MASK) + length) > 4)
    {
        VPrint( "IoWrite: ***Error --- payload > 1 at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }
    else if (addr > ADDR_LO_BIT_MASK)
    {
        VPrint( "IoWrite: ***Error --- 64 bit address at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Create a template for an IO write
    if ((pkt_p = CreateTlpTemplate (TL_IOWR, addr, length, digest, &data_p)) == NULL)
    {
        VPrint( "IoWrite: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_TLP_TAG(tag, pkt_p);
    SET_TLP_RID(rid, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    for (i = 0; i < length; i++)
    {
        data_p[i] = (PktData_t)data[i];
    }

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "IoWrite: ***Error --- memory allocation failed\n");
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;

    this->seq++;
    this->OutstandingCompletions++;

    if (!this->usrconf.DisableFc)
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_NONPOST], 
                             this->flwcntl.TxHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.TxDataCredits[0][FC_NONPOST], 
                             GET_TLP_LENGTH_ADJ(packet->data)))
        {
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_NONPOST]++;
        this->flwcntl.TxDataCredits[0][FC_NONPOST] += GET_TLP_LENGTH_ADJ(packet->data)/4 + ((GET_TLP_LENGTH_ADJ(packet->data)%4) ? 1 : 0);
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    } 
    else
    {
        return packet->data;
    }
}

// -------------------------------------------------------------------------
// IoRead()
//
// Similar to MemRead, but data packet request limited to 1 DW.
//
// -------------------------------------------------------------------------

pPktData_t IoRead (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue, const int node)
{
    return IoReadDigest (addr, length, tag, rid, true, queue, node);
}

pPktData_t IoReadDigest (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("IoRead: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("IoRead: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (((addr & ADDR_DW_OFFSET_MASK) + length) > 4)
    {
        VPrint( "IoRead: ***Error --- payload > 1 at bode %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if (addr > ADDR_LO_BIT_MASK)
    {
        VPrint( "IoRead: ***Error --- 64 bit address at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Create a template for an IO read
    if ((pkt_p = CreateTlpTemplate (TL_IORD, addr, length, digest, &data_p)) == NULL)
    {
        VPrint( "IoRead: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_TLP_TAG(tag, pkt_p);
    SET_TLP_RID(rid, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "IoRead: ***Error --- memory allocation failed\n");
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;

    this->seq++;
    this->OutstandingCompletions++;

    if (!this->usrconf.DisableFc)
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_NONPOST], 
                             this->flwcntl.TxHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.TxDataCredits[0][FC_NONPOST], 
                             0))
        {
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_NONPOST]++;
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    } 
    else
    {
        return packet->data;
    }

}

// -------------------------------------------------------------------------
// CfgWrite()
// 
// Similar to MemWrite, but to configuration space and
// limited to 1 DW.
//
// -------------------------------------------------------------------------

pPktData_t CfgWrite (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, 
                     const bool queue, const int node)
{
    return CfgWriteDigest (addr, data, length, tag, rid, true, queue, node);
}

pPktData_t CfgWriteDigest (const uint64_t addr, const PktData_t *data, const int length, const int tag, const uint32_t rid, const bool digest,
                           const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("CfgWrite: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("CfgWrite: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (((addr & ADDR_DW_OFFSET_MASK) + length) > 4)
    {
        VPrint( "CfgWrite: ***Error --- payload > 1 at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if ((addr & 0xffff) > TLP_CFG_LO_ADDR_MASK)
    {
        VPrint( "CfgWrite: ***Error --- address > 7 bits at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Create a template for a cfg write
    if ((pkt_p = CreateTlpTemplate (TL_CFGWR0, addr, length, digest, &data_p)) == NULL)
    {
        VPrint( "CfgWrite: ***Error --- CreateTlpTemplate failed at node%d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_CFG_TAG(tag, pkt_p);
    SET_CFG_RID(rid, pkt_p);
    SET_CFG_CID((uint32_t)(addr >> 16), pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    for (i = 0; i < length; i++)
    {
        data_p[i] = (PktData_t)data[i];
    }

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "CfgWrite: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;

    this->seq++;
    this->OutstandingCompletions++;

    if (!this->usrconf.DisableFc)
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_NONPOST], 
                             this->flwcntl.TxHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.TxDataCredits[0][FC_NONPOST], 
                             GET_TLP_LENGTH_ADJ(packet->data)))
        {                     
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_NONPOST]++;
        this->flwcntl.TxDataCredits[0][FC_NONPOST] += GET_TLP_LENGTH_ADJ(packet->data)/4 + ((GET_TLP_LENGTH_ADJ(packet->data)%4) ? 1 : 0);
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    } 
    else
    {
        return packet->data;
    }
}

// -------------------------------------------------------------------------
// CfgRead()
//
// Similar to MemRead, but to configuration space and
// limited to 1 DW.
//
// -------------------------------------------------------------------------

pPktData_t CfgRead (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool queue, const int node)
{
    return CfgReadDigest(addr, length, tag, rid, true, queue, node);
}

pPktData_t CfgReadDigest (const uint64_t addr, const int length, const int tag, const uint32_t rid, const bool digest, 
                          const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int i;

    // Do some checks
    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("CfgRead: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("CfgRead: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (((addr & ADDR_DW_OFFSET_MASK) + length) > 4)
    {
        VPrint( "CfgRead: ***Error --- payload > 1 at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }
    else if ((addr & 0xffff) > TLP_CFG_LO_ADDR_MASK)
    {
        VPrint( "CfgRead: ***Error --- address > 7 bits at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Create a template for a cfg read
    if ((pkt_p = CreateTlpTemplate (TL_CFGRD0, addr, length, digest, &data_p)) == NULL)
    {
        VPrint( "CfgRead: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_TLP_TAG(tag, pkt_p);
    SET_CFG_RID(rid, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    }
    CalcLcrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "CfgRead: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = this->seq;
    packet->Retry = 0;

    this->seq++;
    this->OutstandingCompletions++;

    if (!this->usrconf.DisableFc)
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_NONPOST], 
                             this->flwcntl.TxHdrCredits[0][FC_NONPOST], 
                             this->flwcntl.TxDataCredits[0][FC_NONPOST], 
                             0))
        {
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_NONPOST]++;
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    }
    else
    {
        return packet->data;
    }

}

// -------------------------------------------------------------------------
// Message()
//
// Generates a message packet, adds to the send queue 
// (see MemWrite) and sends over the link (if requested).
// Most messages have no data, but for those that do, 
// a pointer to an array of packet data is passed in.
//
// -------------------------------------------------------------------------

pPktData_t Message (const int code, const PktData_t *data, const int length, const int tag, const uint32_t rid,
                    const bool queue, const int node)
{
    return MessageDigest(code, data, length, tag, rid, true, queue, node);
}

pPktData_t MessageDigest (const int code, const PktData_t *data, const int length, const int tag, const uint32_t rid,
                         const bool digest, const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int type, routing, i;

    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("Message: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("Message: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (((code == MSG_VENDOR_0 || code == MSG_VENDOR_1) && length != 0) || code == MSG_SET_PWR_LIMIT)
    {
        type = TL_MSGD;
    }

    switch (code)
    {
    case MSG_VENDOR_0:
    case MSG_VENDOR_1:
    case MSG_SET_PWR_LIMIT:
        if (length || code == MSG_SET_PWR_LIMIT)
        {
            type = TL_MSGD;
        }
        else
        {
            type = TL_MSG;
        }
        routing = MSG_ROUTE_LOCAL;
        break;

    case MSG_ASSERT_INTA:
    case MSG_ASSERT_INTB:
    case MSG_ASSERT_INTC:
    case MSG_ASSERT_INTD:
    case MSG_DEASSERT_INTA:
    case MSG_DEASSERT_INTB:
    case MSG_DEASSERT_INTC:
    case MSG_DEASSERT_INTD:
    case MSG_PM_ACTIVE_NAK:
        type = TL_MSG;
        routing = MSG_ROUTE_LOCAL;
        break;

    case MSG_ERR_COR:
    case MSG_ERR_NONFATAL:
    case MSG_ERR_FATAL:
    case MSG_PM_PME:
        type = TL_MSG;
        routing = MSG_ROUTE_ROOT;
        break;

    case MSG_PME_TO_ACK:
        type = TL_MSG;
        routing = MSG_ROUTE_GATHER;
        break;

    case MSG_PME_OFF:
    case MSG_UNLOCK:
        type = TL_MSG;
        routing = MSG_ROUTE_BCAST;
        break;

    default:
        VPrint( "Message: ***Error --- invalid message code (%0x) at node %d\n", code, node);
        VWrite(PVH_FATAL, 0, 0, node);
        break;
    }

    // Create a template for a message
    if ((pkt_p = CreateTlpTemplate (type | routing, 0, length, digest, &data_p)) == NULL)
    {
        VPrint( "Message: ***Error --- CreateTlpTemplate failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Set the tag and sequence number of the packet
    SET_TLP_TAG(tag, pkt_p);
    SET_TLP_RID(rid, pkt_p);
    SET_MSG_CODE(code, pkt_p);
    SET_DLLP_SEQ(this->seq, pkt_p);

    for (i = 0; i < length; i++)
    {
        data_p[i] = (PktData_t)data[i];
    }

    // Calc CRCs
    if (digest)
    {
        CalcEcrc(pkt_p);
    CalcLcrc(pkt_p);
    }

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "Message: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq  = this->seq;
    packet->Retry = 0;

    this->seq++;

    if (!this->usrconf.DisableFc)
    {
        while (!CheckCredits(this->usrconf.DisableFc,
                             this->flwcntl.fc_state[0], 
                             this->flwcntl.FlowCntlHdrCredits[0][FC_POST], 
                             this->flwcntl.FlowCntlDataCredits[0][FC_POST], 
                             this->flwcntl.TxHdrCredits[0][FC_POST], 
                             this->flwcntl.TxDataCredits[0][FC_POST], 
                             length ? GET_TLP_LENGTH_ADJ(packet->data) : 0))
        {
            SendPacket(node);
        }
    }

    AddPktToQueue(this, packet);

    if (!this->usrconf.DisableFc)
    {
        this->flwcntl.TxHdrCredits[0][FC_POST]++;
        this->flwcntl.TxDataCredits[0][FC_POST] += length ? GET_TLP_LENGTH_ADJ(packet->data)/4 + ((GET_TLP_LENGTH_ADJ(packet->data)%4) ? 1 : 0) : 0;
    }

    if (!queue)
    {
        SendPacket (node);
        return NULL;
    } 
    else
    {        
        return packet->data;
    }

}

// -------------------------------------------------------------------------
// SendAck()
//
// Send an acknowledge over the link. Normally called 
// from within ProcessInput(), it is made available to the
// user for potentially generating error cases. There should
// only ever be 0 or 1 Ack to send, so packet is not added
// directly to the send queue. If a new ack comes in before
// an outstanding ack is sent, it overwrites it, if it meets
// the right criteria. The latest ack is sent from SendPacket()
// in between other traffic whenever the ack_to_send_p indicates
// that there is one to send.
//
// -------------------------------------------------------------------------

void SendAck (const int sequence, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    uint32_t OldTimeStamp;

    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("SendAck: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendAck: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (sequence < 0 || sequence > 4095)
    {
        VPrint( "SendAck: ***Error --- sequence > 4095 at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // If no Ack pending, or requested Ack is higher than current sequence, generate
    // a new packet and set Ack pointer to it
    if (this->ack_to_send_p == NULL || sequence > GET_DLLP_SEQ(this->ack_to_send_p->data))
    {

        // Free up space for superseded Ack
        if (this->ack_to_send_p != NULL)
        {
            OldTimeStamp = this->ack_to_send_p->TimeStamp;
            CheckFree(this->ack_to_send_p->data);
            CheckFree(this->ack_to_send_p);
        }
        
        // Generate an Ack data template
        pkt_p = CreateDllpTemplate (DL_ACK, &data_p);

        data_p[0] = 0;
        data_p[1] = (PktData_t)((sequence >> 8) & 0x0f);
        data_p[2] = (PktData_t)(sequence & BYTE_MASK);

        // Calc CRC
        CalcDllpCrc(pkt_p);

        // Create a new packet
        if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
        {
            VPrint( "MemWrite: ***Error --- memory allocation failed at node %d\n", node);
            VWrite(PVH_FATAL, 0, 0, node);
        }

        packet->NextPkt   = NULL;
        packet->data      = pkt_p;
        packet->seq       = DLLP_SEQ_ID;
        packet->Retry     = 0;

        // Carry over outstanding Ack's timestamp, otherwise we might never send one!
        if (this->ack_to_send_p != NULL)
        {
            packet->TimeStamp = OldTimeStamp;
        }
        else
        {
            packet->TimeStamp = GetCycleCount(node);
        }

        this->ack_to_send_p = packet;
    }
}

// -------------------------------------------------------------------------
// SendNak()
//
// Send an bad acknowledge over the link. Normally called 
// from within ProcessInput(), it is made available to the
// user for potentially generating error cases. Operation
// is similar to SendAck().
//
// -------------------------------------------------------------------------

void SendNak (const int sequence, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    uint32_t OldTimeStamp;

    if (node < 0 || node >=  VP_MAX_NODES)
    {
        VPrint("SendNak: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendNak: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (sequence < 0 || sequence > 4095)
    {
        VPrint( "SendNak: ***Error --- sequence > 4095 at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // If no Nak pending, or requested Nak is lowest sequence, generate
    // a new packet and set NAK pointer to it
    if (this->nak_to_send_p == NULL || sequence < GET_DLLP_SEQ(this->nak_to_send_p->data))
    {
        // Free up space for superseded Nak
        if (this->nak_to_send_p != NULL)
        {
            OldTimeStamp = this->ack_to_send_p->TimeStamp;
            CheckFree(this->nak_to_send_p->data);
            CheckFree(this->nak_to_send_p);
        }
        
        // Generate a NAK data template
        pkt_p = CreateDllpTemplate (DL_NAK, &data_p);

        // Fill in specific data (sequence number)
        data_p[0] = 0;
        data_p[1] = (PktData_t)((sequence >> 8) & 0x0f);
        data_p[2] = (PktData_t)(sequence & BYTE_MASK);

        // Calc CRC
        CalcDllpCrc(pkt_p);

        // Create a new packet
        if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
        {
            VPrint( "SendNak: ***Error --- memory allocation failed\n");
            VWrite(PVH_FATAL, 0, 0, node);
        }

        packet->NextPkt = NULL;
        packet->data    = pkt_p;
        packet->seq     = DLLP_SEQ_ID;
        packet->Retry   = 0;
        packet->TimeStamp = GetCycleCount(node);

        // Carry over outstanding Nak's timestamp, otherwise we might never send one!
        if (this->nak_to_send_p != NULL)
        {
            packet->TimeStamp = OldTimeStamp;
        }
        else
        {
            packet->TimeStamp = GetCycleCount(node);
        }

        this->nak_to_send_p = packet;
    }    
}

// -------------------------------------------------------------------------
// SendFC()
//
// Creates a flow control packet of given type and credits,
// adds to the queue and sends it over the link (if requested).
// The model can always accept packets, and Rx flow control must
// be managed at the user level. 
//
// -------------------------------------------------------------------------

void SendFC (const int type, const int vc, const int hdrfc, const int datafc, const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;
    int Encoding;

    DebugVPrint("** SendFC: type=%d hdrfc=%d datafc=%d queue=%d\n", type, hdrfc, datafc, queue);

    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("SendFC: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendFC: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (vc < 0 || vc >= NUM_VIRTUAL_CHANNELS)
    {
        VPrint( "SendFC: ***Error --- invalid virtual channel (%d). Maximum supported = %d at node %d\n", vc, NUM_VIRTUAL_CHANNELS, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if (hdrfc < 0 || hdrfc > DL_MAX_HDRFC || datafc < 0 || datafc > DL_MAX_DATAFC)
    {
        VPrint( "SendFC: ***Error --- invalid FC credits (hdr=%d data=%d) at node %d\n", hdrfc, datafc, node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    Encoding = type | (vc & DL_VC_BITS);

    pkt_p = CreateDllpTemplate (Encoding, &data_p);

    data_p[0] = (PktData_t)(hdrfc >> 2) & 0x3f;
    data_p[1] = (PktData_t)(((hdrfc & 0x3) << 6) | ((datafc >> 8) & 0xf));
    data_p[2] = (PktData_t)(datafc & BYTE_MASK);

    // Calc CRC
    CalcDllpCrc(pkt_p);

    if ((packet = calloc(1, sizeof(sPkt_t))) == NULL)
    {
        VPrint( "SendFC: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = DLLP_SEQ_ID;
    packet->Retry = 0;

    AddPktToQueue(this, packet);

    if (!queue)
    {
        SendPacket (node);
    }
}

// -------------------------------------------------------------------------
// SendPM()
//
// Send power management packet of given type.
//
// -------------------------------------------------------------------------

void SendPM (const int type, const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;

    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint("SendPM: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendPM: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    pkt_p = CreateDllpTemplate (type, &data_p);

    data_p[0] = 0;
    data_p[1] = 0;
    data_p[2] = 0;

    // Calc CRC
    CalcDllpCrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "SendPM: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = DLLP_SEQ_ID;
    packet->Retry = 0;

    AddPktToQueue(this, packet);

    if (!queue)
    {
        SendPacket (node);
    }
}

// -------------------------------------------------------------------------
// SendVendor()
//
// Generate a Vendor packet. Currently simply sends an
// empty vendor packet.
//
// -------------------------------------------------------------------------

void SendVendor (const bool queue, const int node)
{
    PktData_t *pkt_p, *data_p;
    pPkt_t packet;

    if (node < 0 || node > VP_MAX_NODES)
    {
        VPrint("SendVendor: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendVendor: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    pkt_p = CreateDllpTemplate (DL_VENDOR, &data_p);

    data_p[0] = 0;
    data_p[1] = 0;
    data_p[2] = 0;

    // Calc CRC
    CalcDllpCrc(pkt_p);

    if ((packet = calloc(sizeof(sPkt_t), 1)) == NULL)
    {
        VPrint( "SendVendor: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    packet->NextPkt = NULL;
    packet->data = pkt_p;
    packet->seq = DLLP_SEQ_ID;
    packet->Retry = 0;

    AddPktToQueue(this, packet);

    if (!queue)
    {
        SendPacket (node);
    }
}

// -------------------------------------------------------------------------
// SendOS()
// 
// Sends an ordered set of a given Type. Not queued with TLPs,
// and will send immediately.
//
// -------------------------------------------------------------------------

void SendOs (const int Type, const int node) 
{

    int lanes, sequence;
    int old_draining_state = this->draining_queue;
    uint32_t LinkIn [MAX_LINK_WIDTH];

    if (node < 0 || node > VP_MAX_NODES)
    {
        VPrint("SendOs: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendOs: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (Type != IDL && Type != SKP && Type != FTS)
    {
        VPrint( "SendOs: ***Error --- invalid ordered set(%03x) at node %d\n", Type, node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (Type == SKP)
    {
        //this->LastTxSkipTime = GetCycleCount(node);
        this->SkipScheduled -= this->SkipScheduled ? 1 : 0;
    }

    // Make sure RX queues, rather than sends, any transmitted replies
    this->draining_queue = true;

    for (sequence = 0; sequence < OS_LENGTH; sequence++)
    {
        for (lanes = 0; lanes < this->LinkWidth; lanes++) 
        {
            LinkIn[lanes] = VWrite(LINKADDR0+lanes, Encode((sequence == 0) ? COM : Type, false, lanes, this->LinkWidth, node), lanes != this->LinkWidth-1, node);
        }
        ExtractPhyInput(this, LinkIn);
    }

    this->draining_queue = old_draining_state;
}

// -------------------------------------------------------------------------
// SendTs()
//
// Sends a training sequence with given parameters. Identifier
// must be either TS1_ID or TS2_ID. lane_num and link_num can
// be PAD or a valid number. n_fts indicates the required number
// of fast training sequence ordered sets, and control is the
// four bit value for the control field (so, look it up!)
//
// -------------------------------------------------------------------------

void SendTs(const int identifier, const int lane_num, const int link_num, const int n_fts, const int control, const bool is_gen2, const int node) 
{
    int lanes, sequence;
    int data;
    int old_draining_state = this->draining_queue;
    uint32_t LinkIn [MAX_LINK_WIDTH];

    // Do some checks
    if (node < 0 || node > VP_MAX_NODES)
    {
        VPrint("SendTs: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendTs: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (identifier != TS1_ID && identifier != TS2_ID)
    {
        VPrint( "SendTs: ***Error --- bad identifier (%02x) at node %d\n", identifier, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if (link_num != PAD && (link_num < 0 || link_num > TS_LINK_NUM_MAX_VALUE))
    {
        VPrint( "SendTs: ***Error --- bad link number (%02x) at node %d\n", link_num, node);
        VWrite(PVH_FATAL, 0, 0, node);
    } 
    else if (n_fts < 0 || n_fts > TS_N_FTS_MAX_VALUE)
    {
        VPrint( "SendTs: ***Error --- bad N FTS (%02x) at node %d\n", n_fts, node);
        VWrite(PVH_FATAL, 0, 0, node);
    }
    else if (control < 0 || control > TS_CNTL_MAX_VALUE)
    {
        VPrint( "SendTs: ***Error --- bad control (%02x) at node %d\n", control, node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    // Make sure RX queues, rather than sends, any transmitted replies
    this->draining_queue = true;

    for (sequence = 0; sequence < TS_LENGTH; sequence++)
    {
        for (lanes = 0; lanes < this->LinkWidth; lanes++)
        {
            if (sequence == TS_COMMA_SEQ)
            {
                data = COM;
            }
            else if (sequence == TS_LINK_NUM_SEQ)
            {
                data = (link_num == PAD) ? PAD : link_num;
            }
            else if (sequence == TS_LANE_NUM_SEQ)
            {
                data = (lane_num == PAD) ? PAD : lanes;
            }
            else if (sequence == TS_N_FTS_SEQ)
            {
                data = n_fts;
            }
            else if (sequence == TS_DATA_RATE_SEQ)
            {
                data = is_gen2 ? TS_DATA_RATE_GEN2 : TS_DATA_RATE_GEN1;
            }
            else if (sequence == TS_CONTROL_SEQ)
            {
                data = control;
            }
            else
            {
                data = identifier;
            }

            LinkIn[lanes] = VWrite(LINKADDR0+lanes, Encode(data, true, lanes, this->LinkWidth, node), lanes != this->LinkWidth-1, node);
        }
        ExtractPhyInput(this, LinkIn);
    }

    this->draining_queue = old_draining_state;
}

// -------------------------------------------------------------------------
// SendIdle()
//
// Send idle data for at least Ticks number of symbol times.
//
// -------------------------------------------------------------------------

void SendIdle(const int Ticks, const int node)
{
    int lanes, target_time = (Ticks + this->TicksSinceReset);

    DebugVPrint("** Entering SendIdle\n");

    if (node < 0 || node > VP_MAX_NODES)
    {
        VPrint("SendIdle: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("SendIdle: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (Ticks < 1)
    {
        VPrint("SendIdle: ***Error --- Ticks argument (%d) cannot be less than 1 at node %d\n", Ticks, node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    while (target_time > this->TicksSinceReset)
    {
        SendPacket(node);
    }

    DebugVPrint("** Exiting SendIdle \n");
}

// -------------------------------------------------------------------------
// WaitForCompletion()
//
// Waits for a completion (CompletionEvent non zero). If
// CompletionEvent already non-zero WaitForCompletion
// drops through, decrementing the CompletionEvent counter.
// If CompletionEvent is <= 0, WaitForCompletion loops,
// still extracting input data and calling SendPacket()
// if send_p not NULL, as completion packets may be
// being generated by ProcessInput().
//
// -------------------------------------------------------------------------

void WaitForCompletionN (const uint32_t count, const int node)
{
    int lanes;

    if (node < 0 || node > VP_MAX_NODES)
    {
        VPrint("WaitForCompletionN: ***Error --- Invalid node %d\n", node);
        exit(EXIT_FAILURE);
    }

    if (pms == NULL || this == NULL)
    {
        VPrint("WaitForCompletionN: ***Error --- Called before initialisation. Call InitialisePcie() first from node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (count > this->OutstandingCompletions)
    {
        DebugVPrint("WaitForCompletionN: Warning --- waiting on more completions than outstanding (%d v %d) at node %d\n", count, this->OutstandingCompletions, node);
        DebugVPrint("                    Simulation may hang at this point.\n");
    }

    while (this->CompletionEvent < count)
    {
        SendIdle(1, node);
    }

    this->CompletionEvent -= count;
}

void WaitForCompletion(int node)
{
    WaitForCompletionN (1, node);
}

// -------------------------------------------------------------------------
// InitialisePcie()
//
// Initialisation of Pcie routines. First argument is 
// a pointer to user supplied callback function, called
// whenever a completion packet is not caught and processed 
// internally. The second argument is the number of the 
// virtual processor we're are running on. InitialisePcie()
// *must* be called before attempting to generate Pcie 
// traffic. Note that, currently, only one virtual processor
// can run PCIe code.
//
// -------------------------------------------------------------------------

void InitialisePcie (const callback_t cb_func, void *usrptr, const int node)
{
    uint32_t linkwidth, i;

    VPrint("InitialisePcie() called from node %d\n", node);

    if (node < 0 || node >= VP_MAX_NODES)
    {
        VPrint( "InitialisePcie: ***Error --- Invalid node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if (pms == NULL)
    {
        if ((pms = malloc(sizeof(pPcieModelState_t) * VP_MAX_NODES)) == NULL)
        {
            VPrint( "InitialisePcie: ***Error --- memory allocation failed at node %d\n", node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        for (i = 0; i < VP_MAX_NODES; i++)
        {
            pms[i] = NULL;
        }
    }

    if (this != NULL)
    {
        VPrint( "InitialisePcie: ***Error --- InitialisePcie() already called for node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    if ((this = malloc(sizeof(PcieModelState_t))) == NULL)
    {
        VPrint( "InitialisePcie: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    InitPcieState(this, node);
    InitialiseMem(node);

    // Sync clock counter to simulation
    VRead(CLK_COUNT, &(this->TicksSinceReset), true, node);

    this->vuser_cb = cb_func;
    this->usrptr   = usrptr;

    VRead(LANESADDR, &linkwidth, true, node);
    VRead(EP_ADDR, &(this->Endpoint), true, node);

    if (linkwidth == 1 || linkwidth == 2 || linkwidth == 4 || linkwidth == 8 || linkwidth == 12 || linkwidth == 16)
    {
        VPrint( "InitialisePcie: Info --- valid linkwidth (%d) at node %d\n", linkwidth, node);
        this->LinkWidth = linkwidth;
    } 
    else 
    {
        VPrint( "InitialisePcie: ***Error --- invalid linkwidth (%d) at node %d\n", linkwidth, node);
        VWrite(PVH_FATAL, 0, 0, node);
    }
}

// -------------------------------------------------------------------------
// RegisterOsCallback()
//
// -------------------------------------------------------------------------

void RegisterOsCallback (const os_callback_t cb_func, const int node)
{
    this->vuser_os_cb = cb_func;
}

// -------------------------------------------------------------------------
// ResetEventCount()
//
// Clear count for each lane for give OS/TS type. Returns
// -1 if type is bad, otherwise 0.
//
// -------------------------------------------------------------------------

int ResetEventCount(const int type, const int node)
{
    int i;
    
    if (type == 0)
    {
        for (i = 0; i < MAX_LINK_WIDTH; i++)
        {
            this->linkevent.FlaggedIdle[i] = 0;
        }
    }
    else if (type == IDL)
    {
        for (i = 0; i < MAX_LINK_WIDTH; i++)
        {
            this->linkevent.IdleCount[i] = 0;
        }
    }
    else if (type == SKP)
    {
        for (i = 0; i < MAX_LINK_WIDTH; i++)
        {
            this->linkevent.SkipCount[i] = 0;
        }
    }
    else if (type == FTS)
    {
        for (i = 0; i < MAX_LINK_WIDTH; i++)
        {
            this->linkevent.FtsCount[i] = 0;
        }
    }
    else if (type == TS1_ID)
    {
        for (i = 0; i < MAX_LINK_WIDTH; i++)
        {
            this->linkevent.Ts1Count[i] = 0;
        }
    } 
    else if (type == TS2_ID)
    {
        for (i = 0; i < MAX_LINK_WIDTH; i++)
        {
            this->linkevent.Ts2Count[i] = 0;
        }
    }
    else
    {
        VPrint("ResetEvent: ***Error --- invalid type (%d) at node %d\n", type, node);
        return -1;
    }

    return 0;
}

// -------------------------------------------------------------------------
// ReadEventCount()
//
// Return the counts for all lanes for given OS/TS type
// into ts_data. Returns -1 if type is bad, otherwise 0.
//
// -------------------------------------------------------------------------

int ReadEventCount (const int type, uint32_t *ts_data, const int node)
{
    int i;
    uint32_t * ptr;

    if (type == 0)
    {
	ptr = this->linkevent.FlaggedIdle;
    }
    else if (type == IDL)
    {
        ptr = this->linkevent.IdleCount;
    }
    else if (type == SKP)
    {
        ptr = this->linkevent.SkipCount;
    }
    else if (type == FTS)
    {
        ptr = this->linkevent.FtsCount;
    }
    else if (type == TS1_ID)
    {
        ptr = this->linkevent.Ts1Count;
    } 
    else if (type == TS2_ID)
    {
        ptr = this->linkevent.Ts2Count;
    }
    else
    {
        VPrint("ReadEventCount: ***Error --- invalid type (%d) at node %d\n", type, node);
        return -1;
    }

    for (i = 0; i < MAX_LINK_WIDTH; i++)
    {
        ts_data[i] = ptr[i];
    }

    return 0;
}

// -------------------------------------------------------------------------
// GetTS()
//
// Returns a training sequence type (TS_t) which is the
// last TS value seen by ProcessOS().
//
// -------------------------------------------------------------------------

TS_t GetTS(const int lane, const int node)
{
    return this->linkevent.LastTS[lane];
}

// -------------------------------------------------------------------------
// GetCycleCount()
//
// Return number of cycles since begining
//
// -------------------------------------------------------------------------

uint32_t GetCycleCount (const int node)
{
    return this->TicksSinceReset;
}

// -------------------------------------------------------------------------
// InitFc()
//
// User initiation function for flow control initialisation
//
// -------------------------------------------------------------------------

void InitFc (const int node)
{
    TxFcInitInt (&this->flwcntl, &this->usrconf, node);
}

// -------------------------------------------------------------------------
// ConfigurePcie()
//
// Update internal PCIe model configuration state.
//
// -------------------------------------------------------------------------

void ConfigurePcie (const int type, const int value, const int node)
{
    pUserConfig_t usrconf = &(this->usrconf);
    pFlowControl_t flw = &(this->flwcntl);
    ConfigLinkInit_t ltssm_cfg;
    bool ltssm_cfg_updated = false;

    INIT_CFG_LINK_STRUCT(ltssm_cfg);

    switch (type)
    {
    case CONFIG_FC_HDR_RATE:
        if (value < 0) 
        {   
            VPrint("ConfigurePcie: ***Error --- bad config value at node %d\n", node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        usrconf->HdrConsumptionRate = value;
        break;

    case CONFIG_FC_DATA_RATE:
        if (value < 0)
        {
            VPrint("ConfigurePcie: ***Error --- bad config value at node %d\n", node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        usrconf->DataConsumptionRate = value;
        break;

    case CONFIG_ENABLE_FC:
    case CONFIG_DISABLE_FC:
        usrconf->DisableFc = type & 0x1;
        break;

    case CONFIG_ENABLE_ACK:
    case CONFIG_DISABLE_ACK:
        usrconf->DisableAck = type & 0x1;
        if (type == CONFIG_ENABLE_ACK)
        {
            if (value < 1)
            {
                VPrint("ConfigurePcie: ***Error --- an ACK rate of %d is a bit daft, at node %d\n", value, node);
                VWrite(PVH_FATAL, 0, 0, node);
            } 
            else
            {
                usrconf->AckRate = value;
            }
        }
        break;

    case CONFIG_ENABLE_MEM:
    case CONFIG_DISABLE_MEM:
        usrconf->DisableMem = type & 0x1;
        break;

    case CONFIG_ENABLE_SKIPS:
    case CONFIG_DISABLE_SKIPS:
        usrconf->DisableSkips = type & 0x1;
        if (type == CONFIG_ENABLE_SKIPS)
        {
            if (value < MINIMUM_SKIP_INTERVAL)
            {
                VPrint("ConfigurePcie: ***Error --- a skip interval of %d is a bit daft, at node %d\n", value, node);
                VWrite(PVH_FATAL, 0, 0, node);
            }
            else
            {
                usrconf->SkipInterval = value;
            }
        }
        break;

    case CONFIG_ENABLE_UR_CPL:
    case CONFIG_DISABLE_UR_CPL:
        usrconf->DisableUrCpl = type & 0x1;
        break;

    case CONFIG_POST_HDR_CR:
        if (value > MAX_HDR_CREDITS)
        {
            VPrint("ConfigurePcie: ***Error --- post header credits of %d too big at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            if (flw->ConsumedHdrCredits[0][FC_POST] != usrconf->InitFcHdrCr[0][FC_POST] || flw->AdvertisedHdrCredits[0][FC_POST] != usrconf->InitFcHdrCr[0][FC_POST])
            {
                DebugVPrint("ConfigurePcie: ***Warning --- posted header FC init value altered after active flow control. May by out of sync with other end of link at node %d\n", node);
            }
            usrconf->InitFcHdrCr[0][FC_POST] = value;
            flw->ConsumedHdrCredits[0][FC_POST] = value;
            flw->AdvertisedHdrCredits[0][FC_POST] = value;
        }
        break;

    case CONFIG_NONPOST_HDR_CR:
        if (value > MAX_HDR_CREDITS)
        {
            VPrint("ConfigurePcie: ***Error --- non-post header credits of %d too big at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            if (flw->ConsumedHdrCredits[0][FC_NONPOST] != usrconf->InitFcHdrCr[0][FC_NONPOST] || flw->AdvertisedHdrCredits[0][FC_NONPOST] != usrconf->InitFcHdrCr[0][FC_NONPOST])
            {
                DebugVPrint("ConfigurePcie: ***Warning --- non-posted header FC init value altered after active flow control. May by out of sync with other end of link at node %d\n", node);
            }
            usrconf->InitFcHdrCr[0][FC_NONPOST] = value;
            flw->ConsumedHdrCredits[0][FC_NONPOST] = value;
            flw->AdvertisedHdrCredits[0][FC_NONPOST] = value;
        }
        break;

    case CONFIG_CPL_HDR_CR:
        if (value > MAX_HDR_CREDITS)
        {
            VPrint("ConfigurePcie: ***Error --- completion header credits of %d too big at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            if (flw->ConsumedHdrCredits[0][FC_CMPL] != usrconf->InitFcHdrCr[0][FC_CMPL] || flw->AdvertisedHdrCredits[0][FC_CMPL] != usrconf->InitFcHdrCr[0][FC_CMPL]) 
            {    
                DebugVPrint("ConfigurePcie: ***Warning --- completion header FC init value altered after active flow control. May by out of sync with other end of link at node %d\n", node);
            }
            usrconf->InitFcHdrCr[0][FC_CMPL] = value;
            flw->ConsumedHdrCredits[0][FC_CMPL] = value;
            flw->AdvertisedHdrCredits[0][FC_CMPL] = value;
        }
        break;

    case CONFIG_POST_DATA_CR:
        if (value > MAX_DATA_CREDITS)
        {
            DebugVPrint("ConfigurePcie: ***Error --- posted data credits of %d too big at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            if (flw->ConsumedDataCredits[0][FC_POST] != usrconf->InitFcDataCr[0][FC_POST] || flw->AdvertisedDataCredits[0][FC_POST] != usrconf->InitFcDataCr[0][FC_POST]) 
            {    
                DebugVPrint("ConfigurePcie: ***Warning --- posted data FC init value altered after active flow control. May by out of sync with other end of link at node %d\n", node);
            }
            usrconf->InitFcDataCr[0][FC_POST] = value;
            flw->ConsumedDataCredits[0][FC_POST] = value;
            flw->AdvertisedDataCredits[0][FC_POST] = value;
        }
        break;

    case CONFIG_NONPOST_DATA_CR:
        if (value > MAX_DATA_CREDITS)
        {
            VPrint("ConfigurePcie: ***Error --- non-posted data credits of %d too big at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            if (flw->ConsumedDataCredits[0][FC_NONPOST] != usrconf->InitFcDataCr[0][FC_NONPOST] || flw->AdvertisedDataCredits[0][FC_NONPOST] != usrconf->InitFcDataCr[0][FC_NONPOST]) 
            {    
                DebugVPrint("ConfigurePcie: ***Warning --- non-posted data FC init value altered after active flow control. May by out of sync with other end of link at node %d\n", node);
            }
            usrconf->InitFcDataCr[0][FC_NONPOST] = value;
            flw->ConsumedDataCredits[0][FC_NONPOST] = value;
            flw->AdvertisedDataCredits[0][FC_NONPOST] = value;
        }
        break;

    case CONFIG_CPL_DATA_CR:
        if (value > MAX_DATA_CREDITS)
        {
            VPrint("ConfigurePcie: ***Error --- posted data credits of %d too big at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            if (flw->ConsumedDataCredits[0][FC_CMPL] != usrconf->InitFcDataCr[0][FC_CMPL] || flw->AdvertisedDataCredits[0][FC_CMPL] != usrconf->InitFcDataCr[0][FC_CMPL]) 
            {
                DebugVPrint("ConfigurePcie: ***Warning --- completion data FC init value altered after active flow control. May by out of sync with other end of link at node %d\n", node);
            }
            usrconf->InitFcDataCr[0][FC_CMPL] = value;
            flw->ConsumedDataCredits[0][FC_CMPL] = value;
            flw->AdvertisedDataCredits[0][FC_CMPL] = value;
        }
        break;

    case CONFIG_CPL_DELAY_RATE:
        if (value < 0)
        {
            VPrint("ConfigurePcie: ***Error --- Completion delay value of %d invalid at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            usrconf->CompletionRate = value;
        }
        break;

    case CONFIG_CPL_DELAY_SPREAD:
        if (value < 0)
        {
            VPrint("ConfigurePcie: ***Error --- Completion spread value of %d invalid at node %d\n", value, node);
            VWrite(PVH_FATAL, 0, 0, node);
        }
        else
        {
            usrconf->CompletionSpread = value;
        }
        break;

    case CONFIG_LTSSM_LINKNUM:
	ltssm_cfg.ltssm_linknum = value;
	ltssm_cfg_updated = true;
	break;

    case CONFIG_LTSSM_N_FTS:
	ltssm_cfg.ltssm_n_fts = value;
	ltssm_cfg_updated = true;
	break;

    case CONFIG_LTSSM_TS_CTL:
	ltssm_cfg.ltssm_ts_ctl = value;
	ltssm_cfg_updated = true;
	break;

    case CONFIG_LTSSM_DETECT_QUIET_TO:
	ltssm_cfg.ltssm_detect_quiet_to = value;
	ltssm_cfg_updated = true;
	break;

    case CONFIG_LTSSM_ENABLE_TESTS:
	ltssm_cfg.ltssm_enable_tests = value;
	ltssm_cfg_updated = true;
	break;

    case CONFIG_LTSSM_FORCE_TESTS:
	ltssm_cfg.ltssm_force_tests = value;
	ltssm_cfg_updated = true;
	break;

    default:
        VPrint("ConfigurePcie: ***Error --- bad config type at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
        break;
    }

    if (ltssm_cfg_updated)
    {
        ConfigLinkInit(ltssm_cfg, node);
    }
}

// -------------------------------------------------------------------------
// PcieRand()
//
// -------------------------------------------------------------------------

uint32_t PcieRand (const int node)
{
    this->RandNum = CalcNewRand(this->RandNum);   
    return this->RandNum;
}

// -------------------------------------------------------------------------
// PcieSeed()
//
// -------------------------------------------------------------------------

void PcieSeed (const uint32_t seed, const int node)
{
    this->RandNum = seed;
}

// -------------------------------------------------------------------------
// SetTxDisabled()
//
// -------------------------------------------------------------------------

void SetTxDisabled (const int node)
{
    this->tx_disabled = true;
}

// -------------------------------------------------------------------------
// SetTxEnabled()
//
// -------------------------------------------------------------------------

void SetTxEnabled (const int node)
{
    this->tx_disabled = false;
}

// Allow reuse in other files
#undef this
