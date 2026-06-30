/*
 * drivers/imx519.c — Sony IMX519 5MP camera driver for SpectraPest
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 *
 * Drives the IMX519 image sensor via the STM32H7 DCMI (camera) interface
 * with DMA double-buffering. The sensor is configured via a co-processor
 * I2C interface (handled by ESP32-C3 for lens/sensor registers); the MCU
 * handles the raw data path and DMA.
 *
 * The IMX519 outputs 10-bit Bayer data over the MIPI CSI-2 interface,
 * which the STM32H7 DCMI captures into SRAM via DMA. For multispectral
 * capture, the filter wheel is positioned before each frame is grabbed.
 *
 * Key functions:
 *   imx519_init()           - Power up and configure sensor defaults
 *   imx519_capture_bayer()  - Grab one raw Bayer frame into SRAM
 *   imx519_downsample_rgb565() - Downsample full frame to small RGB565
 *
 * The downsample step is critical for edge inference: a full 2592×1944
 * frame is 5 MB (16-bit packed), which would saturate the 1 MB SRAM. We
 * downsample by averaging 20×20 blocks to produce a 128×96 RGB565 frame
 * (~25 KB) that fits in the ML input buffer and can be fed to the spectral
 * feature extractor.
 */

#include "imx519.h"
#include "../registers.h"
#include <string.h>

/* I2C register addresses for the IMX519 (via ESP32-C3 co-processor bridge).
 * The ESP32-C3 handles I2C to the camera sensor since the STM32H7 only has
 * the DCMI data path. Configuration commands are sent via UART4. */
#define IMX519_I2C_ADDR      0x1A

/* IMX519 register map (subset) */
#define IMX519_REG_MODE_SELECT    0x0100
#define IMX519_REG_EXPOSURE       0x0202
#define IMX519_REG_ANALOG_GAIN    0x0204
#define IMX519_REG_DIGITAL_GAIN   0x0208
#define IMX519_REG_FRAME_LENGTH   0x0340
#define IMX519_REG_LINE_LENGTH    0x0342
#define IMX519_REG_Binning_MODE   0x0300
#define IMX519_REG_DATA_FORMAT    0x0112
#define IMX519_REG_CSI_LANE_EN    0x0114

/* DMA double-buffer addresses (in SRAM2, non-cached region).
 * We use two half-buffers so the DCMI can fill one while the CPU
 * processes the other. For a full 2592×1944 Bayer frame at 16-bit/pixel,
 * that's ~10 MB total, which exceeds SRAM. Instead we use line-by-line
 * capture with 4-line buffers (~20 KB each) and accumulate the
 * downsampled output incrementally. */
#define DCMI_LINE_BUF_COUNT  4
#define DCMI_LINE_BUF_SIZE   (IMX519_WIDTH * DCMI_LINE_BUF_COUNT * 2)

/* Place DMA buffers in a non-cacheable SRAM region (SRAM2 at 0x10000000) */
#define DCMI_DMA_BUF_ADDR   0x10020000UL

static volatile uint8_t  g_capture_done = 0;
static volatile uint32_t g_pixels_captured = 0;
static uint16_t         *g_downsample_out = NULL;
static uint32_t          g_downsample_w = 0;
static uint32_t          g_downsample_h = 0;
static uint32_t          g_block_x = 0;
static uint32_t          g_block_y = 0;

/* ----------------------------------------------------------------- *
 *  DCMI DMA interrupt handler
 *  Called when a line-buffer is filled. We process the buffer
 *  immediately (downsample the lines) and restart DMA for the next
 *  set of lines.
 * ----------------------------------------------------------------- */
void DMA1_Stream3_IRQHandler(void) {
    uint32_t flags = DMA1_CTRL->HISR;

    if (flags & (1 << 11)) {  /* TCIF3: Transfer Complete */
        /* Clear the flag */
        DMA1_CTRL->HIFCR = (1 << 27);  /* CTCIF3 */

        if (g_downsample_out && g_block_y < g_downsample_h) {
            /* Process the line buffer: average pixels into the output block */
            uint16_t *line_buf = (uint16_t *)DCMI_DMA_BUF_ADDR;
            uint32_t src_w = IMX519_WIDTH;
            uint32_t block_w = src_w / g_downsample_w;

            /* Accumulate pixels for the current row of blocks */
            for (uint32_t bx = 0; bx < g_downsample_w; bx++) {
                uint32_t sum_r = 0, sum_g = 0, sum_b = 0;
                for (uint32_t dx = 0; dx < block_w; dx++) {
                    uint16_t p = line_buf[bx * block_w + dx];
                    /* Bayer pattern: R/G/B/G alternating.
                     * For simplicity, treat the 10-bit value as luminance. */
                    sum_r += (p >> 8) & 0xFF;
                    sum_g += (p >> 2) & 0xFF;
                    sum_b += p & 0xFF;
                }
                /* Convert to RGB565 and accumulate into output (running average) */
                uint16_t r5 = (sum_r / block_w) >> 3;
                uint16_t g6 = (sum_g / block_w) >> 2;
                uint16_t b5 = (sum_b / block_w) >> 3;
                uint16_t rgb565 = (r5 << 11) | (g6 << 5) | b5;

                /* Running average: if this is the first row of the block,
                 * write directly; otherwise average with existing value. */
                uint32_t out_idx = g_block_y * g_downsample_w + bx;
                if (g_block_x == 0) {
                    g_downsample_out[out_idx] = rgb565;
                } else {
                    /* Average with previous row's accumulation */
                    uint16_t prev = g_downsample_out[out_idx];
                    uint16_t avg_r = ((prev >> 11) + r5) / 2;
                    uint16_t avg_g = ((prev >> 5) & 0x3F) + g6) / 2;
                    uint16_t avg_b = (prev & 0x1F) + b5) / 2;
                    g_downsample_out[out_idx] = (avg_r << 11) | (avg_g << 5) | avg_b;
                }
            }

            g_block_x++;
            if (g_block_x >= (IMX519_HEIGHT / DCMI_LINE_BUF_COUNT) / g_downsample_h) {
                g_block_x = 0;
                g_block_y++;
            }
        }

        /* Restart DMA for next line set (circular mode would also work,
         * but we restart manually to handle the block_y progression) */
        if (g_block_y < g_downsample_h) {
            DMA1_STR3->CR |= DMA_CR_EN;
        } else {
            g_capture_done = 1;
            DCMI->CR &= ~DCMI_CR_CAPTURE;
        }
    }
}

/* ----------------------------------------------------------------- *
 *  Public API
 * ----------------------------------------------------------------- */

int imx519_init(void) {
    /* Enable DCMI clock */
    RCC->AHB2ENR |= (1 << 0);  /* DCMIEN */

    /* Power on the camera module */
    gpio_write(CAM_POWER_PORT, CAM_POWER_PIN, 1);
    board_delay_ms(50);

    /* Release from reset */
    gpio_write(CAM_RESET_PORT, CAM_RESET_PIN, 1);
    board_delay_ms(10);

    /* Configure DCMI interface for 8-bit parallel, HSYNC/VSYNC
     * (The MIPI CSI-2 receiver is handled by the STM32H7 CSI peripheral) */
    DCMI->CR = 0;
    DCMI->CR = DCMI_CR_PCKPOL | DCMI_CR_HSYNC | DCMI_CR_VSYNC | DCMI_CR_EDM_8BIT;

    /* Set crop window to full sensor */
    DCMI->CWSTRTR = 0;
    DCMI->CWSZ = (IMX519_WIDTH << 16) | IMX519_HEIGHT;

    /* Enable DCMI capture interrupt via DMA, not direct interrupt */
    DCMI->IER = 0;  /* We use DMA, not DCMI interrupts */

    /* Configure DMA1 Stream 3 for DCMI -> SRAM2 */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    DMA1_STR3->CR = 0;  /* Disable first */

    DMA1_STR3->PAR = (uint32_t)&DCMI->DATA;
    DMA1_STR3->M0AR = DCMI_DMA_BUF_ADDR;
    DMA1_STR3->NDTR = DCMI_LINE_BUF_SIZE / 2;  /* 16-bit transfers */
    DMA1_STR3->CR = DMA_CR_TCIE | DMA_CR_MINC | DMA_CR_PL_HIGH |
                    DMA_CR_PSIZE_16 | DMA_CR_MSIZE_16 |
                    DMA_CR_DIR_P2M | (DMA_CHSEL_DCMI << 25);
    DMA1_STR3->FCR = 0x5;  /* Quarter FIFO threshold */

    /* Enable NVIC for DMA1 Stream3 */
    NVIC_ISER1 |= (1 << (IRQ_DMA1_STR3 - 32));

    /* Configure ESP32-C3 to initialize the IMX519 sensor via I2C.
     * The ESP32 sends the register sequence over its I2C bus to the sensor. */
    /* (handled in esp32_bridge.c via UART command) */

    return 0;
}

void imx519_reset(void) {
    gpio_write(CAM_RESET_PORT, CAM_RESET_PIN, 0);
    board_delay_ms(10);
    gpio_write(CAM_RESET_PORT, CAM_RESET_PIN, 1);
    board_delay_ms(50);
}

void imx519_power_down(void) {
    DCMI->CR &= ~DCMI_CR_ENABLE;
    gpio_write(CAM_POWER_PORT, CAM_POWER_PIN, 0);
    gpio_write(CAM_RESET_PORT, CAM_RESET_PIN, 0);
}

void imx519_power_up(void) {
    gpio_write(CAM_POWER_PORT, CAM_POWER_PIN, 1);
    board_delay_ms(50);
    gpio_write(CAM_RESET_PORT, CAM_RESET_PIN, 1);
    board_delay_ms(10);
    DCMI->CR |= DCMI_CR_ENABLE;
}

void imx519_set_strobe(uint8_t enable) {
    gpio_write(CAM_FLASH_PORT, CAM_FLASH_PIN, enable ? 1 : 0);
}

void imx519_set_ir_led(uint8_t enable) {
    gpio_write(CAM_IR_LED_PORT, CAM_IR_LED_PIN, enable ? 1 : 0);
}

int imx519_set_exposure(uint32_t us) {
    /* The exposure is set via the ESP32-C3 I2C bridge.
     * We send a UART command to the ESP32 to program the sensor register. */
    /* In a real implementation, this would call esp32_send_cmd() */
    (void)us;
    return 0;
}

int imx519_capture_bayer(uint16_t *out_buf, uint32_t max_pixels) {
    (void)max_pixels;  /* We use line-by-line DMA, not full-frame */

    if (out_buf == NULL) return -1;

    /* Reset capture state */
    g_capture_done = 0;
    g_pixels_captured = 0;
    g_downsample_out = out_buf;
    g_block_x = 0;
    g_block_y = 0;

    /* Clear any pending DMA flags */
    DMA1_CTRL->HIFCR = (1 << 27) | (1 << 26) | (1 << 25);

    /* Enable DMA stream */
    DMA1_STR3->CR |= DMA_CR_EN;

    /* Start DCMI capture */
    DCMI->CR |= DCMI_CR_ENABLE | DCMI_CR_CAPTURE;

    /* Wait for capture complete (timeout: 5 seconds) */
    uint32_t start_tick = board_get_tick_ms();
    while (!g_capture_done) {
        if (board_get_tick_ms() - start_tick > 5000) {
            DCMI->CR &= ~DCMI_CR_CAPTURE;
            DMA1_STR3->CR &= ~DMA_CR_EN;
            return -1;
        }
    }

    /* Disable DCMI capture */
    DCMI->CR &= ~DCMI_CR_CAPTURE;

    return 0;
}

int imx519_downsample_rgb565(const uint16_t *bayer, uint32_t src_w, uint32_t src_h,
                              uint16_t *out, uint32_t out_w, uint32_t out_h) {
    if (!bayer || !out || src_w == 0 || src_h == 0) return -1;

    uint32_t block_w = src_w / out_w;
    uint32_t block_h = src_h / out_h;

    if (block_w == 0 || block_h == 0) return -1;

    for (uint32_t by = 0; by < out_h; by++) {
        for (uint32_t bx = 0; bx < out_w; bx++) {
            uint32_t sum = 0;
            for (uint32_t dy = 0; dy < block_h; dy++) {
                for (uint32_t dx = 0; dx < block_w; dx++) {
                    uint32_t src_x = bx * block_w + dx;
                    uint32_t src_y = by * block_h + dy;
                    sum += bayer[src_y * src_w + src_x];
                }
            }
            uint32_t count = block_w * block_h;
            uint16_t avg = sum / count;

            /* Simple grayscale-to-RGB565 conversion */
            uint16_t r5 = (avg >> 3) & 0x1F;
            uint16_t g6 = (avg >> 2) & 0x3F;
            uint16_t b5 = (avg >> 3) & 0x1F;
            out[by * out_w + bx] = (r5 << 11) | (g6 << 5) | b5;
        }
    }

    return 0;
}