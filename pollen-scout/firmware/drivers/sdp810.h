/*
 * sdp810.h - SDP810 differential pressure / flow driver header
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#ifndef SDP810_H
#define SDP810_H

int   sdp810_init(void);
float sdp810_read_flow_lpm(void);   /* liters per minute */

#endif /* SDP810_H */