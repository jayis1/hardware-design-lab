/**
 * hub75e_spi_driver.c — SPI Pixel Stream Driver Implementation
 *
 * DMA-driven SPI2 communication from STM32H743 to FPGA pixel input FIFO.
 * Transmits 48-bit pixels at 50 MHz with automatic buffering and flush.
 *
 * SPI Protocol (32-bit words, MSB first):
 *   Word 0: [31:28]=CMD_PIXEL(0x1) [27:16]=X[11:0] [15:0]=Y[15:0]
 *   Word 1: [31:16]=R[15:0] [15:0]=G[15:0]
 *   Word 2: [31:16]=B[15:0] [15:0]=Reserved(0)
 *
 * Frame markers:
 *   FRAME_START: [31:28]=0x3, rest=0
 *   FRAME_END:   [31:28]=0x2, rest=0
 */

#include "hub75e_spi_driver.h"
#include "registers.h"
#include <string.h>

/* =========================================================================
 * Static Variables
 * ========================================================================= */

static SPI_HandleTypeDef *g_hspi2 = NULL;
static DMA_HandleTypeDef g_hdma_spi2_tx;

/* Transmit buffer: 2048 32-bit words = 8 KB */
static uint32_t g_spi_tx_buf[SPI_PIXEL_BUF_SIZE];
static volatile uint32_t g_spi_buf_count = 0;
static volatile bool g_spi_transfer_active = false;
static volatile bool g_spi_transfer_error = false;

/* Statistics */
static volatile uint32_t g_pixel_count = 0;
static volatile uint32_t g_frame_count = 0;
static volatile uint32_t g_error_count = 0;
static volatile uint32_t g_overflow_count = 0;

/* =========================================================================
 * Initialization
 * ========================================================================= */

void fpga_spi_init(SPI_HandleTypeDef *hspi)
{
    g_hspi2 = hspi;

    /* Enable SPI2 clock */
    __HAL_RCC_SPI2_CLK_ENABLE();

    /* Configure SPI2: Master, 32-bit, 50 MHz, MSB first, CPOL=0, CPHA=0 */
    hspi->Instance = SPI2;
    hspi->Init.Mode = SPI_MODE_MASTER;
    hspi->Init.Direction = SPI_DIRECTION_2LINES;
    hspi->Init.DataSize = SPI_DATASIZE_32BIT;
    hspi->Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi->Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi->Init.NSS = SPI_NSS_SOFT;
    hspi->Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;  /* 100MHz/2 = 50MHz */
    hspi->Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi->Init.TIMode = SPI_TIMODE_DISABLE;
    hspi->Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi->Init.CRCPolynomial = 7;
    hspi->Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi->Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi->Init.FifoThreshold = SPI_FIFO_THRESHOLD_04DATA;
    hspi->Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi->Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi->Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi->Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi->Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi->Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi->Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(hspi) != HAL_OK) {
        g_error_count++;
        return;
    }

    /* Configure DMA for SPI2 TX (DMA1 Stream 0, Channel 3 for SPI2_TX) */
    __HAL_RCC_DMA1_CLK_ENABLE();

    g_hdma_spi2_tx.Instance = DMA1_Stream0;
    g_hdma_spi2_tx.Init.Request = DMA_REQUEST_SPI2_TX;
    g_hdma_spi2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_hdma_spi2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    g_hdma_spi2_tx.Init.MemInc = DMA_MINC_ENABLE;
    g_hdma_spi2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    g_hdma_spi2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    g_hdma_spi2_tx.Init.Mode = DMA_NORMAL;
    g_hdma_spi2_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    g_hdma_spi2_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    g_hdma_spi2_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    g_hdma_spi2_tx.Init.MemBurst = DMA_MBURST_SINGLE;
    g_hdma_spi2_tx.Init.PeriphBurst = DMA_PBURST_SINGLE;

    if (HAL_DMA_Init(&g_hdma_spi2_tx) != HAL_OK) {
        g_error_count++;
        return;
    }

    /* Link DMA to SPI */
    __HAL_LINKDMA(hspi, hdmatx, g_hdma_spi2_tx);

    /* Configure DMA interrupt */
    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);

    /* Configure SPI interrupt for error handling */
    HAL_NVIC_SetPriority(SPI2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SPI2_IRQn);

    /* Initialize NSS high (deselected) */
    GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);

    /* Clear buffer */
    memset(g_spi_tx_buf, 0, sizeof(g_spi_tx_buf));
    g_spi_buf_count = 0;
    g_spi_transfer_active = false;
    g_spi_transfer_error = false;
}

/* =========================================================================
 * Pixel Transmission
 * ========================================================================= */

int fpga_spi_send_pixel(uint16_t x, uint16_t y,
                         uint16_t r, uint16_t g, uint16_t b)
{
    /* Check if buffer has room for 3 more words */
    if (g_spi_buf_count >= SPI_PIXEL_BUF_SIZE - 3) {
        /* Buffer nearly full — flush it */
        fpga_spi_flush_buffer();

        /* If flush failed or buffer still full, report overflow */
        if (g_spi_buf_count >= SPI_PIXEL_BUF_SIZE - 3) {
            g_overflow_count++;
            return -1;
        }
    }

    /* Word 0: Command (0x1) + X[11:0] + Y[15:0] */
    g_spi_tx_buf[g_spi_buf_count++] = (SPI_CMD_PIXEL << 28)
                                    | ((x & 0x0FFF) << 16)
                                    | (y & 0xFFFF);

    /* Word 1: R[15:0] + G[15:0] */
    g_spi_tx_buf[g_spi_buf_count++] = ((uint32_t)r << 16) | (g & 0xFFFF);

    /* Word 2: B[15:0] + Reserved */
    g_spi_tx_buf[g_spi_buf_count++] = ((uint32_t)b << 16);

    g_pixel_count++;

    return 0;
}

/* =========================================================================
 * Frame Markers
 * ========================================================================= */

void fpga_spi_send_frame_start(void)
{
    /* Flush any pending pixels first */
    fpga_spi_flush_buffer();

    /* Send frame start command */
    g_spi_tx_buf[0] = (SPI_CMD_FRAME_START << 28);
    g_spi_buf_count = 1;
    fpga_spi_flush_buffer();
}

void fpga_spi_send_frame_end(void)
{
    /* Flush any pending pixels first */
    fpga_spi_flush_buffer();

    /* Send frame end command */
    g_spi_tx_buf[0] = (SPI_CMD_FRAME_END << 28);
    g_spi_buf_count = 1;
    fpga_spi_flush_buffer();

    g_frame_count++;
}

/* =========================================================================
 * Buffer Flush (DMA Transfer)
 * ========================================================================= */

void fpga_spi_flush_buffer(void)
{
    uint32_t count;

    if (g_spi_buf_count == 0) {
        return;  /* Nothing to flush */
    }

    /* Wait for any previous transfer to complete (with timeout) */
    uint32_t timeout = HAL_GetTick() + SPI_DMA_TIMEOUT_MS;
    while (g_spi_transfer_active) {
        if (HAL_GetTick() > timeout) {
            /* Timeout — force abort */
            HAL_SPI_Abort(g_hspi2);
            g_spi_transfer_active = false;
            g_spi_transfer_error = true;
            g_error_count++;
            GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
            return;
        }
    }

    /* Save count and reset buffer */
    count = g_spi_buf_count;
    g_spi_buf_count = 0;
    g_spi_transfer_active = true;
    g_spi_transfer_error = false;

    /* Assert NSS (active low) */
    GPIO_CLR(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);

    /* Small delay for NSS setup time (12.5 ns at 50 MHz — 1 cycle is enough) */
    __NOP();

    /* Start DMA transfer */
    HAL_StatusTypeDef status = HAL_SPI_Transmit_DMA(g_hspi2, g_spi_tx_buf, count);

    if (status != HAL_OK) {
        /* DMA start failed */
        g_spi_transfer_active = false;
        g_error_count++;
        GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
    }
}

/* =========================================================================
 * Status Functions
 * ========================================================================= */

bool fpga_spi_is_busy(void)
{
    return g_spi_transfer_active;
}

uint32_t fpga_spi_get_pixel_count(void)
{
    return g_pixel_count;
}

uint32_t fpga_spi_get_error_count(void)
{
    return g_error_count;
}

void fpga_spi_reset_counters(void)
{
    g_pixel_count = 0;
    g_frame_count = 0;
    g_error_count = 0;
    g_overflow_count = 0;
}

/* =========================================================================
 * DMA Transfer Complete Callback
 * ========================================================================= */

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        /* De-assert NSS */
        GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
        g_spi_transfer_active = false;
    }
}

void HAL_SPI_TxHalfCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        /* Half-transfer complete — NSS stays asserted.
         * This is normal for DMA streaming. */
    }
}

/* =========================================================================
 * DMA / SPI Error Callbacks
 * ========================================================================= */

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        /* Abort transfer on error */
        HAL_SPI_Abort_IT(hspi);
        GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
        g_spi_transfer_active = false;
        g_spi_transfer_error = true;
        g_error_count++;
    }
}

void HAL_SPI_AbortCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
        g_spi_transfer_active = false;
    }
}

void HAL_DMA_ErrorCallback(DMA_HandleTypeDef *hdma)
{
    if (hdma->Instance == DMA1_Stream0) {
        /* DMA error on SPI2 TX stream */
        HAL_SPI_Abort_IT(g_hspi2);
        g_spi_transfer_active = false;
        g_spi_transfer_error = true;
        g_error_count++;
    }
}

/* =========================================================================
 * SPI2 Interrupt Handler
 * ========================================================================= */

void SPI2_IRQHandler(void)
{
    HAL_SPI_IRQHandler(g_hspi2);
}

/* =========================================================================
 * DMA1 Stream 0 Interrupt Handler
 * ========================================================================= */

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_hdma_spi2_tx);
}

/* =========================================================================
 * Advanced: Burst Pixel Transmission
 * ========================================================================= */

/**
 * Send multiple pixels in a single buffer flush.
 * Useful for row-based or block-based pixel streaming.
 *
 * @param pixels  Array of pixel data
 * @param count   Number of pixels to send
 * @return Number of pixels successfully queued, -1 on error
 */
int fpga_spi_send_pixel_burst(const pixel_data_t *pixels, uint32_t count)
{
    uint32_t queued = 0;

    for (uint32_t i = 0; i < count; i++) {
        if (fpga_spi_send_pixel(pixels[i].x, pixels[i].y,
                                 pixels[i].r, pixels[i].g, pixels[i].b) != 0) {
            break;
        }
        queued++;
    }

    /* Flush remaining pixels */
    fpga_spi_flush_buffer();

    return queued;
}

/**
 * Send a complete row of pixels.
 * Optimized for HUB75E row-based scanning.
 *
 * @param y       Row number
 * @param width   Number of pixels in row
 * @param r_data  Red channel data array
 * @param g_data  Green channel data array
 * @param b_data  Blue channel data array
 * @return 0 on success, -1 on error
 */
int fpga_spi_send_row(uint16_t y, uint16_t width,
                       const uint16_t *r_data,
                       const uint16_t *g_data,
                       const uint16_t *b_data)
{
    for (uint16_t x = 0; x < width; x++) {
        if (fpga_spi_send_pixel(x, y, r_data[x], g_data[x], b_data[x]) != 0) {
            return -1;
        }
    }
    fpga_spi_flush_buffer();
    return 0;
}

/**
 * Send a complete frame of pixels.
 * Sends FRAME_START, all pixels row by row, then FRAME_END.
 *
 * @param width   Frame width
 * @param height  Frame height
 * @param r_data  Red channel data (width*height elements)
 * @param g_data  Green channel data
 * @param b_data  Blue channel data
 * @return 0 on success, -1 on error
 */
int fpga_spi_send_frame(uint16_t width, uint16_t height,
                         const uint16_t *r_data,
                         const uint16_t *g_data,
                         const uint16_t *b_data)
{
    fpga_spi_send_frame_start();

    for (uint16_t y = 0; y < height; y++) {
        uint32_t offset = (uint32_t)y * width;
        if (fpga_spi_send_row(y, width,
                               &r_data[offset],
                               &g_data[offset],
                               &b_data[offset]) != 0) {
            return -1;
        }
    }

    fpga_spi_send_frame_end();
    return 0;
}
