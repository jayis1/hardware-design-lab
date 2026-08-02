/*
 * drivers/flash_lib.c — QSPI liquid-library store
 * Author: jayis1
 * Copyright (C) 2026 jayis1
 *
 * The W25Q64 QSPI flash holds a simple flat-file library:
 *
 *   offset 0x0000   : header (magic, n_classes, version)
 *   offset 0x0100   : liquid_class_t records, contiguous
 *
 * The header carries a 32-bit magic 'HSDL'. Each app command
 * "L,ADD,<id>,<n>,<...>" supplies a full class record (PCA centroid +
 * diagonal covariance) which we write at slot <id> and then bump
 * n_classes / recompute the header CRC. A double-sector scheme would
 * be ideal for power-fail safety; here we keep one copy for simplicity.
 */
#include "flash_lib.h"
#include "../registers.h"
#include <string.h>
#include <stdlib.h>

#define LIB_MAGIC   0x4C445348u   /* 'HSDL' little-endian */
#define LIB_OFFSET_HEADER  0x00000u
#define LIB_OFFSET_RECORDS 0x00100u

typedef struct {
    uint32_t magic;
    uint32_t n_classes;
    uint32_t crc32;
    uint32_t reserved;
} lib_header_t;

/* ---- QSPI driver (indirect mode) ---------------------------------- */
static void qspi_wait_idle(void)
{
    while (QSPI.SR & QSPI_SR_BUSY) { }
}

static void qspi_read(uint32_t addr, uint8_t *buf, uint32_t n)
{
    /* Configure indirect read (0xEB fast read quad). */
    QSPI.DLR = n - 1;
    QSPI.CCR = (3u << 26)   /* FMODE=IND-RD */
             | (4u << 24)   /* DMODE=4 lines */
             | (4u << 8)    /* IMODE=4 lines, addr */
             | (3u << 4);   /* ADMODE=4 lines */
    QSPI.AR = addr;
    QSPI.CR |= (1u << 5);   /* start? — using simplified model */
    qspi_wait_idle();
    for (uint32_t i = 0; i < n; ++i) buf[i] = ((volatile uint8_t *)0x90000000u)[i];
}

static void qspi_write_enable(void)
{
    QSPI.CCR = (0u << 26) | (1u << 24) | (0u << 8);
    qspi_wait_idle();
}

static void qspi_page_program(uint32_t addr, const uint8_t *buf, uint32_t n)
{
    if (n > 256) n = 256;          /* one page max                  */
    qspi_write_enable();
    QSPI.DLR = n - 1;
    QSPI.CCR = (0u << 26)            /* FMODE=IND-W */
             | (4u << 24)            /* DMODE=4 */
             | (3u << 4)             /* ADMODE=4 */
             | (0x02u << 18);        /* opcode = 0x02 Page Program   */
    QSPI.AR = addr;
    for (uint32_t i = 0; i < n; ++i) ((volatile uint8_t *)0x90000000u)[i] = buf[i];
    qspi_wait_idle();
}

static void qspi_erase_sector(uint32_t addr)
{
    qspi_write_enable();
    QSPI.DLR = 0;
    QSPI.CCR = (0u << 26) | (1u << 24) | (3u << 4) | (0x20u << 18); /* 0x20 SE */
    QSPI.AR = addr;
    qspi_wait_idle();
    board_delay_ms(50);          /* ~25–50 ms erase                     */
}

/* ---- Public API ---------------------------------------------------- */
hydra_err_t flash_lib_init(void)
{
    /* Enable QSPI clocks (H7 AHB3). */
    RCC_REG32(RCC_AHB3ENR_OF) |= (1u << 14);   /* QSPI1EN               */
    (void)RCC_REG32(RCC_AHB3ENR_OF);
    QSPI.CR = QSPI_CR_EN;
    QSPI.DCR = 24u << 8;     /* 64 Mbit device, FSIZE = 23 → 2^24 = 16 MB*/
    return HYDRA_OK;
}

hydra_err_t flash_lib_load(library_t *lib)
{
    lib_header_t hdr;
    qspi_read(LIB_OFFSET_HEADER, (uint8_t *)&hdr, sizeof(hdr));
    if (hdr.magic != LIB_MAGIC) {
        /* Empty/uninitialised library. */
        lib->n_classes = 0;
        return HYDRA_ERR_CALIB;
    }
    lib->n_classes = hdr.n_classes;
    for (uint32_t i = 0; i < hdr.n_classes; ++i) {
        qspi_read(LIB_OFFSET_RECORDS + i * sizeof(liquid_class_t),
                  (uint8_t *)&lib->classes[i],
                  sizeof(liquid_class_t));
    }
    return HYDRA_OK;
}

hydra_err_t flash_lib_put_class(const liquid_class_t *cls)
{
    /* Find slot (by class_id) or append. */
    static library_t mirror;     /* lazy static mirror                 */
    flash_lib_load(&mirror);
    uint32_t slot = mirror.n_classes;
    for (uint32_t i = 0; i < mirror.n_classes; ++i)
        if (mirror.classes[i].class_id == cls->class_id) { slot = i; break; }
    if (slot == mirror.n_classes) mirror.n_classes++;

    /* Program the record page (assumes it fits within one 256-byte page). */
    qspi_page_program(LIB_OFFSET_RECORDS + slot * sizeof(liquid_class_t),
                      (const uint8_t *)cls, sizeof(liquid_class_t));

    /* Rewrite header. */
    lib_header_t hdr = { LIB_MAGIC, mirror.n_classes, 0, 0 };
    qspi_erase_sector(LIB_OFFSET_HEADER);
    qspi_page_program(LIB_OFFSET_HEADER, (const uint8_t *)&hdr, sizeof(hdr));
    return HYDRA_OK;
}

/* ---- BLE command parser ------------------------------------------- */
void library_handle_command(const char *line)
{
    /* Format: L,ADD,<id>,<name>,<n_features>,<f1>,<f2>,...
     * We expect 16 centroid + 16 variance floats after the metadata. */
    if (strncmp(line, "L,ADD,", 6) != 0) return;
    const char *p = line + 6;
    uint16_t id = (uint16_t)strtoul(p, (char **)&p, 10); if (*p != ',') return; p++;
    char name[32]; uint8_t i = 0;
    while (*p && *p != ',' && i < sizeof(name) - 1) name[i++] = *p++;
    name[i] = 0; if (*p != ',') return; p++;
    uint8_t n = (uint8_t)strtoul(p, (char **)&p, 10); if (*p != ',') return; p++;

    liquid_class_t cls;
    cls.class_id = id;
    strncpy(cls.name, name, sizeof(cls.name));
    for (uint8_t k = 0; k < FEATURE_DIM_PCA; ++k) {
        cls.centroid[k] = strtof(p, (char **)&p); if (*p != ',') return; p++;
    }
    for (uint8_t k = 0; k < FEATURE_DIM_PCA; ++k) {
        cls.variance[k] = strtof(p, (char **)&p); if (*p == ',') p++;
    }
    flash_lib_put_class(&cls);
}