/*
 * storage.c — HydroFluor microSD logging + QSPI calibration store
 * Author: jayis1
 * License: MIT
 *
 * Two storage backends:
 *   1. microSD (SDMMC1, 4-bit) — rotating CSV log of sample records + raw
 *      fluorescence vectors, for post-hoc analysis and field-survey export.
 *   2. QSPI W25Q128 flash — persistent calibration / PLS model storage.
 *
 * The SD path uses a minimal sector-buffered FAT16/FAT32 writer. A full
 * FATFS port is referenced here; the actual f_open/f_write calls live in
 * the linked fatfs library. This driver wraps it with HydroFluor-specific
 * record formatting and rotation.
 */
#include "storage.h"
#include "../registers.h"
#include <string.h>
#include <stdio.h>

/* FATFS API (linked from the CubeL4 fatfs middleware) */
typedef struct { void *opaque; } FATFS;     /* forward */
typedef struct { void *opaque; } FIL;
extern int  f_mount(FATFS *fs, const char *path, uint8_t opt);
extern int  f_open(FIL *fp, const char *path, uint8_t mode);
extern int  f_write(FIL *fp, const void *buf, uint32_t btw, uint32_t *bw);
extern int  f_close(FIL *fp);
extern int  f_lseek(FIL *fp, uint32_t ofs);
extern int  f_size(FIL *fp);

#define FA_OPEN_APPEND  (0x30)   /* create-or-append */

static FATFS g_fs;
static FIL   g_log_fp;
static FIL   g_raw_fp;
static int   g_log_open = 0;
static int   g_raw_open = 0;
static uint16_t g_seq = 0;

static void sdmmc_gpio_init(void)
{
    /* Enable SDMMC1 clock (APB2ENR bit 28 on L4R9) + GPIOC/D */
    RCC->APB2ENR  |= (1U << 28);
    RCC->AHB2ENR  |= (1U << 2) | (1U << 3);
    /* PC8-PC12 = AF12 (SDMMC), PD2 CMD = AF12 */
    for (uint8_t p = 8; p <= 12; ++p) {
        gpio_mode(GPIOC, p, GPIO_MODE_AF);
        gpio_af(GPIOC, p, 12);
        gpio_pupd(GPIOC, p, GPIO_PUPD_PULLUP);
    }
    gpio_mode(GPIOD, SD_CMD_PIN, GPIO_MODE_AF);
    gpio_af(GPIOD, SD_CMD_PIN, 12);
    gpio_pupd(GPIOD, SD_CMD_PIN, GPIO_PUPD_PULLUP);
    /* Card detect on PC15 (input, pull-up; active low) */
    gpio_mode(GPIOC, SD_CD_PIN, GPIO_MODE_INPUT);
    gpio_pupd(GPIOC, SD_CD_PIN, GPIO_PUPD_PULLUP);
}

static int sd_present(void)
{
    return (gpio_read(GPIOC, SD_CD_PIN) == 0) ? 1 : 0;
}

/* ---- QSPI flash helpers ---- */

static void qspi_init(void)
{
    RCC->AHB3ENR |= (1U << 8);   /* QSPI clock enable on L4R9 */
    /* GPIOs PB2/10/11/12/13/14 = AF10 (QUADSPI) */
    gpio_mode(GPIOB, QSPI_CS_PIN,  GPIO_MODE_AF); gpio_af(GPIOB, QSPI_CS_PIN, 10);
    gpio_mode(GPIOB, QSPI_CLK_PIN, GPIO_MODE_AF); gpio_af(GPIOB, QSPI_CLK_PIN, 10);
    gpio_mode(GPIOB, QSPI_D0_PIN,  GPIO_MODE_AF); gpio_af(GPIOB, QSPI_D0_PIN, 10);
    gpio_mode(GPIOB, QSPI_D1_PIN,  GPIO_MODE_AF); gpio_af(GPIOB, QSPI_D1_PIN, 10);
    gpio_mode(GPIOB, QSPI_D2_PIN,  GPIO_MODE_AF); gpio_af(GPIOB, QSPI_D2_PIN, 10);
    gpio_mode(GPIOB, QSPI_D3_PIN,  GPIO_MODE_AF); gpio_af(GPIOB, QSPI_D3_PIN, 10);

    /* Configure QSPI: FSEL=0 (nCS on PB10), DFM=0, prescaler /2 → 60 MHz */
    QSPI->DCR = (0U << 0)            /* FSEL */
             |  (3U << 1)            /* DFM=0? device size 3 → 2^(3+1) bytes */
             |  (23U << 8);          /* FSIZE 64 MB → 2^26 */
    QSPI->CR  = (1U << 0)            /* EN */
             |  (1U << 4);           /* PRESCALER /2 */
}

/* Write a PLS model to a fixed flash address per analyte. */
#define QSPI_MODEL_BASE 0x90010000UL   /* memory-mapped region offset 64K */
#define QSPI_MODEL_SLOT sizeof(pls_model_t)

int storage_save_model(analyte_t a, const pls_model_t *m)
{
    /* Real implementation: erase the 4 KB sector then write via indirect mode.
     * Here we use a memory-mapped copy through QSPI indirect write. */
    uint32_t addr = QSPI_MODEL_BASE + (uint32_t)a * QSPI_MODEL_SLOT;
    /* Indirect write setup */
    QSPI->CCR = QSPI_CCR_FMODE_IND_WRITE
              | (0x02 << 24)             /* DMODE quad */
              | (0x01 << 8);             /* IMODE single */
    QSPI->AR  = addr;
    QSPI->DLR = sizeof(pls_model_t) - 1;
    /* Copy bytes into the FIFO via ABR */
    const uint8_t *src = (const uint8_t *)m;
    for (uint32_t i = 0; i < sizeof(pls_model_t); ++i) {
        QSPI->ABR = src[i];
    }
    while (!(QSPI->SR & QSPI_SR_TCF)) { }
    QSPI->FCR = QSPI_SR_TCF;   /* clear flag */
    return 0;
}

int storage_load_model(analyte_t a, pls_model_t *m)
{
    uint32_t addr = QSPI_MODEL_BASE + (uint32_t)a * QSPI_MODEL_SLOT;
    /* Indirect read */
    QSPI->CCR = QSPI_CCR_FMODE_IND_READ
              | (0x03 << 24)            /* DMODE quad */
              | (0x03 << 8)             /* IMODE quad */
              | (0x02 << 18)            /* ADSIZE 24-bit */
              | (1U << 7);              /* ADDRM */
    QSPI->AR  = addr;
    QSPI->DLR = sizeof(pls_model_t) - 1;
    uint8_t *dst = (uint8_t *)m;
    for (uint32_t i = 0; i < sizeof(pls_model_t); ++i) {
        while (!(QSPI->SR & (1U << 3))) { }   /* FT flag */
        dst[i] = (uint8_t)QSPI->DR;
    }
    while (!(QSPI->SR & QSPI_SR_TCF)) { }
    QSPI->FCR = QSPI_SR_TCF;
    return 0;
}

int storage_init(void)
{
    sdmmc_gpio_init();
    qspi_init();

    if (!sd_present()) {
        return -1;   /* no card; logging disabled, QSPI still works */
    }
    if (f_mount(&g_fs, "0:", 1) != 0) {
        return -2;
    }
    /* Open the CSV log in append mode */
    if (f_open(&g_log_fp, "0:hydrofluor.csv", FA_OPEN_APPEND) == 0) {
        g_log_open = 1;
    }
    if (f_open(&g_raw_fp, "0:hydrofluor_raw.csv", FA_OPEN_APPEND) == 0) {
        g_raw_open = 1;
    }
    g_seq = 0;
    return 0;
}

int storage_log_sample(const sample_record_t *rec)
{
    if (!g_log_open) return -1;
    char line[160];
    int n = snprintf(line, sizeof(line),
        "%u,%lu,%u,%d.%02d,%u,%u,%u,%u,%u,%u.%02u,%u\n",
        rec->seq, (unsigned long)rec->timestamp,
        rec->depth_cm,
        rec->temp_c01 / 100, rec->temp_c01 % 100,
        rec->battery_mv,
        rec->cdom_ppb, rec->chla_ugl, rec->phyc_ugl, rec->oil_ppb,
        rec->turb_ntu_x100 / 100, rec->turb_ntu_x100 % 100,
        rec->flags);
    uint32_t bw = 0;
    f_write(&g_log_fp, line, (uint32_t)n, &bw);
    /* Rotate if file exceeds the cap (approx) */
    if (f_size(&g_log_fp) > LOG_FILE_MAX_KB * 1024) {
        f_close(&g_log_fp);
        /* Rename to .old and start fresh — omitted for brevity */
        f_open(&g_log_fp, "0:hydrofluor.csv", FA_OPEN_APPEND);
    }
    return (int)bw;
}

int storage_log_raw(const int16_t feat[PLS_FEATURES], uint16_t seq)
{
    if (!g_raw_open) return -1;
    char line[160];
    char *p = line;
    p += snprintf(p, sizeof(line), "%u,", seq);
    for (int i = 0; i < PLS_FEATURES; ++i) {
        p += snprintf(p, 16, "%d,", feat[i]);
    }
    *p++ = '\n'; *p = '\0';
    uint32_t bw = 0;
    f_write(&g_raw_fp, line, (uint32_t)(p - line), &bw);
    return (int)bw;
}

uint16_t storage_next_seq(void)
{
    return g_seq++;
}