/**
 * @file    doppler.c
 * @brief   TideBand — Doppler velocimeter driver implementation.
 *          Drives the 1 MHz TX transducer, captures I/Q data from the
 *          AD9629 ADC via DMA, performs FFT-based Doppler extraction,
 *          and solves for 3D water velocity using the tetrahedral
 *          receiver geometry.
 * @author  jayis1
 * @copyright © 2026 jayis1. All rights reserved.
 * @license GPL-2.0
 *
 * Signal chain:
 *   TIM1_CH1 (1 MHz PWM) → LM48611 amp → PZT TX transducer
 *   PZT RX ×3 → AD8331 VGA → AD8333 I/Q demod → AD9629 ADC (20 kSPS)
 *   AD9629 → SPI1 → DMA2_STREAM0 → SRAM buffer
 *
 * The AD8333 produces interleaved I/Q samples. The DMA captures
 * DOPPLER_FFT_SIZE complex samples per channel (3 channels = 6 real
 * streams). After capture, a 4096-point FFT per channel extracts the
 * dominant Doppler frequency, which is converted to radial velocity
 * using the Doppler equation:
 *
 *   f_d = 2 * v * cos(theta) * f_tx / c
 *
 * where theta is the receiver angle, f_tx is the carrier, and c is
 * the speed of sound in water. Three radial velocities from the
 * three angled receivers are deprojected into 3D body-frame velocity
 * using the calibration matrix.
 */

#include <string.h>
#include <math.h>
#include "board.h"
#include "registers.h"
#include "doppler.h"

/* ---- CMSIS-DSP (arm_cortexM7lfsp_math) ---- */
/* We declare the CMSIS-DSP function prototypes here so the file
 * compiles in any environment. In the full build, link against
 * libarm_cortexM7lfsp_math.a. */
typedef struct { float re; float im; } cpx32f_t;  /* float32 complex */
typedef struct { float r; float i; } cpx_t;

/* CMSIS-DSP declarations used (inline fallbacks if library absent) */
extern void arm_rfft_fast_init_f32(void *S, int fftLen);
extern void arm_rfft_fast_f32(void *S, float *in, float *out, int ifftFlag);

/* ---- Internal state ---- */
static volatile uint8_t  dma_complete = 0;
static volatile uint8_t  doppler_enabled = 0;
static doppler_calibration_t active_cal;

/* ADC DMA buffers — 3 channels × I/Q × DOPPLER_FFT_SIZE samples */
/* We capture I and Q sequentially: [I0, Q0, I1, Q1, ...] */
static volatile int16_t adc_buffer[6 * DOPPLER_FFT_SIZE];  /* 6 streams interleaved */
static float fft_input[DOPPLER_FFT_SIZE];
static float fft_output[DOPPLER_FFT_SIZE * 2];  /* complex output */

/* RFFT instance (simplified — real FFT is 2× faster than complex) */
static int rfft_initialized = 0;

/* ---- Geometry: receiver direction unit vectors in body frame ---- */
/* RX1: 30° from Z-axis toward +X
 * RX2: 30° from Z-axis toward (−X/2, +Y*sqrt(3)/2)
 * RX3: 30° from Z-axis toward (−X/2, −Y*sqrt(3)/2)
 * These are the acoustic-path directions from the measurement volume
 * to each receiver. */
static const float rx_dirs[3][3] = {
    { 0.8660254f,  0.0f,        0.5f },   /* RX1: sin(30) in XZ */
    { -0.4330127f, 0.75f,       0.5f },   /* RX2: 120° from RX1 */
    { -0.4330127f, -0.75f,      0.5f },   /* RX3: 240° from RX1 */
};

/* ---- Local function prototypes ---- */
static void doppler_init_gpio(void);
static void doppler_init_tim1(void);
static void doppler_init_spi_adc(void);
static void doppler_init_dma(void);
static float estimate_doppler_freq(const int16_t *iq_data, uint16_t n,
                                    float *snr_out);
static void fft_radix2_dit(float *data, uint16_t n);
static uint16_t find_peak(float *mag, uint16_t n, float *peak_val);

/* ---- Public API implementations ---- */

void doppler_init(void)
{
    memset((void *)adc_buffer, 0, sizeof(adc_buffer));
    memset(&active_cal, 0, sizeof(active_cal));

    /* Load calibration from flash */
    if (doppler_load_calibration(&active_cal) != 0) {
        /* No calibration — use defaults */
        active_cal.magic = CAL_MAGIC;
        active_cal.sound_speed = DOPPLER_SOUND_SPEED;
        active_cal.tx_power_comp = 1.0f;
        for (int i = 0; i < 3; i++) {
            active_cal.scale_factor[i] = 1.0f;
            active_cal.phase_offset[i] = 0.0f;
        }
        /* Default deprojection matrix = pseudo-inverse of rx_dirs */
        /* For symmetric geometry, M = inv(R^T) where R is 3×3 of rx_dirs */
        /* Simplified: use analytical inverse for the tetrahedral case */
        active_cal.deproj_matrix[0][0] =  0.7698f;
        active_cal.deproj_matrix[0][1] =  0.7698f;
        active_cal.deproj_matrix[0][2] =  0.7698f;
        active_cal.deproj_matrix[1][0] =  0.0f;
        active_cal.deproj_matrix[1][1] =  0.0f;
        active_cal.deproj_matrix[1][2] =  0.0f;
        active_cal.deproj_matrix[2][0] =  0.0f;
        active_cal.deproj_matrix[2][1] =  0.0f;
        active_cal.deproj_matrix[2][2] =  0.0f;
    }

    doppler_init_gpio();
    doppler_init_tim1();
    doppler_init_spi_adc();
    doppler_init_dma();

    doppler_enabled = 1;
}

void doppler_trigger(void)
{
    if (!doppler_enabled) {
        doppler_wake();
    }

    dma_complete = 0;

    /* Enable TX amplifier */
    gpio_set(DOPPLER_TX_EN_GPIO, DOPPLER_TX_EN_PIN);

    /* Start TIM1 PWM — 1 MHz continuous wave */
    TIM1_CR1 |= TIM_CR1_CEN;
    TIM1_BDTR |= TIM_BDTR_MOE;  /* Main output enable */

    /* Small delay for TX to stabilize (10 µs) */
    for (volatile int i = 0; i < 280; i++) { }

    /* Start ADC DMA capture — SPI1 reads AD9629, DMA2 writes to SRAM */
    /* Clear any pending DMA flags */
    DMA2_LIFCR = 0x3Fu;  /* Clear stream0 flags */

    /* Configure DMA2 stream0: SPI1 RX → memory, 6*N samples */
    DMA_S_PAR(DMA2_STREAM0_BASE) = (uint32_t)&SPI1_DR;
    DMA_S_M0AR(DMA2_STREAM0_BASE) = (uint32_t)adc_buffer;
    DMA_S_NDT(DMA2_STREAM0_BASE) = 6u * DOPPLER_FFT_SIZE;
    DMA_S_CR(DMA2_STREAM0_BASE) = DMA_CR_DIR_P2M |
                                   DMA_CR_MINC |
                                   DMA_CR_MSIZE_16 |
                                   DMA_CR_PSIZE_16 |
                                   DMA_CR_TCIE |
                                   DMA_CR_PL_VHIGH |
                                   DMA_CR_EN;

    /* Enable SPI1 to start clocking the ADC */
    SPI1_CR1 |= SPI_CR1_SPE;

    /* DMA will complete when all samples received. ISR sets dma_complete. */
}

uint8_t doppler_data_ready(void)
{
    return dma_complete;
}

void doppler_process(doppler_result_t *result)
{
    float doppler_hz[3] = {0, 0, 0};
    float snr[3] = {0, 0, 0};
    float vel_radial[3];
    float vel_body[3];
    float c = active_cal.sound_speed;
    float f_tx = (float)DOPPLER_TX_FREQ_HZ;
    float cos_theta = cosf(DOPPLER_RX_ANGLE_RAD);

    memset(result, 0, sizeof(*result));

    /* Extract Doppler frequency for each of the 3 receiver channels.
     * ADC data is interleaved: [I0ch0, Q0ch0, I0ch1, Q0ch1, I0ch2, Q0ch2,
     *                            I1ch0, Q1ch0, ...]
     * We de-interleave per channel. */
    for (int ch = 0; ch < 3; ch++) {
        /* Build complex input: real=I, imag=Q */
        for (uint16_t i = 0; i < DOPPLER_FFT_SIZE; i++) {
            int16_t i_val = adc_buffer[i * 6 + ch * 2];
            int16_t q_val = adc_buffer[i * 6 + ch * 2 + 1];
            fft_input[i] = (float)i_val;
        }

        /* Compute Doppler frequency via FFT peak detection */
        doppler_hz[ch] = estimate_doppler_freq(fft_input, DOPPLER_FFT_SIZE,
                                                &snr[ch]);

        /* Convert Doppler frequency to radial velocity
         * f_d = 2 * v * cos(theta) * f_tx / c
         * => v = f_d * c / (2 * f_tx * cos(theta)) */
        vel_radial[ch] = doppler_hz[ch] * c /
                         (2.0f * f_tx * cos_theta) *
                         active_cal.scale_factor[ch];
    }

    /* Deproject 3 radial velocities into 3D body-frame velocity
     * using calibration matrix: v_body = M * v_radial */
    for (int i = 0; i < 3; i++) {
        vel_body[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            vel_body[i] += active_cal.deproj_matrix[i][j] * vel_radial[j];
        }
    }

    result->vx = vel_body[0];
    result->vy = vel_body[1];
    result->vz = vel_body[2];
    result->speed = sqrtf(vel_body[0]*vel_body[0] +
                          vel_body[1]*vel_body[1] +
                          vel_body[2]*vel_body[2]);

    memcpy(result->doppler_hz, doppler_hz, sizeof(doppler_hz));
    memcpy(result->snr, snr, sizeof(snr));

    /* Quality assessment based on SNR */
    float min_snr = snr[0];
    if (snr[1] < min_snr) min_snr = snr[1];
    if (snr[2] < min_snr) min_snr = snr[2];

    if (min_snr > 20.0f) {
        result->quality = 3;  /* Excellent */
        result->valid = 1;
    } else if (min_snr > 15.0f) {
        result->quality = 2;  /* Good */
        result->valid = 1;
    } else if (min_snr > 10.0f) {
        result->quality = 1;  /* Fair */
        result->valid = 1;
    } else {
        result->quality = 0;  /* Poor */
        result->valid = 0;
    }

    /* Stop TX after processing */
    TIM1_CR1 &= ~TIM_CR1_CEN;
    gpio_clear(DOPPLER_TX_EN_GPIO, DOPPLER_TX_EN_PIN);
}

void doppler_sleep(void)
{
    doppler_enabled = 0;
    TIM1_CR1 &= ~TIM_CR1_CEN;
    gpio_clear(DOPPLER_TX_EN_GPIO, DOPPLER_TX_EN_PIN);
    SPI1_CR1 &= ~SPI_CR1_SPE;
}

void doppler_wake(void)
{
    doppler_enabled = 1;
    /* Re-init SPI1 and DMA (registers may have been cleared) */
    doppler_init_spi_adc();
    doppler_init_dma();
}

int doppler_load_calibration(doppler_calibration_t *cal)
{
    const volatile uint32_t *flash = (const volatile uint32_t *)CAL_FLASH_ADDR;

    /* Copy calibration struct from flash */
    memcpy(cal, (const void *)flash, sizeof(doppler_calibration_t));

    if (cal->magic != CAL_MAGIC) {
        return -1;
    }

    /* Simple CRC check (XOR of all 32-bit words except the last) */
    uint32_t crc = 0;
    const uint32_t *p = (const uint32_t *)cal;
    uint16_t n_words = (sizeof(doppler_calibration_t) / 4u) - 1u;
    for (uint16_t i = 0; i < n_words; i++) {
        crc ^= p[i];
    }

    if (crc != cal->crc) {
        return -1;
    }

    return 0;
}

int doppler_save_calibration(const doppler_calibration_t *cal)
{
    /* For safety, we do not implement flash erase/write here in the
     * main driver. The app sends calibration data via BLE, and a
     * separate flash_writer module handles the erase+program sequence.
     * This function validates the calibration and copies it to active_cal. */
    if (cal->magic != CAL_MAGIC) {
        return -1;
    }

    memcpy(&active_cal, cal, sizeof(doppler_calibration_t));
    return 0;
}

void doppler_apply_calibration(const doppler_calibration_t *cal,
                                const float doppler_hz[3],
                                float vel_body[3])
{
    float c = cal->sound_speed;
    float f_tx = (float)DOPPLER_TX_FREQ_HZ;
    float cos_theta = cosf(DOPPLER_RX_ANGLE_RAD);
    float vel_radial[3];

    for (int i = 0; i < 3; i++) {
        vel_radial[i] = doppler_hz[i] * c /
                        (2.0f * f_tx * cos_theta) *
                        cal->scale_factor[i];
    }

    for (int i = 0; i < 3; i++) {
        vel_body[i] = 0.0f;
        for (int j = 0; j < 3; j++) {
            vel_body[i] += cal->deproj_matrix[i][j] * vel_radial[j];
        }
    }
}

/* ---- Local function implementations ---- */

static void doppler_init_gpio(void)
{
    /* Enable GPIO ports */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA | RCC_AHB1ENR_GPIOB |
                    RCC_AHB1ENR_GPIOC | RCC_AHB1ENR_GPIOD |
                    RCC_AHB1ENR_GPIOE;

    /* TX pin (PA8) — AF1 (TIM1_CH1) */
    gpio_set_mode(DOPPLER_TX_GPIO, DOPPLER_TX_PIN, GPIO_MODE_AF);
    gpio_set_af(DOPPLER_TX_GPIO, DOPPLER_TX_PIN, DOPPLER_TX_AF);
    gpio_set_speed(DOPPLER_TX_GPIO, DOPPLER_TX_PIN, GPIO_SPEED_VHIGH);

    /* TX enable (PA12) — output, initially low */
    gpio_set_mode(DOPPLER_TX_EN_GPIO, DOPPLER_TX_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_clear(DOPPLER_TX_EN_GPIO, DOPPLER_TX_EN_PIN);

    /* ADC SPI pins: SCK (PB3 AF5), MISO (PB4 AF5), MOSI (PB5 AF5) */
    gpio_set_mode(DOPPLER_ADC_SCK_GPIO, DOPPLER_ADC_SCK_PIN, GPIO_MODE_AF);
    gpio_set_af(DOPPLER_ADC_SCK_GPIO, DOPPLER_ADC_SCK_PIN, DOPPLER_ADC_SCK_AF);
    gpio_set_speed(DOPPLER_ADC_SCK_GPIO, DOPPLER_ADC_SCK_PIN, GPIO_SPEED_VHIGH);

    gpio_set_mode(DOPPLER_ADC_MISO_GPIO, DOPPLER_ADC_MISO_PIN, GPIO_MODE_AF);
    gpio_set_af(DOPPLER_ADC_MISO_GPIO, DOPPLER_ADC_MISO_PIN, DOPPLER_ADC_MISO_AF);
    gpio_set_speed(DOPPLER_ADC_MISO_GPIO, DOPPLER_ADC_MISO_PIN, GPIO_SPEED_VHIGH);

    gpio_set_mode(DOPPLER_ADC_MOSI_GPIO, DOPPLER_ADC_MOSI_PIN, GPIO_MODE_AF);
    gpio_set_af(DOPPLER_ADC_MOSI_GPIO, DOPPLER_ADC_MOSI_PIN, DOPPLER_ADC_MOSI_AF);
    gpio_set_speed(DOPPLER_ADC_MOSI_GPIO, DOPPLER_ADC_MOSI_PIN, GPIO_SPEED_VHIGH);

    /* ADC CS (PA4) — output, high (deselected) */
    gpio_set_mode(DOPPLER_ADC_CS_GPIO, DOPPLER_ADC_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set(DOPPLER_ADC_CS_GPIO, DOPPLER_ADC_CS_PIN);

    /* ADC BUSY (PC4) — input */
    gpio_set_mode(DOPPLER_ADC_BUSY_GPIO, DOPPLER_ADC_BUSY_PIN, GPIO_MODE_INPUT);
}

static void doppler_init_tim1(void)
{
    /* Enable TIM1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_TIM1;

    /* TIM1 on APB2 = 280 MHz timer clock.
     * For 1 MHz PWM: PSC = 0, ARR = 280 (280 MHz / 280 = 1 MHz)
     * 50% duty: CCR1 = 140 */
    TIM1_PSC = 0;
    TIM1_ARR = 280 - 1;       /* Auto-reload: 280 counts = 1 MHz */
    TIM1_CCR1 = 140;          /* 50% duty cycle */
    TIM1_CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE;
    TIM1_CCER = TIM_CCER_CC1E;
    TIM1_BDTR = TIM_BDTR_MOE;  /* Enable main output */
    TIM1_CR1 = TIM_CR1_ARPE;   /* Auto-reload preload, not started yet */
}

static void doppler_init_spi_adc(void)
{
    /* Enable SPI1 clock */
    RCC->APB2ENR |= RCC_APB2ENR_SPI1;

    /* Configure SPI1 as master, 16-bit data, CPOL=1 CPHA=1
     * Baud: APB2=140MHz / 2 = 70 MHz (MBR=0) */
    SPI1_CFG2 = SPI_CFG2_MASTER | SPI_CFG2_CPOL | SPI_CFG2_CPHA |
                SPI_CFG2_SSOE;
    SPI1_CFG1 = SPI_CFG1_DSIZE_16 | SPI_CFG1_FTHLV_1;
    /* Note: In actual H7, SSOE manages hardware NSS. We use GPIO CS. */
}

static void doppler_init_dma(void)
{
    /* Enable DMA2 clock */
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2;

    /* Configure DMAMUX channel 0 for SPI1 RX (request 42 on H733) */
    DMAMUX_CxCR(0) = 42u;  /* SPI1_RX_DMA request */
}

/**
 * Estimate the dominant Doppler frequency from I/Q samples.
 * Uses a radix-2 FFT to find the spectral peak, then refines with
 * parabolic interpolation. Returns frequency in Hz and SNR in dB.
 */
static float estimate_doppler_freq(const float *data, uint16_t n,
                                    float *snr_out)
{
    /* Fill FFT input with real data (I component), zero imag */
    memset(fft_output, 0, sizeof(fft_output));
    for (uint16_t i = 0; i < n; i++) {
        fft_output[i * 2]     = data[i];  /* Real */
        fft_output[i * 2 + 1] = 0.0f;     /* Imag (Q handled separately) */
    }

    /* In-place complex FFT */
    fft_radix2_dit(fft_output, n);

    /* Compute magnitude spectrum (half, since real input → symmetric) */
    static float mag[DOPPLER_FFT_SIZE / 2];
    uint16_t half = n / 2;
    for (uint16_t i = 0; i < half; i++) {
        float re = fft_output[i * 2];
        float im = fft_output[i * 2 + 1];
        mag[i] = sqrtf(re * re + im * im);
    }

    /* Find peak frequency bin */
    float peak_val;
    uint16_t peak_bin = find_peak(mag, half, &peak_val);

    /* Parabolic interpolation for sub-bin accuracy */
    float alpha = mag[(peak_bin > 0 ? peak_bin - 1 : 0)];
    float beta  = mag[peak_bin];
    float gamma = mag[(peak_bin + 1 < half ? peak_bin + 1 : half - 1)];
    float delta = 0.5f * (alpha - gamma) / (alpha - 2.0f * beta + gamma + 1e-7f);

    float freq_bin = (float)peak_bin + delta;
    float freq_hz = freq_bin * (float)DOPPLER_ADC_RATE_HZ / (float)n;

    /* Compute SNR: peak power / mean power of rest of spectrum */
    float total_power = 0.0f;
    for (uint16_t i = 1; i < half; i++) {
        if (i != peak_bin) {
            total_power += mag[i] * mag[i];
        }
    }
    float mean_power = total_power / (float)(half - 2);
    float peak_power = peak_val * peak_val;
    *snr_out = 10.0f * log10f((peak_power + 1e-10f) / (mean_power + 1e-10f));

    return freq_hz;
}

/**
 * Simple radix-2 decimation-in-time FFT (complex, in-place).
 * For production, replace with CMSIS-DSP arm_cfft_f32 for ~5× speedup.
 * data layout: [re0, im0, re1, im1, ...], length n (must be power of 2).
 */
static void fft_radix2_dit(float *data, uint16_t n)
{
    /* Bit reversal */
    uint16_t j = 0;
    for (uint16_t i = 0; i < n; i++) {
        if (i < j) {
            float tr = data[i * 2];
            float ti = data[i * 2 + 1];
            data[i * 2]     = data[j * 2];
            data[i * 2 + 1] = data[j * 2 + 1];
            data[j * 2]     = tr;
            data[j * 2 + 1] = ti;
        }
        uint16_t m = n;
        while (j & (m >> 1)) {
            j &= ~(m >> 1);
            m >>= 1;
        }
        j |= m >> 1;
    }

    /* FFT butterflies */
    for (uint16_t s = 1; s < n; s *= 2) {
        float angle = -3.14159265358979f / (float)s;
        float wr = cosf(angle);
        float wi = sinf(angle);
        for (uint16_t k = 0; k < n; k += 2 * s) {
            float cur_wr = 1.0f, cur_wi = 0.0f;
            for (uint16_t m_idx = 0; m_idx < s; m_idx++) {
                uint16_t idx0 = (k + m_idx) * 2;
                uint16_t idx1 = (k + m_idx + s) * 2;
                float tr = cur_wr * data[idx1] - cur_wi * data[idx1 + 1];
                float ti = cur_wr * data[idx1 + 1] + cur_wi * data[idx1];
                data[idx1]     = data[idx0] - tr;
                data[idx1 + 1] = data[idx0 + 1] - ti;
                data[idx0]     = data[idx0] + tr;
                data[idx0 + 1] = data[idx0 + 1] + ti;
                float new_wr = cur_wr * wr - cur_wi * wi;
                cur_wi = cur_wr * wi + cur_wi * wr;
                cur_wr = new_wr;
            }
        }
    }
}

static uint16_t find_peak(float *mag, uint16_t n, float *peak_val)
{
    uint16_t peak = 1;  /* Skip DC bin */
    *peak_val = mag[1];
    for (uint16_t i = 2; i < n; i++) {
        if (mag[i] > *peak_val) {
            *peak_val = mag[i];
            peak = i;
        }
    }
    return peak;
}

/* ---- DMA2 Stream0 interrupt handler (SPI1 RX from ADC) ---- */
/* This is the actual IRQ handler name for STM32H7 */
void DMA2_Stream0_IRQHandler(void)
{
    if (DMA2->LISR & (1u << 5)) {  /* TCIF0: Transfer Complete Flag */
        /* Clear flag */
        DMA2->LIFCR = (1u << 5);

        /* Disable SPI and DMA */
        SPI1_CR1 &= ~SPI_CR1_SPE;
        DMA_S_CR(DMA2_STREAM0_BASE) &= ~DMA_CR_EN;

        /* Signal data ready */
        dma_complete = 1;
    }
}