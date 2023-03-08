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
// $Id: codec.c,v 1.4 2016/10/10 13:07:51 simon Exp $
// $Source: /home/simon/CVS/src/HDL/pcieVHost/src/codec.c,v $
//
//=============================================================

// -------------------------------------------------------------------------
// INCLUDES
// -------------------------------------------------------------------------

#include "codec.h"
#include "pci_express.h"
#include "pcie_vhost_map.h"

// -------------------------------------------------------------------------
// STATICS
// -------------------------------------------------------------------------

static pCodecState_t CodecState [VP_MAX_NODES];

// -------------------------------------------------------------------------
// DEFINES
// -------------------------------------------------------------------------

#define this CodecState[node]

// -------------------------------------------------------------------------
// CONSTANTS
// -------------------------------------------------------------------------

// NegTable5 and PosTable5 have equal values when disparity is 0,
// other entries are inverse, except at index 7 where they
// are bit reversed

static const TblType NegTable5[32] = {
    {0x39,  2}, {0x2e,  2}, {0x2d,  2}, {0x23,  0},
    {0x2b,  2}, {0x25,  0}, {0x26,  0}, {0x07,  0},
    {0x27,  2}, {0x29,  0}, {0x2a,  0}, {0x0b,  0},
    {0x2c,  0}, {0x0d,  0}, {0x0e,  0}, {0x3a,  2},
    {0x36,  2}, {0x31,  0}, {0x32,  0}, {0x13,  0},
    {0x34,  0}, {0x15,  0}, {0x16,  0}, {0x17,  2},
    {0x33,  2}, {0x19,  0}, {0x1a,  0}, {0x1b,  2},
    {0x1c,  0}, {0x1d,  2}, {0x1e,  2}, {0x35,  2},
};

static const TblType PosTable5[32] = {
    {0x06, -2}, {0x11, -2}, {0x12, -2}, {0x23,  0},
    {0x14, -2}, {0x25,  0}, {0x26,  0}, {0x38,  0},
    {0x18, -2}, {0x29,  0}, {0x2a,  0}, {0x0b,  0},
    {0x2c,  0}, {0x0d,  0}, {0x0e,  0}, {0x05, -2},
    {0x09, -2}, {0x31,  0}, {0x32,  0}, {0x13,  0},
    {0x34,  0}, {0x15,  0}, {0x16,  0}, {0x28, -2},
    {0x0c, -2}, {0x19,  0}, {0x1a,  0}, {0x24, -2},
    {0x1c,  0}, {0x22, -2}, {0x21, -2}, {0x0a, -2},
}; 

// NegTable3 and PosTable3 have equal values when disparity is 0,
// otherwise entries are inverse. 

static const TblType NegTable3[8] = {
    {0x2, -2}, {0x9,  0}, {0xa,  0}, {0xc,  0},
    {0x4, -2}, {0x5,  0}, {0x6,  0}, {0x8, -2},
};

static const TblType PosTable3[8] = {
    {0xd, 2}, {0x9, 0}, {0xa, 0}, {0x3, 0},
    {0xb, 2}, {0x5, 0}, {0x6, 0}, {0x7, 2},
};

// Tables for control codes
static const TblType K_NegTable5[12] = {
    {0x3c, 2}, {0x3c, 2}, {0x3c, 2}, {0x3c, 2},
    {0x3c, 2}, {0x3c, 2}, {0x3c, 2}, {0x3c, 2},
    {0x17, 2}, {0x1b, 2}, {0x1d, 2}, {0x1e, 2},
};

static const TblType K_PosTable5[12] = {
    {0x03, -2}, {0x03, -2}, {0x03, -2}, {0x03, -2},
    {0x03, -2}, {0x03, -2}, {0x03, -2}, {0x03, -2},
    {0x28, -2}, {0x24, -2}, {0x22, -2}, {0x21, -2},
};

static const TblType K_NegTable3[12] = {
    {0x2, -2}, {0x9,  0}, {0xa,  0}, {0xc,  0},
    {0x4, -2}, {0x5,  0}, {0x6,  0}, {0x1, -2},
    {0x1, -2}, {0x1, -2}, {0x1, -2}, {0x1, -2},
};

static const TblType K_PosTable3[12] = {
    {0xd,  2}, {0x6,  0}, {0x5,  0}, {0x3,  0},
    {0xb,  2}, {0xa,  0}, {0x9,  0}, {0xe,  2},
    {0xe,  2}, {0xe,  2}, {0xe,  2}, {0xe,  2},
};

static const unsigned int Bitrev4 [16] = 
                                   {0x0, 0x8, 0x4, 0xc, 
                                    0x2, 0xa, 0x6, 0xe, 
                                    0x1, 0x9, 0x5, 0xd, 
                                    0x3, 0xb, 0x7, 0xf};


static const unsigned int NumOfOnes4 [16] = 
                                      {0, 1, 1, 2,
                                       1, 2, 2, 3,
                                       1, 2, 2, 3,
                                       2, 3, 3, 4};

unsigned const int Bitrev8 [256] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8, 
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc, 
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6, 
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1, 
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd, 
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb, 
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};

// -------------------------------------------------------------------------
// ScrambleAdvance()
//
// Advance scrambler by one
//
// -------------------------------------------------------------------------

static void ScrambleAdvance(const int reset, uint32_t* const lfsr)
{
    uint32_t newlfsr;

    if (reset)
    {
        *lfsr = DEFAULTLFSRVALUE;
        return;
    }

    newlfsr = ((*lfsr & 0xff) << 8) | ((*lfsr & 0xff00) >> 8);
    newlfsr ^= (*lfsr & 0xff00) >> 5;
    newlfsr ^= (*lfsr & 0xff00) >> 4;
    newlfsr ^= (*lfsr & 0xff00) >> 3;

    *lfsr = newlfsr;
}

// -------------------------------------------------------------------------
// Encode()
//
// Do 8b/10b encoding
//
// -------------------------------------------------------------------------

unsigned int Encode(const int data, const int no_scramble, const int lane, const int linkwidth, const int node)
{
    unsigned int code, code3, code5, c, tblidx; 
    const TblType *tbl5, *tbl3;

    if (!no_scramble && data <= 0xff)
    {
        c = data ^ Bitrev8[(this->elfsr & 0xff00) >> 8];
    }
    else
    {
        c = data;
    }

    // Advance scrambler unless a SKIP, or reset if COMMA
    if (data != SKP && lane == (linkwidth-1))
    {
        ScrambleAdvance(data == COM, &(this->elfsr));
    }

    // For data bytes
    if (c < 256)
    {
        // Pick 5 bit table based on running disparity
        tbl5 = (this->rd[lane] == -1) ? NegTable5 : PosTable5;

        // 3 bit table based in the disparity of the 5 bit table and current 
        // running disparity. If total Positive, choose NegTable3 else PosTable3.
        tbl3 = (tbl5[c & 0x1f].disparity == (this->rd[lane] == 1 ? 0 : 2)) ? NegTable3 : PosTable3;

        // Extract codes
        code5 = tbl5[c & 0x1f].code;
        code3 = tbl3[c >> 5].code ; 

        // Bit reverse 3b/4b code to avoid a run of 5 bits
        if (((code5 & 0x30) == 0x30 && (code3 & 0x7) == 0x7) || ((code5 & 0x30) == 0x0 && (code3 & 0x7) == 0x0)) 
        {
            code3 = Bitrev4[code3];
        }

        // Calculate new running disparity
        this->rd[lane] += tbl5[c & 0x1f].disparity  +  tbl3[c >> 5].disparity;

    // For control codes
    }
    else
    {
        // K28.0 to K28.7
        if ((c & 0xf) == 0xc)
        {
            tblidx = (c >> 5) & 0x7;
        }
        else 
        {
            tblidx = (c == 0x1f7) ? 8 :
                     (c == 0x1fb) ? 9 :
                     (c == 0x1fd) ? 10 :
                                    11;
        }

        tbl5 = (this->rd[lane] == -1) ? K_NegTable5 : K_PosTable5;

        tbl3 = (tbl5[tblidx].disparity == (this->rd[lane] == 1 ? 0 : 2)) ? K_NegTable3 : K_PosTable3;

        code5 = tbl5[tblidx].code;
        code3 = tbl3[tblidx].code;

        // Calculate new running disparity
        this->rd[lane] += tbl5[tblidx].disparity + tbl3[tblidx].disparity;
    }

    // Construct 10 bit code
    code = code3 << 6 | code5;

    return code;
}

// -------------------------------------------------------------------------
// Decode()
//
// Do 8b/10b decoding
//
// -------------------------------------------------------------------------

unsigned int Decode (const int data, const int no_scramble, const int lane, const int linkwidth, const int node)
{

    int TwoOnes, ThreeZeros, ThreeOnes;
    int Invert21, Invert430, Invert4, Invert3210;
    int Invert31, Invert420, Invert42, Invert310, InvertAll5;
    int InvertMask5, InvertAll3, InvertMask3;
    int LowBits, Hibits, Control, Raw;

    // Run length flags of input first 4 bits
    TwoOnes    = NumOfOnes4[data&0xf] == 2;
    ThreeZeros = NumOfOnes4[data&0xf] == 1;
    ThreeOnes  = NumOfOnes4[data&0xf] == 3;

    // Calculate bit inversion requirements for 6b/5b code
    Invert21   = (TwoOnes     && Data2and1 && Data5xnor4);
    Invert430  = (TwoOnes     && Data2nor1 && Data5xnor4);
    Invert4    = (ThreeZeros  && !Data5);
    Invert3210 = (ThreeOnes   &&  Data5);
    Invert31   = (TwoOnes     &&  Data0 &&  Data2 && Data5xnor4);
    Invert420  = (TwoOnes     && !Data0 && !Data2 && Data5xnor4);
    Invert42   = (Data1nor0   && Data5nor4);
    Invert310  = (Data1and0   && Data5and4);
    InvertAll5 = ((ThreeZeros && (Data5to3and || !Data4)) || Data5to2nor);

    // Create an inversion mask for 6b/5b code
    InvertMask5 = ((Invert430 | Invert4    | Invert420 | Invert42  | InvertAll5) << 4) | 
                  ((Invert430 | Invert3210 | Invert31  | Invert310 | InvertAll5) << 3) |
                  ((Invert21  | Invert3210 | Invert420 | Invert42  | InvertAll5) << 2) |
                  ((Invert21  | Invert3210 | Invert31  | Invert310 | InvertAll5) << 1) |
                  ((Invert430 | Invert3210 | Invert420 | Invert310 | InvertAll5) << 0); 

    // Low order 5 bits
    LowBits =  (data & 0x1f) ^ InvertMask5;

    // Flag condition where all 3 bits need inverting
    InvertAll3 = (Data5to2nor && Data9xor8) || (Data7nor6 && Data9and8) || (Data7and6 && Data9) || (Data8to6nor);

    // Create an inversion mask for 4b/3b code
    InvertMask3 = (
                   (((Data9and8 &&  Data6) || Data9to7nor) << 2) |
                   (((Data9nor8 && !Data6) || Data9to7nor) << 1) |
                   (((Data9and8 &&  Data6) || Data9to7and)     )) | (InvertAll3 ? 0x07 : 0);

    // High order 3 bits
    Hibits = ((data >> 6) & 0x7) ^ InvertMask3;


    // Output is a control code, not a data byte
    Control = (Data5to2and) || (Data5to2nor) || (ThreeZeros && !Data4 && Data5 && Data9to7and) || (ThreeOnes && Data4 && !Data5 & Data9to7nor);

    Raw = (Control << 8) | (Hibits << 5) | LowBits;

    // Scrambling

    // Decide if we're receiving a training sequence 
    if (lane == 0)
    {
        if (this->last_lane0_sym == COM && (Raw == PAD || !Control))
        {
            this->ts_active = true;
        }
        else if (this->ts_active && (this->last_lane0_sym == TS1_ID || this->last_lane0_sym == TS2_ID) && Raw != this->last_lane0_sym)
        {
            this->ts_active = false;
        }
    }
    this->last_lane0_sym = Raw;

    if (!this->ts_active && !no_scramble && Raw <= 0xff)
    {        
        Raw = Raw ^ Bitrev8[(this->dlfsr & 0xff00) >> 8];
    }

    // Advance scrambler unless a SKIP, or reset if COMMA
    if (Raw != SKP && lane == (linkwidth-1))
    {
        ScrambleAdvance(Raw == COM, &(this->dlfsr));
    }

    return Raw;
}

// -------------------------------------------------------------------------
// PciCrc()
//
// Calculate CRC after next 'Bits' shifted
//
// -------------------------------------------------------------------------

uint32_t PciCrc (uint32_t Data, uint32_t CrcIn, uint32_t Bits, uint32_t poly, uint32_t crcsize)
{
    uint32_t Crc = CrcIn, topbit;
    int i;

    topbit = MAX_CRC_TOP_BIT >> (MAXCRCSIZE - crcsize);

    for (i = 0; i < Bits; i++)
    {
        Crc = (Crc << 1UL) ^ ((((Crc & topbit) ? 1 : 0) ^ ((Data >> i) & 1)) ? poly : 0);
    }

    return Crc;
}

// -------------------------------------------------------------------------
// InitCodec()
//
// Initialise the codec state for a particular node.
//
// -------------------------------------------------------------------------

void InitCodec (const int node)
{
    int idx;

    if ((this = malloc(sizeof(CodecState_t))) == NULL)
    {
        VPrint( "InitCodec: ***Error --- memory allocation failed at node %d\n", node);
        VWrite(PVH_FATAL, 0, 0, node);
    }

    for (idx = 0; idx < MAX_LINK_WIDTH; idx++)
    {
        this->rd[idx]    = 1;
    }
    
    this->elfsr = DEFAULTLFSRVALUE;
    this->dlfsr = DEFAULTLFSRVALUE;

    this->ts_active = 0;
    this->last_lane0_sym = 0;
}

// Allow reuse in other files
#undef this
