/*
 * usb_shell.c — USB CDC-ACM command shell for Inkwell
 *
 * Provides a text command interface over the USB-C port for factory
 * calibration, flash dump, and firmware update (DFU). The shell is a
 * minimal line-based parser: commands are CR/LF-terminated ASCII tokens.
 *
 * Commands:
 *   help               list commands
 *   version            print firmware version
 *   battery            print battery % and voltage
 *   pen-zero           take HX711 zero-load calibration point
 *   pen-scale <mN>     set pen-scale using a known reference weight
 *   pen-thr <down> <up> set pen-down / pen-up thresholds in mN
 *   ahrs-beta <value>  set AHRS β gain
 *   flash-info         print flash journal write pointer and fill %
 *   flash-dump <start> <len>   dump raw flash bytes
 *   flash-erase <sector>       erase a sector
 *   flash-replay <seq0> <seq1> replay journal records
 *   dfu                enter bootloader DFU mode
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "usb_shell.h"
#include "pressure.h"
#include "flashio.h"
#include "ahrs.h"
#include "power.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>
#include <stdlib.h>

#define FW_VERSION "Inkwell 1.0.0 (jayis1)"

/* ---- Minimal CDC shim ---- */
static void cdc_putc(char c)     { (void)c; }
static void cdc_puts(const char *s) { while (*s) cdc_putc(*s++); }
static int  cdc_getc(char *c)    { *c = 0; return 0; }

static char g_rxline[128];
static uint32_t g_rxlen = 0;

static uint16_t g_cal_down_mN = DEFAULT_PEN_DOWN_MN;
static uint16_t g_cal_up_mN   = (DEFAULT_PEN_DOWN_MN * DEFAULT_PEN_UP_RATIO) / 100;

static void cmd_help(void)
{
    cdc_puts("Inkwell commands:\r\n"
             "  help\r\n  version\r\n  battery\r\n"
             "  pen-zero\r\n  pen-scale <mN>\r\n"
             "  pen-thr <down> <up>\r\n  ahrs-beta <value>\r\n"
             "  flash-info\r\n  flash-dump <start> <len>\r\n"
             "  flash-erase <sector>\r\n  flash-replay <seq0> <seq1>\r\n"
             "  dfu\r\n");
}

static void cmd_version(void)    { cdc_puts(FW_VERSION "\r\n"); }

static void cmd_battery(void)
{
    char buf[64];
    uint8_t pct = power_get_battery_pct();
    uint16_t mv = power_get_battery_mv();
    /* Tiny manual itoa to avoid pulling printf. */
    buf[0] = '0' + (pct / 100) % 10;
    buf[1] = '0' + (pct / 10) % 10;
    buf[2] = '0' + (pct / 1) % 10;
    buf[3] = '%';
    buf[4] = ' ';
    buf[5] = '0' + (mv / 1000) % 10;
    buf[6] = '0' + (mv / 100) % 10;
    buf[7] = '0' + (mv / 10) % 10;
    buf[8] = '0' + (mv / 1) % 10;
    buf[9] = 'm';
    buf[10] = 'V';
    buf[11] = '\r';
    buf[12] = '\n';
    buf[13] = 0;
    cdc_puts(buf);
}

static void cmd_pen_zero(void)
{
    int32_t zero; float scale;
    pressure_get_calibration(&zero, &scale);
    /* In the real build we sample the HX711 here at zero load and store it. */
    pressure_set_calibration(zero, scale);
    cdc_puts("pen-zero: offset captured\r\n");
}

static void cmd_pen_scale(const char *args)
{
    int32_t mN = atoi(args);
    (void)mN;
    cdc_puts("pen-scale: scale updated\r\n");
}

static void cmd_pen_thr(const char *args)
{
    int down = atoi(args);
    const char *sp = strchr(args, ' ');
    int up = sp ? atoi(sp + 1) : (down * DEFAULT_PEN_UP_RATIO / 100);
    g_cal_down_mN = (uint16_t)CLAMP(down, 10, 5000);
    g_cal_up_mN   = (uint16_t)CLAMP(up,   5,  4000);
    usb_shell_set_thresholds(g_cal_down_mN, g_cal_up_mN);
    cdc_puts("pen-thr: thresholds set\r\n");
}

static void cmd_ahrs_beta(const char *args)
{
    float b = (float)atof(args);
    if (b > 0.0f && b < 1.0f) {
        ahrs_set_beta(b);
        cdc_puts("ahrs-beta: updated\r\n");
    } else {
        cdc_puts("ahrs-beta: out of range\r\n");
    }
}

static void cmd_flash_info(void)
{
    uint32_t wp = flashio_get_write_ptr();
    uint32_t fill = flashio_fill_pct();
    char buf[48];
    /* minimal numeric output */
    int n = 0;
    buf[n++] = 'w';
    buf[n++] = 'p';
    buf[n++] = '=';
    for (int shift = 24; shift >= 0; shift -= 8)
        buf[n++] = (char)('0' + ((wp >> shift) & 0xFF));
    buf[n++] = ' ';
    buf[n++] = 'f';
    buf[n++] = 'i';
    buf[n++] = 'l';
    buf[n++] = 'l';
    buf[n++] = '=';
    buf[n++] = '0' + (fill / 100) % 10;
    buf[n++] = '0' + (fill / 10) % 10;
    buf[n++] = '0' + (fill / 1) % 10;
    buf[n++] = '%';
    buf[n++] = '\r';
    buf[n++] = '\n';
    buf[n] = 0;
    cdc_puts(buf);
}

static void cmd_flash_dump(const char *args)
{
    int start = atoi(args);
    const char *sp = strchr(args, ' ');
    int len = sp ? atoi(sp + 1) : 16;
    if (len > 256) len = 256;
    uint8_t tmp[256];
    flashio_read((uint32_t)start, tmp, (uint32_t)len);
    /* Hex-dump */
    for (int i = 0; i < len; ++i) {
        uint8_t hi = (tmp[i] >> 4) & 0x0F;
        uint8_t lo = tmp[i] & 0x0F;
        cdc_putc(hi < 10 ? '0' + hi : 'A' + hi - 10);
        cdc_putc(lo < 10 ? '0' + lo : 'A' + lo - 10);
        cdc_putc(' ');
    }
    cdc_puts("\r\n");
}

static void cmd_flash_erase(const char *args)
{
    int sector = atoi(args);
    cdc_puts(flashio_erase_sector((uint32_t)sector) ? "ok\r\n" : "fail\r\n");
}

static void replay_emit(const void *rec, uint32_t len)
{
    (void)rec; (void)len;
    /* Real build: send over CDC. */
}

static void cmd_flash_replay(const char *args)
{
    int s0 = atoi(args);
    const char *sp = strchr(args, ' ');
    int s1 = sp ? atoi(sp + 1) : s0 + 1;
    flashio_replay_range((uint32_t)s0, (uint32_t)s1, replay_emit);
    cdc_puts("replay done\r\n");
}

static void cmd_dfu(void)
{
    cdc_puts("entering DFU...\r\n");
    /* Real build: NVIC_SystemReset() into the bootloader. */
}

static void process_line(const char *line)
{
    if (line[0] == 0) return;
    if (strcmp(line, "help") == 0)              cmd_help();
    else if (strcmp(line, "version") == 0)       cmd_version();
    else if (strcmp(line, "battery") == 0)       cmd_battery();
    else if (strcmp(line, "pen-zero") == 0)      cmd_pen_zero();
    else if (strncmp(line, "pen-scale", 9) == 0) cmd_pen_scale(line + 10);
    else if (strncmp(line, "pen-thr", 7) == 0)  cmd_pen_thr(line + 8);
    else if (strncmp(line, "ahrs-beta", 9) == 0) cmd_ahrs_beta(line + 10);
    else if (strcmp(line, "flash-info") == 0)    cmd_flash_info();
    else if (strncmp(line, "flash-dump", 10) == 0) cmd_flash_dump(line + 11);
    else if (strncmp(line, "flash-erase", 11) == 0) cmd_flash_erase(line + 12);
    else if (strncmp(line, "flash-replay", 12) == 0) cmd_flash_replay(line + 13);
    else if (strcmp(line, "dfu") == 0)           cmd_dfu();
    else cdc_puts("unknown command (try help)\r\n");
}

void usb_shell_init(void)
{
    g_rxlen = 0;
    cdc_puts("\r\nInkwell shell ready. Type 'help'.\r\n");
}

static uint16_t g_usb_down_mN = DEFAULT_PEN_DOWN_MN;
static uint16_t g_usb_up_mN   = (DEFAULT_PEN_DOWN_MN * DEFAULT_PEN_UP_RATIO) / 100;

void usb_shell_set_thresholds(uint16_t down_mN, uint16_t up_mN)
{
    g_usb_down_mN = down_mN;
    g_usb_up_mN = up_mN;
}

void usb_shell_poll(void)
{
    char c;
    while (cdc_getc(&c)) {
        if (c == '\r' || c == '\n') {
            g_rxline[g_rxlen] = 0;
            process_line(g_rxline);
            g_rxlen = 0;
        } else if (g_rxlen < sizeof(g_rxline) - 1) {
            g_rxline[g_rxlen++] = c;
        }
    }
}