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
//=============================================================

#include <stdint.h>

#ifdef __cplusplus
#define EXTERN extern "C"
#else
#define EXTERN extern
#endif

#ifndef _LTSSM_H_
#define _LTSSM_H_

#define LINK_INIT_NO_CHANGE  (-1)

typedef struct
{
    int ltssm_linknum;
    int ltssm_n_fts;
    int ltssm_ts_ctl;
    int ltssm_detect_quiet_to;
    int ltssm_enable_tests;
    int ltssm_force_tests;
    int ltssm_poll_active_tx_count;

} ConfigLinkInit_t;

#define INIT_CFG_LINK_STRUCT(_cfg) {                  \
  (_cfg).ltssm_linknum              = LINK_INIT_NO_CHANGE; \
  (_cfg).ltssm_n_fts                = LINK_INIT_NO_CHANGE; \
  (_cfg).ltssm_ts_ctl               = LINK_INIT_NO_CHANGE; \
  (_cfg).ltssm_detect_quiet_to      = LINK_INIT_NO_CHANGE; \
  (_cfg).ltssm_enable_tests         = LINK_INIT_NO_CHANGE; \
  (_cfg).ltssm_force_tests          = LINK_INIT_NO_CHANGE; \
  (_cfg).ltssm_poll_active_tx_count = LINK_INIT_NO_CHANGE; \
}

// Link initialisation
EXTERN void       InitLink             (const int linkwidth,        const int node);
EXTERN void       ConfigLinkInit       (const ConfigLinkInit_t cfg, const int node);

#endif
