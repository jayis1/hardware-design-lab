/*
 * fpga.h — iCE40-UP5K SPI front-end driver interface
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: MIT
 */
#ifndef MUONSCAPE_DRV_FPGA_H
#define MUONSCAPE_DRV_FPGA_H

#include <stdint.h>
#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Hit word format from FPGA (32-bit) ----
 * [31:24] layer (0,1,2) | [23:16] strip index 0..15
 * [15: 6] TDC bin (0..1023, 50 ps/bin -> 0..51.2 ns)
 * [ 5: 0] ASIC channel within layer (0..63)
 */
typedef union {
    uint32_t raw;
    struct {
        uint32_t chan6  : 6;   /* sub-channel within layer */
        uint32_t tdc    : 10;  /* TDC bin */
        uint32_t strip  : 8;   /* strip index within layer */
        uint32_t layer  : 3;   /* 0,1,2 */
        uint32_t _rsv   : 5;
    } f;
} muon_hit_t;

/* ---- FPGA command opcodes (sent over MOSI before hit read) ---- */
#define FPGA_CMD_NOP        0x00
#define FPGA_CMD_RD_FIFO    0x10  /* read one hit word */
#define FPGA_CMD_RD_BURST   0x11  /* read burst, length in next byte */
#define FPGA_CMD_FIFO_CLR   0x20
#define FPGA_CMD_SET_WINDOW 0x30  /* coincidence window, 1 byte in ns */
#define FPGA_CMD_GET_VER   0xF0
#define FPGA_CMD_GET_HITS   0x12  /* get hit FIFO occupancy (1 word) */

/* ---- API ---- */
muon_status_t fpga_init(void);
muon_status_t fpga_load_bitstream(const uint8_t *bit, uint32_t len);
muon_status_t fpga_set_coincidence_window(uint8_t ns);
uint32_t      fpga_read_version(void);
uint32_t      fpga_hit_count(void);              /* FIFO occupancy */
muon_status_t fpga_read_burst(muon_hit_t *out, uint16_t max, uint16_t *got);
muon_status_t fpga_clear_fifo(void);
void          fpga_hold_set(int hold);            /* freeze FIFO */
void          fpga_irq_enable(int en);

/* Called from EXTI handler when hit FIFO passes the watermark */
void fpga_irq_handler(void);

#ifdef __cplusplus
}
#endif
#endif /* MUONSCAPE_DRV_FPGA_H */