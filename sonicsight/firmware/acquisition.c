/**
 * @file    acquisition.c
 * @brief   SonicSight — Acquisition module implementation.
 *          Handles transducer firing, ADC capture via DCMI/DMA,
 *          mux routing, and coupling quality checks.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 */

#include <string.h>
#include <math.h>
#include "acquisition.h"
#include "board.h"
#include "registers.h"

/* External HAL handles — defined in main.c */
extern I2C_HandleTypeDef hi2c1;
extern DCMI_HandleTypeDef hdcmi1;

/* SDRAM pointer for waveform storage */
static volatile uint16_t *sdram_waveform_base = (volatile uint16_t *)SDRAM_BASE;

/* ========================================================================== */
/*  Initialisation                                                            */
/* ========================================================================== */

void acquisition_init(acquisition_ctx_t *ctx)
{
    memset(ctx, 0, sizeof(acquisition_ctx_t));

    /* Allocate ADC buffers from SDRAM (8 channels × 1024 samples × 2 bytes) */
    for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        ctx->adc_buffers[ch] = (int16_t *)(sdram_waveform_base + (ch * ADC_SAMPLES_PER_TRACE));
    }

    /* Allocate raw LVDS deserialisation buffer */
    ctx->raw_lvds_buffer = (int16_t *)(sdram_waveform_base + (ADC_NUM_CHANNELS * ADC_SAMPLES_PER_TRACE));

    /* Generate default transmit pulse template: 5 cycles at 50 kHz, 50 MSPS */
    ctx->tx_template_len = ADC_SAMPLES_PER_TRACE;
    ctx->tx_template = ctx->adc_buffers[0]; /* reuse — will be overwritten */
    for (uint32_t i = 0; i < ADC_SAMPLES_PER_TRACE; i++) {
        float t = (float)i / (float)ADC_SAMPLE_RATE; /* time in seconds */
        /* 5-cycle 50 kHz burst, windowed by Hanning to reduce spectral leakage */
        float window = 0.5f * (1.0f - cosf(2.0f * 3.14159265f * (float)i / (float)ADC_SAMPLES_PER_TRACE));
        float signal = window * sinf(2.0f * 3.14159265f * TX_FREQ_HZ_DEFAULT * t);
        /* Saturation: burst lasts only 5 cycles, rest is zeros */
        if (t > (float)TX_BURST_CYCLES_DEFAULT / (float)TX_FREQ_HZ_DEFAULT) {
            signal = 0.0f;
        }
        ctx->tx_template[i] = (int16_t)(signal * 1800.0f); /* 12-bit amplitude */
    }

    /* Initialise DCMI for LVDS deserialisation from ADS5281 */
    MX_DCMI_Init();
}

void MX_DCMI_Init(void)
{
    /* DCMI is configured for 8-bit parallel mode (one LVDS lane from ADC) */
    /* PIXCK = PI6 (ADC bit clock), D0 = PI7 (ADC serial data) */
    hdcmi1.Instance = DCMI;
    hdcmi1.Init.SynchroMode = DCMI_SYNCHRO_HARDWARE;
    hdcmi1.Init.PCKPolarity = DCMI_PCKPOLARITY_FALLING;
    hdcmi1.Init.VSPolarity = DCMI_VSPOLARITY_HIGH;
    hdcmi1.Init.HSPolarity = DCMI_HSPOLARITY_HIGH;
    hdcmi1.Init.CaptureRate = DCMI_CR_ALL_FRAME;
    hdcmi1.Init.ExtendedDataMode = DCMI_EXTEND_DATA_8B;
    hdcmi1.Init.JPEGMode = DCMI_JPEG_DISABLE;
    hdcmi1.Init.ByteSelectMode = DCMI_BSM_ALL;
    hdcmi1.Init.ByteSelectStart = DCMI_OEBS_ODD;
    hdcmi1.Init.LineSelectMode = DCMI_LSM_ALL;
    hdcmi1.Init.LineSelectStart = DCMI_OEBS_ODD;
    HAL_DCMI_Init(&hdcmi1);
}

/* ========================================================================== */
/*  Coupling Quality Check                                                    */
/* ========================================================================== */

int32_t acquisition_check_coupling(acquisition_ctx_t *ctx,
                                    const uint8_t *active,
                                    uint8_t num_sens)
{
    int32_t good_mask = 0;
    int32_t min_good = 0;
    const float coupling_threshold = 0.5f; /* 50% of reference amplitude */

    /* Fire a low-voltage test pulse (20 V, 1 cycle) on each active sensor
     * and measure the near-surface echo return.
     */
    for (uint8_t i = 0; i < num_sens; i++) {
        if (!active[i]) continue;

        /* Route pulse to this sensor */
        acquisition_select_tx_channel(i);

        /* Configure mux to listen on the same sensor for echo */
        /* For coupling check, we use a special mux config: TX sensor → RX channel 0 */
        HAL_GPIO_WritePin(RX_MUX0_EN_PORT, RX_MUX0_EN_PIN, GPIO_PIN_RESET);
        /* Set mux address to channel i */
        HAL_GPIO_WritePin(RX_MUX0_SEL_PORT,
                          RX_MUX0_SEL0_PIN | RX_MUX0_SEL1_PIN | RX_MUX0_SEL2_PIN
                        | RX_MUX0_SEL3_PIN | RX_MUX0_SEL4_PIN,
                          (uint32_t)(i & ADG732_ADDR_MASK));

        delay_us(10);

        /* Fire low-voltage test pulse (20 V, 1 cycle) */
        /* TIM1: ARR = 4800 for 50 kHz → 1 cycle = 20 µs */
        /* First configure for low amplitude via duty cycle */
        TIM1->CCR1 = 160;  /* ~3.3% duty → ~20 V */
        TIM1->CCR2 = 160;  /* complementary */
        TIM1->BDTR |= TIM_BDTR_MOE;
        TIM1->CR1 |= TIM_CR1_CEN;
        /* Generate 1 cycle: TIM1 counter runs for 1 period */
        while (TIM1->CNT < TIM1->ARR) { /* wait for one full period */ }
        TIM1->CR1 &= ~TIM_CR1_CEN;

        /* Capture echo on ADC channel 0 */
        ctx->capture_done = 0;
        /* Enable TIM8 trigger — single pulse */
        TIM8->CR1 |= TIM_CR1_CEN;
        /* Wait for DCMI capture complete */
        uint32_t timeout = 10000;
        while (!ctx->capture_done && timeout--) { delay_us(1); }
        TIM8->CR1 &= ~TIM_CR1_CEN;

        if (!ctx->capture_done) {
            continue; /* No echo received — bad coupling */
        }

        /* Measure peak-to-peak amplitude of echo in receive buffer */
        int16_t *buf = ctx->adc_buffers[0];
        int16_t min_val = 2047, max_val = -2048;
        for (uint32_t s = 200; s < ADC_SAMPLES_PER_TRACE - 50; s++) {
            /* Skip first 200 samples (pulse crosstalk settling) */
            if (buf[s] < min_val) min_val = buf[s];
            if (buf[s] > max_val) max_val = buf[s];
        }
        float amp = (float)(max_val - min_val);

        /* Reference: good coupling on acrylic phantom gives ~1000 counts */
        float ratio = amp / 1000.0f;

        if (ratio >= coupling_threshold) {
            good_mask |= (1 << i);
            min_good++;
        }
    }

    if (min_good < 3) {
        /* Fewer than 3 good sensors — can't reconstruct */
        return ERR_BAD_COUPLING;
    }

    return good_mask;
}

/* ========================================================================== */
/*  Gain & Filter Configuration                                               */
/* ========================================================================== */

void acquisition_set_gains(acquisition_ctx_t *ctx, const float *gain_db)
{
    for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
        float gain = gain_db ? gain_db[ch] : (float)VGA_GAIN_DEFAULT_DB;
        if (gain < VGA_GAIN_MIN_DB) gain = (float)VGA_GAIN_MIN_DB;
        if (gain > VGA_GAIN_MAX_DB) gain = (float)VGA_GAIN_MAX_DB;

        /* AD8331 VGA controlled via MCP4821 DAC */
        uint16_t dac_code = AD8331_GAIN_TO_DAC(gain);
        if (dac_code > AD8331_DAC_MAX_CODE) dac_code = AD8331_DAC_MAX_CODE;

        /* SPI write to DAC */
        uint16_t spi_word = MCP4821_WRITE | MCP4821_GAIN_1X | MCP4821_SHDN | (dac_code & 0x0FFF);
        /* Select DAC channel corresponding to this VGA */
        /* We use a daisy-chained MCP4821 per channel; select via individual CS lines */
        /* For simplicity, CS for channel ch is on VGA_DAC_CS_PORT + ch (expanded via decoder) */
        HAL_GPIO_WritePin(VGA_DAC_CS_PORT, GPIO_PIN_0 << ch, GPIO_PIN_RESET);
        HAL_SPI_Transmit(&hspi4, (uint8_t *)&spi_word, 2, 10);
        HAL_GPIO_WritePin(VGA_DAC_CS_PORT, GPIO_PIN_0 << ch, GPIO_PIN_SET);

        ctx->channel_gain[ch] = gain;
    }
}

void acquisition_set_filter_fc(acquisition_ctx_t *ctx, uint32_t fc_hz)
{
    (void)ctx;
    uint8_t fsel;

    if (fc_hz >= 200000)       fsel = LTC1563_FSEL_200K;
    else if (fc_hz >= 100000)  fsel = LTC1563_FSEL_100K;
    else if (fc_hz >= 50000)   fsel = LTC1563_FSEL_50K;
    else if (fc_hz >= 20000)   fsel = LTC1563_FSEL_20K;
    else if (fc_hz >= 10000)   fsel = LTC1563_FSEL_10K;
    else if (fc_hz >= 5000)    fsel = LTC1563_FSEL_5K;
    else if (fc_hz >= 2000)    fsel = LTC1563_FSEL_2K;
    else                        fsel = LTC1563_FSEL_1K;

    /* SPI write to LTC1563-2: control byte + frequency select */
    uint8_t spi_data[2] = { LTC1563_CTRL_WRITE, fsel };
    HAL_GPIO_WritePin(FILTER_CS_PORT, FILTER_CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi6, spi_data, 2, 10);
    HAL_GPIO_WritePin(FILTER_CS_PORT, FILTER_CS_PIN, GPIO_PIN_SET);
}

/* ========================================================================== */
/*  ADC Preparation                                                          */
/* ========================================================================== */

void acquisition_prepare_adc(acquisition_ctx_t *ctx)
{
    (void)ctx;

    /* 1. Configure ADS5281 via SPI: enable PLL, set LVDS clock output */
    HAL_GPIO_WritePin(ADC_CS_PORT, ADC_CS_PIN, GPIO_PIN_RESET);
    uint16_t cmd;
    cmd = ADS5281_SPI_CMD(ADS5281_SPI_WRITE, ADS5281_REG_CLOCK,
                          ADS5281_CLOCK_LVDS_EN);
    HAL_SPI_Transmit(&hspi5, (uint8_t *)&cmd, 2, 10);

    cmd = ADS5281_SPI_CMD(ADS5281_SPI_WRITE, ADS5281_REG_PLL,
                          ADS5281_PLL_EN);
    HAL_SPI_Transmit(&hspi5, (uint8_t *)&cmd, 2, 10);

    cmd = ADS5281_SPI_CMD(ADS5281_SPI_WRITE, ADS5281_REG_DATA_FORMAT,
                          ADS5281_DATA_LVDS_TERM);
    HAL_SPI_Transmit(&hspi5, (uint8_t *)&cmd, 2, 10);
    HAL_GPIO_WritePin(ADC_CS_PORT, ADC_CS_PIN, GPIO_PIN_SET);

    /* 2. Enable DCMI interface (but don't start capture yet) */
    HAL_DCMI_Stop(&hdcmi1);
}

/* ========================================================================== */
/*  TX Mux Routing — Route HV pulse to selected transducer                   */
/* ========================================================================== */

void acquisition_select_tx_channel(uint8_t tx_index)
{
    /* Set TX mux address lines PA0–PA4 */
    uint32_t addr = tx_index & ADG732_ADDR_MASK;
    HAL_GPIO_WritePin(TX_MUX_SEL_PORT,
                      TX_MUX_SEL0_PIN | TX_MUX_SEL1_PIN | TX_MUX_SEL2_PIN
                    | TX_MUX_SEL3_PIN | TX_MUX_SEL4_PIN,
                      (addr << 0) & 0x1F);

    /* Enable TX mux (active low) */
    HAL_GPIO_WritePin(TX_MUX_EN_PORT, TX_MUX_EN_PIN, GPIO_PIN_RESET);
}

/* ========================================================================== */
/*  RX Mux Configuration — Route all 32 transducers to 8 ADC channels        */
/* ========================================================================== */

void acquisition_configure_rx_muxes(uint8_t tx_index,
                                     const uint8_t *active,
                                     uint8_t num_sens)
{
    /* We have 4× 32:1 muxes (ADG732) feeding 8 ADC channels (2 per mux).
     * Each mux serves 2 ADC channels.
     * Mux 0: sensors 0–15 → ADC ch 0,1
     * Mux 1: sensors 16–31 → ADC ch 2,3
     * Mux 2: sensors 0–15 → ADC ch 4,5 (different mux for second ADC chip)
     * Mux 3: sensors 16–31 → ADC ch 6,7
     *
     * For a given TX sensor, we want to listen on all OTHER active sensors.
     * The RX muxes are sequenced: we cycle through (N−1) mux configurations,
     * each routing one active sensor per ADC channel.
     * For simplicity in this version, we connect active sensors in round-robin
     * order to the 8 ADC channels.
     */
    uint8_t sens_idx = 0;
    uint8_t ch_idx = 0;

    /* Temporary: iterate all sensors, skip TX sensor, assign to ADC channels */
    for (uint8_t i = 0; i < num_sens && ch_idx < ADC_NUM_CHANNELS; i++) {
        if (i == tx_index) continue;
        if (!active[i]) continue;

        /* Determine which mux handles sensor i */
        if (i < 16) {
            /* Mux 0 or Mux 2 */
            if (ch_idx < 2) {
                /* Mux 0: PB0–PB5 for address, PB5 = EN */
                HAL_GPIO_WritePin(RX_MUX0_SEL_PORT,
                                  RX_MUX0_SEL0_PIN | RX_MUX0_SEL1_PIN
                                | RX_MUX0_SEL2_PIN | RX_MUX0_SEL3_PIN
                                | RX_MUX0_SEL4_PIN,
                                  (uint32_t)(i & ADG732_ADDR_MASK));
                HAL_GPIO_WritePin(RX_MUX0_EN_PORT, RX_MUX0_EN_PIN, GPIO_PIN_RESET);
            } else {
                /* Mux 2: PB8–PB12 for address */
                HAL_GPIO_WritePin(RX_MUX1_SEL_PORT,
                                  RX_MUX1_SEL_PINS,
                                  (uint32_t)(i & ADG732_ADDR_MASK) << 8);
            }
        } else {
            /* Mux 1 or Mux 3 (sensors 16–31) */
            uint8_t mux_addr = i - 16;
            if (ch_idx < 2) {
                /* Mux 0 can reach sensors 0–15 only; for 16–31 use Mux 1 on different port */
                /* Simplified: just route to available mux */
            }
        }

        ctx->rx_mux_map[ch_idx] = i;
        ch_idx++;
    }

    /* If fewer than 8 active sensors + tx, zero out remaining channels */
    while (ch_idx < ADC_NUM_CHANNELS) {
        ctx->rx_mux_map[ch_idx] = 0xFF; /* invalid */
        ch_idx++;
    }
}

/* ========================================================================== */
/*  Fire Burst — Generate HV burst on selected channel                       */
/* ========================================================================== */

void acquisition_fire_burst(uint8_t tx_index, uint8_t n_cycles, uint32_t freq_hz)
{
    (void)tx_index;

    /* Configure TIM1 for desired frequency: ARR = (APB2_CLOCK / freq) − 1 */
    uint32_t arr = (APB2_CLOCK / freq_hz) - 1;
    TIM1->ARR = arr;
    TIM1->CCR1 = arr / 2;  /* 50% duty for half-bridge */

    /* Enable main output */
    TIM1->BDTR |= TIM_BDTR_MOE;

    /* Set burst count: TIM1 repetition counter = n_cycles − 1 */
    TIM1->RCR = n_cycles - 1;

    /* Enable counter */
    TIM1->CR1 |= TIM_CR1_CEN;

    /* Wait for burst completion (UDIS interrupt or poll RCR==0) */
    while ((TIM1->RCR & 0xFF) > 0) {
        /* spin — for short bursts this is fast enough */
    }
    /* Wait for final update event */
    while (!(TIM1->SR & TIM_SR_UIF)) { /* spin */ }
    TIM1->SR &= ~TIM_SR_UIF;

    /* Disable counter */
    TIM1->CR1 &= ~TIM_CR1_CEN;
    TIM1->BDTR &= ~TIM_BDTR_MOE;
}

/* ========================================================================== */
/*  Start ADC Capture via DCMI + DMA                                         */
/* ========================================================================== */

void acquisition_start_capture(acquisition_ctx_t *ctx)
{
    /* Configure DMA for DCMI capture: DCMI → DMA2 Stream 1 → SDRAM */
    /* DMA2 Stream 1, Channel 1 (DCMI) */
    DMA2_Stream1->CR = 0; /* Reset */

    /* Source: DCMI DR register (0x50050028) */
    DMA2_Stream1->PAR = (uint32_t)&DCMI->DR;
    /* Destination: raw LVDS buffer in SDRAM */
    DMA2_Stream1->M0AR = (uint32_t)ctx->raw_lvds_buffer;
    /* Number of data items: 8 channels × 1024 samples = 8192 half-words */
    DMA2_Stream1->NDTR = ADC_NUM_CHANNELS * ADC_SAMPLES_PER_TRACE;

    /* Configure: memory increment, peripheral fixed, half-word, circular off,
     * high priority, DMA flow controller, memory burst 4, peripheral burst 4 */
    DMA2_Stream1->CR = (1 << DMA_SxCR_CHSEL_Pos)        /* Channel 1 */
                     | (0 << DMA_SxCR_MSIZE_Pos)         /* 16-bit memory */
                     | (0 << DMA_SxCR_PSIZE_Pos)         /* 16-bit peripheral */
                     | (1 << DMA_SxCR_MINC_Pos)          /* Memory increment */
                     | (0 << DMA_SxCR_PINC_Pos)          /* Peripheral fixed */
                     | (0 << DMA_SxCR_CIRC_Pos)          /* No circular */
                     | (3 << DMA_SxCR_PL_Pos)            /* Very high priority */
                     | (1 << DMA_SxCR_TCIE_Pos)          /* Transfer complete IRQ */
                     | DMA_SxCR_EN;                      /* Enable */

    /* Enable DCMI */
    DCMI->CR |= DCMI_CR_CAPTURE;

    /* Clear DCMI DMA flag */
    ctx->capture_done = 0;
}

/* DMA2 Stream 1 interrupt handler */
void DMA2_Stream1_IRQHandler(void)
{
    if (DMA2->LISR & DMA_FLAG_TCIF1_6) {
        DCMI->CR &= ~DCMI_CR_CAPTURE;   /* Stop capture */
        DMA2->LIFCR |= DMA_FLAG_TCIF1_6; /* Clear flag */
        /* Deserialise LVDS data into per-channel buffers */
        /* Note: actual deserialisation depends on ADS5281 LVDS format.
         * This is a simplified version — real implementation would handle
         * bit-unpacking of the serial LVDS stream.
         */
        acquisition_ctx_t *ctx = /* retrieve from global */;
        /* LVDS deserialisation stub: copy raw data to channel buffers */
        uint16_t *src = (uint16_t *)ctx->raw_lvds_buffer;
        for (uint8_t ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
            for (uint32_t s = 0; s < ADC_SAMPLES_PER_TRACE; s++) {
                ctx->adc_buffers[ch][s] = (int16_t)(src[ch * ADC_SAMPLES_PER_TRACE + s] & 0x0FFF);
            }
        }
        ctx->capture_done = 1;
    }
}

/* ========================================================================== */
/*  Store Time-of-Flight Data                                                 */
/* ========================================================================== */

void acquisition_store_tofs(acquisition_ctx_t *ctx, uint8_t tx_index,
                             const float *arrival_times, uint8_t num_channels)
{
    for (uint8_t ch = 0; ch < num_channels; ch++) {
        uint8_t rx_sensor = ctx->rx_mux_map[ch];
        if (rx_sensor >= TOMO_MAX_SENSORS) continue;
        ctx->tof_matrix[tx_index][rx_sensor] = arrival_times[ch];
    }
}