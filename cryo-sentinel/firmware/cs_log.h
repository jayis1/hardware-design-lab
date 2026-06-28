/*
 * cs_log.h — FRAM-backed audit log for Cryo-Sentinel.
 *
 * Author: jayis1
 * License: MIT
 */
#ifndef CRYO_SENTINEL_CS_LOG_H
#define CRYO_SENTINEL_CS_LOG_H

#include <stdint.h>
#include <stdbool.h>
#include "cs_alarm.h"

typedef enum {
    CS_LOG_BOOT        = 0x01,
    CS_LOG_STATE       = 0x02,   /* state transition */
    CS_LOG_LID_OPEN    = 0x03,
    CS_LOG_LID_CLOSE   = 0x04,
    CS_LOG_ENC_OPEN    = 0x05,
    CS_LOG_ENC_CLOSE   = 0x06,
    CS_LOG_MAINS_LOST  = 0x07,
    CS_LOG_MAINS_BACK  = 0x08,
    CS_LOG_ALARM_SEND  = 0x09,
    CS_LOG_ALARM_ACK   = 0x0A,
    CS_LOG_CAL_START   = 0x0B,
    CS_LOG_CAL_DONE    = 0x0C,
    CS_LOG_HEARTBEAT   = 0x0D,
    CS_LOG_FAULT       = 0x0E,
} cs_log_event_t;

/* 16-byte record stored in FRAM. */
typedef struct __attribute__((packed)) {
    uint8_t  event;          /* cs_log_event_t */
    uint8_t  reserved;
    uint16_t crc;
    uint32_t seq;            /* monotonic, persists across reboots */
    uint32_t epoch_min;      /* time of event */
    int32_t  aux;            /* event-specific (state, reason, etc.) */
} cs_log_record_t;

_Static_assert(sizeof(cs_log_record_t) == 16, "log record must be 16 bytes");

/* Initialize FRAM and recover the monotonic seq counter. */
int cs_log_init(void);

/* Append a record. seq and epoch_min are filled by the logger. */
int cs_log_append(cs_log_event_t ev, int32_t aux, uint32_t epoch_min);

/* Read out records for cloud sync. Returns count read. */
int cs_log_read_since(uint32_t since_seq, cs_log_record_t *out, int max);

/* Current monotonic seq. */
uint32_t cs_log_seq(void);

/* The most recent N records (for the BLE local preview). */
int cs_log_tail(int n, cs_log_record_t *out);

#endif /* CRYO_SENTINEL_CS_LOG_H */