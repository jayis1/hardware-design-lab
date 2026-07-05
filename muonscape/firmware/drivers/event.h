/*
 * event.h — muon event builder + SD card logging
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_EVENT_H
#define MUONSCAPE_DRV_EVENT_H

#include <stdint.h>
#include "board.h"
#include "track.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t seq;             /* event sequence number          */
    uint32_t timestamp_ms;     /* acquisition-relative time       */
    float    theta;           /* zenith (rad)                   */
    float    phi;             /* azimuth (rad)                  */
    float    x, y, z;          /* entry point (cm)               */
    float    dx, dy, dz;       /* direction                       */
    float    energy_gev;       /* rough energy estimate          */
    uint8_t  quality;          /* 0..255                         */
    uint8_t  _pad[3];
} __attribute__((packed)) muon_event_t;

muon_status_t event_init(void);
muon_status_t event_open(const char *path);   /* open acquisition file */
muon_status_t event_write(const muon_event_t *e);
muon_status_t event_flush(void);              /* drain ring buffer to SD */
void          event_close(void);
uint32_t      event_count(void);

/* Build an event from a track + hit timing */
void event_from_track(const muon_track_t *t, muon_event_t *out);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_EVENT_H */