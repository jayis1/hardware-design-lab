/*
 * fpga.h — iCE40UP5K FPGA driver interface
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-2.0
 */

#ifndef SPECKLEFLOW_FPGA_H
#define SPECKLEFLOW_FPGA_H

#include <stdint.h>

/**
 * Initialize the FPGA: wait for CDONE, verify version, configure defaults.
 * @return 0 on success, negative on error
 */
int fpga_init(void);

/**
 * Set the speckle contrast window size.
 * @param size  0=5×5, 1=7×7, 2=9×9
 * @return 0 on success, -1 on invalid size
 */
int fpga_set_window(uint8_t size);

/**
 * Set the output frame rate.
 * @param mode  0=30, 1=60, 2=120 fps
 * @return 0 on success, -1 on invalid mode
 */
int fpga_set_frame_rate(uint8_t mode);

/**
 * Set calibration offset and scale for flow-map computation.
 * @param offset  Static-reference contrast K_static (8-bit)
 * @param scale   Scale factor for flow index (8-bit)
 */
void fpga_set_calibration(uint16_t offset, uint16_t scale);

/**
 * Enable or disable the contrast pipeline.
 */
void fpga_enable(int on);

/**
 * Trigger a single-shot frame capture.
 */
void fpga_trigger_single(void);

/**
 * Start a calibration cycle (captures static reference).
 */
void fpga_start_calibration(void);

/**
 * Read the FPGA status register.
 */
uint8_t fpga_get_status(void);

/**
 * Read the 32-bit frame counter.
 */
uint32_t fpga_get_frame_count(void);

/**
 * Read the FPGA design version.
 */
uint32_t fpga_get_version(void);

/**
 * Start DMA reception of a new flow-map frame from the FPGA.
 * @param buf  307,200-byte buffer to receive the frame
 */
void fpga_start_frame_dma(uint8_t *buf);

/**
 * Interrupt handler — called when FPGA signals frame ready (PC4 IRQ).
 */
void fpga_isr_frame_ready(void);

/**
 * Interrupt handler — called when SPI DMA transfer completes.
 */
void fpga_isr_dma_complete(void);

/**
 * Get a pointer to the most recently completed frame, or NULL.
 * The caller must process the frame before the next one arrives.
 */
uint8_t *fpga_get_frame(void);

/**
 * Check if a new frame is ready for processing.
 */
int fpga_is_frame_ready(void);

#endif /* SPECKLEFLOW_FPGA_H */