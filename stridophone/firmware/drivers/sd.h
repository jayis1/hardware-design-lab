/*
 * sd.h — minimal SDMMC1 block driver + thin FAT layer for event logging.
 * Author : jayis1
 * License: MIT
 */
#ifndef STRIDOPHONE_SD_H
#define STRIDOPHONE_SD_H

#include <stdint.h>

void sd_init(void);

/* Read/write 512-byte blocks. */
int sd_read_block(uint32_t lba, void *buf);
int sd_write_block(uint32_t lba, const void *buf);

/* Open/append/close a log file using a tiny FAT16/FAT32 writer. */
int sd_log_open(const char *fname);
int sd_log_append(const char *line);
int sd_log_close(void);

/* Write a 2-second WAV snippet (PCM 16-bit mono 16 kHz) to SD. */
int sd_write_wav(const char *fname, const int16_t *pcm, int nsamp);

#endif