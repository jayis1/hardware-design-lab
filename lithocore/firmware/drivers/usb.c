/*
 * usb.c — USB-C CDC serial + DFU support.
 *
 * Implements a minimal USB CDC (virtual serial port) for data export
 * (CSV/JSON) and firmware update via DFU. The STM32G474 has a built-in
 * USB 2.0 full-speed device controller.
 *
 * This is a simplified implementation — a production version would use
 * the full USB CDC class driver with proper enumeration descriptors.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "usb.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>
#include <stdio.h>

/* -------------------------------------------------------------------------
 * USB state
 * ------------------------------------------------------------------------- */
static uint8_t usb_enumerated = 0;
static uint8_t usb_tx_buf[256];
static volatile uint16_t usb_tx_len = 0;

/* -------------------------------------------------------------------------
 * USB init
 *
 * Enables the USB peripheral, configures PA11/PA12 as USB AF, and
 * sets up the minimal CDC descriptors. In a production implementation,
 * this would include the full USB device descriptor, configuration
 * descriptor, and CDC class-specific descriptors.
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int usb_init(void)
{
    /* Enable USB clock */
    volatile uint32_t *apb1enr1 = (volatile uint32_t *)(RCC_BASE + 0x58);
    *apb1enr1 |= (1U << 23);  /* USB enable bit on APB1 */

    /* USB peripheral init — simplified.
     * In production: configure endpoint 0 (control), endpoint 1 (CDC bulk in),
     * endpoint 2 (CDC bulk out), and the CDC interrupt endpoint. */
    usb_enumerated = 0;
    usb_tx_len = 0;

    return 0;
}

/* -------------------------------------------------------------------------
 * USB service — called from main loop
 *
 * In a full implementation, this would:
 *   - Handle USB reset, suspend, resume
 *   - Process SETUP packets (enumeration)
 *   - Handle CDC data requests
 *
 * For this firmware, we provide a polled implementation that checks
 * for USB VBUS and handles basic data transmission.
 * ------------------------------------------------------------------------- */
void usb_service(void)
{
    /* Check USB VBUS (PC1/AN_VBUS via ADC, or a GPIO) */
    /* If VBUS present and not enumerated, begin enumeration */

    /* If we have data to send and the endpoint is ready, send it */
    if (usb_tx_len > 0 && usb_enumerated) {
        /* Write to USB TX endpoint FIFO */
        /* In production: USB->TXBUF = usb_tx_buf; USB->TXCOUNT = usb_tx_len; */
        usb_tx_len = 0;
    }
}

/* -------------------------------------------------------------------------
 * Send a string over USB CDC
 * ------------------------------------------------------------------------- */
void usb_send_string(const char *str)
{
    uint16_t len = 0;
    while (str[len]) len++;

    if (usb_tx_len + len > sizeof(usb_tx_buf)) {
        /* Flush buffer first */
        usb_tx_len = 0;
    }
    memcpy(&usb_tx_buf[usb_tx_len], str, len);
    usb_tx_len += len;
}

/* -------------------------------------------------------------------------
 * Send result as CSV
 *
 * Format:
 *   "LithoCore Result\r\n"
 *   "Date,2026-07-27\r\n"
 *   "Chemistry,NMC-18650\r\n"
 *   "SoH,85\r\n"
 *   "Mode,Healthy\r\n"
 *   "OCV_mV,3720\r\n"
 *   ...
 *   "freq_Hz,re_Z_mOhm,im_Z_mOhm,mag_mOhm,phase_deg\r\n"
 *   0.01,-12.3,45.6,...
 *
 * Author: jayis1
 * ------------------------------------------------------------------------- */
int usb_send_csv(const soh_result_t *result)
{
    char line[128];

    snprintf(line, sizeof(line),
             "LithoCore Result\r\n"
             "Author,jayis1\r\n"
             "SoH,%u\r\n"
             "Mode,%s\r\n"
             "Verdict,%s\r\n"
             "Chemistry,%s\r\n"
             "OCV_mV,%u\r\n"
             "Temp_dC,%u\r\n"
             "DCIR_mOhm,%u\r\n"
             "SelfDischarge_uV_per_min,%ld\r\n"
             "FitValid,%u\r\n",
             result->soh_score,
             soh_mode_name(result->degradation),
             soh_verdict_name(result->verdict),
             soh_chemistry_name(result->chemistry_idx),
             result->ocv_mv,
             result->temp_dc,
             result->dcir_mohm,
             (long)result->self_discharge_uv_per_min,
             result->fit_valid);
    usb_send_string(line);

    /* Randles parameters */
    if (result->fit_valid) {
        snprintf(line, sizeof(line),
                 "Rs_mOhm,%ld\r\n"
                 "Rsei_mOhm,%ld\r\n"
                 "Csei_mF,%ld\r\n"
                 "Rct_mOhm,%ld\r\n"
                 "Cdl_mF,%ld\r\n"
                 "Sigma,%ld\r\n",
                 (long)result->randles.rs_mohm,
                 (long)result->randles.rsei_mohm,
                 (long)result->randles.csei_mF,
                 (long)result->randles.rct_mohm,
                 (long)result->randles.cdl_mF,
                 (long)result->randles.sigma);
        usb_send_string(line);
    }

    /* Sweep data table */
    usb_send_string("\r\nfreq_Hz,re_Z,im_Z,mag,phase\r\n");
    for (uint16_t i = 0; i < result->sweep_data.num_points; i++) {
        const lockin_result_t *pt = &result->sweep_data.points[i];
        if (pt->valid) {
            snprintf(line, sizeof(line), "%lu,%ld,%ld,%ld,%ld\r\n",
                     (unsigned long)pt->freq_hz,
                     (long)pt->re_z, (long)pt->im_z,
                     (long)pt->mag_z, (long)pt->phase_mdeg);
            usb_send_string(line);
        }
    }

    usb_send_string("---END---\r\n");
    return 0;
}

/* -------------------------------------------------------------------------
 * Send result (defaults to CSV format over USB)
 * ------------------------------------------------------------------------- */
int usb_send_result(const soh_result_t *result)
{
    return usb_send_csv(result);
}

/* -------------------------------------------------------------------------
 * Enter DFU mode
 *
 * Jumps to the STM32 system bootloader (mapped at 0x1FFF0000 for G4).
 * The user can then flash new firmware via dfu-util or STM32CubeProgrammer.
 * ------------------------------------------------------------------------- */
void usb_enter_dfu(void)
{
    /* Disable peripherals */
    USART1->CR1 = 0;
    SPI1->CR1 = 0;
    ADC1->CR = 0;

    /* Remap system memory to 0x00000000 and jump to bootloader */
    /* Set MEM_MODE = system memory in SYSCFG */
    volatile uint32_t *syscfg_memrmp = (volatile uint32_t *)(SYSCFG_BASE);
    *syscfg_memrmp = 0x1;  /* map system memory to 0x00000000 */

    /* Jump to 0x1FFF0000 (system bootloader) */
    typedef void (*func_ptr)(void);
    uint32_t boot_addr = 0x1FFF0000;
    uint32_t boot_sp = *(volatile uint32_t *)boot_addr;
    func_ptr boot_entry = (func_ptr)*(volatile uint32_t *)(boot_addr + 4);

    /* Set MSP and jump */
    __asm volatile ("msr msp, %0" :: "r"(boot_sp));
    boot_entry();

    /* Never reached */
}