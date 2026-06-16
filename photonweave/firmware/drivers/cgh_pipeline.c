/**
 * cgh_pipeline.c — CGH Compute Pipeline Driver Implementation
 *
 * Full driver for the Computer-Generated Holography pipeline in the
 * PhotonWeave FPGA. Communicates with the FPGA via SPI-mapped register
 * access to configure parameters, submit depth maps, and retrieve
 * computed holograms.
 *
 * The CGH pipeline implements the Angular Spectrum Method (ASM) for
 * real-time hologram computation from depth-map input:
 *   1. Depth map → complex amplitude field
 *   2. 2D FFT of source field
 *   3. Multiply by transfer function for each depth plane
 *   4. Accumulate contributions
 *   5. 2D IFFT to spatial domain
 *   6. Phase quantize to 8-bit grayscale
 */

#include "cgh_pipeline.h"
#include "registers.h"
#include "board.h"
#include <string.h>

//=============================================================================
// Internal Helpers
//=============================================================================

/**
 * Read a CGH register via SPI.
 */
static uint32_t cgh_reg_read(uint32_t offset)
{
    uint8_t tx[4] = {
        SPI_CMD_REG_READ,
        (uint8_t)((offset >> 16) & 0xFF),
        (uint8_t)((offset >> 8) & 0xFF),
        (uint8_t)(offset & 0xFF)
    };
    uint8_t rx[4] = {0};

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    return ((uint32_t)rx[1] << 16) | ((uint32_t)rx[2] << 8) | rx[3];
}

/**
 * Write a CGH register via SPI.
 */
static void cgh_reg_write(uint32_t offset, uint32_t value)
{
    // Write byte 0 (LSB)
    uint8_t tx[4] = {
        SPI_CMD_REG_WRITE,
        (uint8_t)((offset >> 16) & 0xFF),
        (uint8_t)((offset >> 8) & 0xFF),
        (uint8_t)(value & 0xFF)
    };
    uint8_t rx[4];

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Write byte 1
    tx[0] = SPI_CMD_REG_WRITE;
    tx[1] = (uint8_t)((offset >> 16) & 0xFF);
    tx[2] = (uint8_t)(((offset + 1) >> 8) & 0xFF);
    tx[3] = (uint8_t)((value >> 8) & 0xFF);

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Write byte 2
    tx[0] = SPI_CMD_REG_WRITE;
    tx[1] = (uint8_t)((offset >> 16) & 0xFF);
    tx[2] = (uint8_t)(((offset + 2) >> 8) & 0xFF);
    tx[3] = (uint8_t)((value >> 16) & 0xFF);

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

    // Write byte 3 (MSB)
    tx[0] = SPI_CMD_REG_WRITE;
    tx[1] = (uint8_t)((offset >> 16) & 0xFF);
    tx[2] = (uint8_t)(((offset + 3) >> 8) & 0xFF);
    tx[3] = (uint8_t)((value >> 24) & 0xFF);

    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 50);
    HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
}

/**
 * Write a parameter set to the FPGA register file.
 * Each parameter set occupies 64 bytes (16 × 32-bit registers).
 */
static void cgh_write_param_set(uint8_t param_set, const cgh_params_t *params)
{
    uint32_t base = PW_REG_CGH_PARAM_BASE + (param_set * PW_REG_CGH_PARAM_STRIDE);

    cgh_reg_write(base + 0x00, params->wavelength_nm_x1000);
    cgh_reg_write(base + 0x04, params->depth_min_um);
    cgh_reg_write(base + 0x08, params->depth_max_um);
    cgh_reg_write(base + 0x0C, params->depth_planes);
    cgh_reg_write(base + 0x10, params->input_width);
    cgh_reg_write(base + 0x14, params->input_height);
    cgh_reg_write(base + 0x18, params->output_width);
    cgh_reg_write(base + 0x1C, params->output_height);
    cgh_reg_write(base + 0x20, params->fft_size);
    cgh_reg_write(base + 0x24, params->propagation_model);
    cgh_reg_write(base + 0x28, params->phase_quantize_bits);
    cgh_reg_write(base + 0x2C, params->color_channel);
    cgh_reg_write(base + 0x30, params->pixel_pitch_um_x100);
    cgh_reg_write(base + 0x34, params->fill_factor_percent);
    cgh_reg_write(base + 0x38, 0); // reserved
    cgh_reg_write(base + 0x3C, 0); // reserved
}

//=============================================================================
// Public API Implementation
//=============================================================================

int cgh_pipeline_init(void)
{
    // 1. Verify device magic number
    uint32_t magic = cgh_reg_read(PW_REG_MAGIC);
    if (magic != 0x50484F54) { // "PHOT"
        debug_uart_printf("CGH: Invalid device magic: 0x%08lX\r\n", magic);
        return -1;
    }

    // 2. Check FPGA ready
    uint32_t status = cgh_reg_read(PW_REG_STATUS);
    if (!(status & PW_STATUS_FPGA_READY)) {
        debug_uart_printf("CGH: FPGA not ready (status=0x%08lX)\r\n", status);
        return -2;
    }

    // 3. Check DDR4 calibration
    if (!(status & PW_STATUS_DDR4_CAL_DONE)) {
        debug_uart_printf("CGH: DDR4 not calibrated\r\n");
        return -3;
    }

    // 4. Reset CGH pipeline
    int reset_result = cgh_pipeline_soft_reset();
    if (reset_result != 0) {
        debug_uart_printf("CGH: Pipeline reset failed\r\n");
        return -4;
    }

    // 5. Configure default parameter sets for RGB color channels
    // Red channel (param set 0)
    cgh_params_t red_params = {
        .wavelength_nm_x1000 = CGH_WAVELENGTH_RED,
        .depth_min_um = 0,
        .depth_max_um = 100000,     // 100 mm depth range
        .depth_planes = 256,
        .input_width = 2048,
        .input_height = 2048,
        .output_width = 3840,
        .output_height = 2160,
        .fft_size = 4096,
        .propagation_model = CGH_PROP_ANGULAR_SPECTRUM,
        .phase_quantize_bits = 8,
        .color_channel = CGH_COLOR_RED,
        .pixel_pitch_um_x100 = 360,  // 3.6 µm SLM pixel pitch
        .fill_factor_percent = 92
    };
    cgh_pipeline_configure(&red_params, 0);

    // Green channel (param set 1)
    cgh_params_t green_params = red_params;
    green_params.wavelength_nm_x1000 = CGH_WAVELENGTH_GREEN;
    green_params.color_channel = CGH_COLOR_GREEN;
    cgh_pipeline_configure(&green_params, 1);

    // Blue channel (param set 2)
    cgh_params_t blue_params = red_params;
    blue_params.wavelength_nm_x1000 = CGH_WAVELENGTH_BLUE;
    blue_params.color_channel = CGH_COLOR_BLUE;
    cgh_pipeline_configure(&blue_params, 2);

    // 6. Enable all 8 FFT engines
    cgh_pipeline_set_fft_engine_mask(0xFF);

    // 7. Verify pipeline is idle
    uint32_t cgh_status = cgh_reg_read(PW_REG_CGH_STATUS);
    if (!(cgh_status & CGH_STATUS_IDLE)) {
        debug_uart_printf("CGH: Pipeline not idle after init\r\n");
        return -5;
    }

    debug_uart_printf("CGH: Pipeline initialized successfully\r\n");
    debug_uart_printf("CGH: RGB params configured, 8 FFT engines active\r\n");
    return 0;
}

int cgh_pipeline_configure(const cgh_params_t *params, uint8_t param_set)
{
    if (params == NULL || param_set > 15) return -1;

    // Validate parameters
    int validation = cgh_pipeline_validate_params(params);
    if (validation != 0) {
        debug_uart_printf("CGH: Parameter validation failed (code %d)\r\n", validation);
        return validation;
    }

    // Write to FPGA register file
    cgh_write_param_set(param_set, params);

    debug_uart_printf("CGH: Param set %d configured (λ=%lu.%03lunm, planes=%lu)\r\n",
                      param_set,
                      params->wavelength_nm_x1000 / 1000,
                      params->wavelength_nm_x1000 % 1000,
                      params->depth_planes);
    return 0;
}

int cgh_pipeline_start(void)
{
    // Check if pipeline is idle
    uint32_t status = cgh_reg_read(PW_REG_CGH_STATUS);
    if (status & CGH_STATUS_BUSY) {
        return -1; // Already processing
    }

    // Set start bit in control register
    uint32_t ctrl = cgh_reg_read(PW_REG_CGH_CTRL);
    ctrl |= CGH_CTRL_START;
    cgh_reg_write(PW_REG_CGH_CTRL, ctrl);

    return 0;
}

int cgh_pipeline_abort(void)
{
    uint32_t ctrl = cgh_reg_read(PW_REG_CGH_CTRL);
    ctrl |= CGH_CTRL_ABORT;
    cgh_reg_write(PW_REG_CGH_CTRL, ctrl);

    // Wait for pipeline to return to idle (up to 100 ms)
    for (int retry = 0; retry < 1000; retry++) {
        uint32_t status = cgh_reg_read(PW_REG_CGH_STATUS);
        if (status & CGH_STATUS_IDLE) return 0;
        board_delay_us(100);
    }

    return -1; // Timeout
}

void cgh_pipeline_get_status(cgh_status_t *status)
{
    if (status == NULL) return;

    uint32_t reg_status = cgh_reg_read(PW_REG_CGH_STATUS);

    status->idle         = (reg_status & CGH_STATUS_IDLE) != 0;
    status->busy         = (reg_status & CGH_STATUS_BUSY) != 0;
    status->frame_done   = (reg_status & CGH_STATUS_FRAME_DONE) != 0;
    status->error        = (reg_status & CGH_STATUS_ERROR) != 0;
    status->error_flags  = cgh_reg_read(PW_REG_CGH_ERROR_FLAGS);
    status->frame_count  = cgh_reg_read(PW_REG_CGH_FRAME_COUNT);
    status->frame_time_us = cgh_reg_read(PW_REG_CGH_FRAME_TIME);
    status->fps_x1000    = cgh_reg_read(PW_REG_CGH_FPS);

    // Count active engines from FFT fault mask (inverted)
    uint32_t fft_fault = (reg_status >> 16) & 0xFF;
    status->active_engines = 8 - __builtin_popcount(fft_fault);
}

int cgh_pipeline_wait_frame_done(uint32_t timeout_ms)
{
    uint32_t elapsed_us = 0;

    while (elapsed_us < (timeout_ms * 1000)) {
        uint32_t status = cgh_reg_read(PW_REG_CGH_STATUS);

        if (status & CGH_STATUS_FRAME_DONE) {
            // Clear frame done bit
            cgh_reg_write(PW_REG_CGH_STATUS, status & ~CGH_STATUS_FRAME_DONE);
            return 0;
        }

        if (status & CGH_STATUS_ERROR) {
            uint32_t errors = cgh_reg_read(PW_REG_CGH_ERROR_FLAGS);
            debug_uart_printf("CGH: Frame error (flags=0x%04lX)\r\n", errors);
            return -1;
        }

        board_delay_us(100);
        elapsed_us += 100;
    }

    debug_uart_printf("CGH: Frame timeout after %lu ms\r\n", timeout_ms);
    return -2;
}

int cgh_pipeline_submit_depth_map(uint64_t ddr4_addr, uint32_t width,
                                   uint32_t height, uint32_t color_channel)
{
    if (ddr4_addr & 0x3F) return -1; // Must be 64-byte aligned
    if (color_channel > 2) return -2;

    // Write input dimensions
    cgh_reg_write(PW_REG_CGH_INPUT_W, width);
    cgh_reg_write(PW_REG_CGH_INPUT_H, height);

    // Select parameter set for this color channel
    uint32_t ctrl = cgh_reg_read(PW_REG_CGH_CTRL);
    ctrl &= ~CGH_CTRL_PARAM_SET_MASK;
    ctrl |= (color_channel << CGH_CTRL_PARAM_SET_SHIFT);
    cgh_reg_write(PW_REG_CGH_CTRL, ctrl);

    // Configure DMA to read depth map from DDR4
    // DMA source = ddr4_addr, destination = CGH pipeline input FIFO
    cgh_reg_write(PW_REG_DMA_SRC_ADDR_LO, (uint32_t)(ddr4_addr & 0xFFFFFFFF));
    cgh_reg_write(PW_REG_DMA_SRC_ADDR_HI, (uint32_t)(ddr4_addr >> 32));
    cgh_reg_write(PW_REG_DMA_LENGTH, width * height * 2); // 16-bit depth values

    // Start pipeline
    return cgh_pipeline_start();
}

int cgh_pipeline_retrieve_hologram(uint64_t *ddr4_addr)
{
    if (ddr4_addr == NULL) return -1;

    // Check frame done
    uint32_t status = cgh_reg_read(PW_REG_CGH_STATUS);
    if (!(status & CGH_STATUS_FRAME_DONE)) {
        return -2; // No frame ready
    }

    // Determine which hologram buffer is active (ping-pong)
    uint32_t buf_sel = cgh_reg_read(PW_REG_SCRATCH_0);
    if (buf_sel == 0) {
        *ddr4_addr = DDR4_PART_HOLOGRAM_0;
    } else {
        *ddr4_addr = DDR4_PART_HOLOGRAM_1;
    }

    // Clear frame done
    cgh_reg_write(PW_REG_CGH_STATUS, status & ~CGH_STATUS_FRAME_DONE);

    return 0;
}

void cgh_pipeline_set_fft_engine_mask(uint8_t mask)
{
    uint32_t ctrl = cgh_reg_read(PW_REG_CGH_CTRL);
    ctrl &= ~CGH_CTRL_FFT_ENGINE_MASK;
    ctrl |= ((uint32_t)mask << CGH_CTRL_FFT_ENGINE_SHIFT);
    cgh_reg_write(PW_REG_CGH_CTRL, ctrl);
}

int cgh_pipeline_soft_reset(void)
{
    // Assert soft reset via main control register
    uint32_t ctrl = cgh_reg_read(PW_REG_CONTROL);
    ctrl |= PW_CTRL_SOFT_RESET_CGH;
    cgh_reg_write(PW_REG_CONTROL, ctrl);

    // Wait for reset to propagate
    board_delay_us(100);

    // Release reset
    ctrl &= ~PW_CTRL_SOFT_RESET_CGH;
    cgh_reg_write(PW_REG_CONTROL, ctrl);

    // Wait for pipeline to return to idle
    for (int retry = 0; retry < 100; retry++) {
        uint32_t status = cgh_reg_read(PW_REG_CGH_STATUS);
        if (status & CGH_STATUS_IDLE) return 0;
        board_delay_us(100);
    }

    return -1;
}

uint32_t cgh_pipeline_recommended_fft_size(uint32_t output_width,
                                            uint32_t output_height)
{
    // FFT size must be power of 2 and ≥ max(output_width, output_height)
    // For 4K output (3840×2160), need 4096
    uint32_t max_dim = (output_width > output_height) ? output_width : output_height;

    // Find next power of 2
    uint32_t fft_size = 64; // Minimum
    while (fft_size < max_dim) {
        fft_size <<= 1;
    }

    // Cap at 8192 (hardware limit)
    if (fft_size > 8192) fft_size = 8192;

    return fft_size;
}

int cgh_pipeline_validate_params(const cgh_params_t *params)
{
    if (params == NULL) return -10;

    // Wavelength must be in visible range (380-780 nm)
    if (params->wavelength_nm_x1000 < 380000 ||
        params->wavelength_nm_x1000 > 780000) {
        return -11;
    }

    // Depth range must be valid
    if (params->depth_max_um <= params->depth_min_um) {
        return -12;
    }

    // Depth planes: 1-256
    if (params->depth_planes < 1 || params->depth_planes > 256) {
        return -13;
    }

    // Input dimensions: 64-4096
    if (params->input_width < 64 || params->input_width > 4096 ||
        params->input_height < 64 || params->input_height > 4096) {
        return -14;
    }

    // Output dimensions: 64-4096
    if (params->output_width < 64 || params->output_width > 4096 ||
        params->output_height < 64 || params->output_height > 4096) {
        return -15;
    }

    // FFT size must be power of 2
    if (params->fft_size & (params->fft_size - 1)) {
        return -16;
    }

    // FFT size must be ≥ output dimensions
    if (params->fft_size < params->output_width ||
        params->fft_size < params->output_height) {
        return -17;
    }

    // FFT size must be ≤ 8192
    if (params->fft_size > 8192) {
        return -18;
    }

    // Propagation model: 0-2
    if (params->propagation_model > 2) {
        return -19;
    }

    // Phase quantize bits: 8 or 10
    if (params->phase_quantize_bits != 8 &&
        params->phase_quantize_bits != 10) {
        return -20;
    }

    // Color channel: 0-2
    if (params->color_channel > 2) {
        return -21;
    }

    // Pixel pitch: 1.0-20.0 µm (100-2000 in our units)
    if (params->pixel_pitch_um_x100 < 100 ||
        params->pixel_pitch_um_x100 > 2000) {
        return -22;
    }

    // Fill factor: 0-100
    if (params->fill_factor_percent > 100) {
        return -23;
    }

    return 0; // All valid
}
