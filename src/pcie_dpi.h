//=============================================================
//
// Copyright (c) 2023 Simon Southwell. All rights reserved.
//
// Date: 8th Mar 2023
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

#ifndef _PCIE_DPI_H_
#define _PCIE_DPI_H_


#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#define __USE_GNU
#include <dlfcn.h>
#undef __USE_GNU
#include <pthread.h>

// For file IO
#include <fcntl.h>

// For inode manipulation
#include <unistd.h>

#include "svdpi.h"

#define VP_MAX_NODES            2

#define V_IDLE                  0
#define V_WRITE                 1
#define V_READ                  2
#define V_HALT                  4
#define V_SWAP                  8

#define DEFAULT_STR_BUF_SIZE    32

#define MIN_INTERRUPT_LEVEL     1
#define MAX_INTERRUPT_LEVEL     7

#define MONITOR_SEG_SIZE        4096

#define DELTA_CYCLE             -1
#define GO_TO_SLEEP             0x7fffffff

#define MAX_INT_LEVEL           7
#define MIN_INT_LEVEL           1

typedef struct {
    unsigned int        addr;
    unsigned int        data_out;
    unsigned int        rw;
    int                 ticks;
} send_buf_t, *psend_buf_t;

typedef struct {
    unsigned int        data_in;
} rcv_buf_t, *prcv_buf_t;


// Shared object handle type
typedef void * handle_t;

typedef struct {
    send_buf_t          send_buf;
    rcv_buf_t           rcv_buf;
} SchedState_t, *pSchedState_t;

// Pointer to pthread_create compatible function
typedef void *(*pThreadFunc_t)(void *);

// Pointer to VUserMain function type definition
typedef void (*pVUserMain_t)(void);

extern pSchedState_t ns[VP_MAX_NODES];

extern int notReset;

// VUser function prototypes
extern int  VWrite        (unsigned int addr,  unsigned int  data, int delta, unsigned int node);
extern int  VRead         (unsigned int addr,  unsigned int *data, int delta, unsigned int node);
extern int  VTick         (unsigned int ticks, unsigned int node);

# define VPrint(...) printf (__VA_ARGS__)

#ifdef DEBUG
#define DebugVPrint VPrint
#else
#define DebugVPrint(...) {}
#endif

#endif
