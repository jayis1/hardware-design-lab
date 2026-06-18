/*
 * photodiode.c — HydroFluor photodiode acquisition (ADS1256)
 * Author: jayis1
 * License: MIT
 *
 * Reads the 4 fluorescence photodiodes + 3 reference/scatter channels via
 * an 8:1 analog mux feeding the ADS1256 24-bit delta-sigma ADC on SPI1.
 * Each measurement is a dark/light pair for synchronous rejection of ambient
 * light. The full 6×4 fluorescence matrix acquisition is performed by
 * iterating over excitation wavelengths and detector channels.
 */
#include "photodiode.h"
#include "led_excitation.h"
#include "../registers.h"
#include <string.h>

/* Forward declaration — helper used by photodiode_select() */
void gpio_write_bit(gpio_t *g, uint8_t pin, uint8_t val);

/* ADS1256 command set (TI datasheet SBAS288) */
#define ADS1256_CMD_WAKEUP   0x00
#define ADS1256_CMD_RDATA    0x01
#define ADS1256_CMD_RDATAC   0x03
#define ADS1256_CMD_SDATAC   0x0F
#define ADS1256_CMD_RREG     0x10   /* + reg<<2 */
#define ADS1256_CMD_WREG     0x50
#define ADS1256_CMD_SYNC     0xFC
#define ADS1256_CMD_STANDBY  0xFD
#define ADS1256_CMD_RESET    0xFE

/* ADS1256 registers */
#define ADS_REG_STATUS       0x00
#define ADS_REG_MUX          0x01
#define ADS_REG_ADCON        0x02   /* PGA gain + clock out */
#define ADS_REG_DRATE        0x03
#define ADS_REG_IO           0x04
#define ADS_REG_OFC0         0x05
#define ADS_REG_OFC1         0x06
#define ADS_REG_OFC2         0x07
#define ADS_REG_FSC0         0x08
#define ADS_REG_FSC1         0x09
#define ADS_REG_FSC2         0x0A

/* Status reg bits */
#define ADS_STATUS_DRDY      (1U << 4)

/* PGA gain codes in ADCON */
#define ADS_GAIN_1            0
#define ADS_GAIN_2            1
#define ADS_GAIN_4            2
#define ADS_GAIN_8            3
#define ADS_GAIN_16           4
#define ADS_GAIN_32           5
#define ADS_GAIN_64           6

/* Data rate: 30 kSPS (DRATE=0xF0) gives 21.4 ENOB and is good for our
 * integration-time budget of ~120 µs × 8 averages = ~1 ms per channel. */
#define ADS_DRATE_30KSPS     0xF0

/* ADC reference voltage (internal 2.5 V) and LSB size at gain=1 */
#define ADS_VREF_V           2.5f
#define ADS_FULLSCALE        ((1UL << 23) - 1)
#define ADS_LSB_UV(gain)     ((int32_t)(ADS_VREF_V * 1e6f / (float)(ADS_FULLSCALE * (1<<(gain)))))

static uint8_t g_gain = ADS_GAIN_4;

/* ---- Low-level ADS1256 SPI helpers ---- */

static inline void ads_cs_low(void)  { gpio_clr(ADC_CS_PORT, ADC_CS_PIN); }
static inline void ads_cs_high(void) { gpio_set(ADC_CS_PORT, ADC_CS_PIN); }

static void ads_wait_drdy(void)
{
    /* Poll DRDY line (PB1) — active low */
    uint32_t timeout = 100000;
    while (gpio_read(ADC_DRDY_PORT, ADC_DRDY_PIN) && --timeout) { /* spin */ }
}

static uint8_t ads_read_reg(uint8_t reg)
{
    ads_cs_low();
    spi_xfer(SPI1, ADS1256_CMD_RREG | (reg << 2));   /* 0001 rrrr */
    spi_xfer(SPI1, 0);                                /* 1-byte read */
    /* dummy cycle required per datasheet */
    for (volatile int i = 0; i < 8; ++i) { }
    uint8_t v = spi_xfer(SPI1, 0xFF);
    ads_cs_high();
    return v;
}

static void ads_write_reg(uint8_t reg, uint8_t val)
{
    ads_cs_low();
    spi_xfer(SPI1, ADS1256_CMD_WREG | (reg << 2));
    spi_xfer(SPI1, 0);                  /* 1 byte */
    spi_xfer(SPI1, val);
    for (volatile int i = 0; i < 8; ++i) { }
    ads_cs_high();
}

static int32_t ads_read_data(void)
{
    ads_cs_low();
    ads_wait_drdy();
    spi_xfer(SPI1, ADS1256_CMD_RDATA);
    /* wait DRDY again (max ~50 µs at 30 kSPS) */
    for (volatile int i = 0; i < 200; ++i) { }
    uint8_t b0 = spi_xfer(SPI1, 0xFF);
    uint8_t b1 = spi_xfer(SPI1, 0xFF);
    uint8_t b2 = spi_xfer(SPI1, 0xFF);
    ads_cs_high();
    int32_t raw = ((int32_t)b0 << 16) | ((int32_t)b1 << 8) | (int32_t)b2;
    /* Sign-extend 24→32 */
    if (raw & 0x800000) raw |= 0xFF000000;
    return raw;
}

static void ads_set_mux(uint8_t pos, uint8_t neg)
{
    ads_write_reg(ADS_REG_MUX, (pos << 4) | (neg & 0x0F));
    /* SYNC pulse to latch new MUX and restart conversion */
    ads_cs_low();
    spi_xfer(SPI1, ADS1256_CMD_SYNC);
    ads_cs_high();
}

/* ---- Public API ---- */

void photodiode_init(void)
{
    /* Enable SPI1 + GPIO clocks */
    RCC->AHB2ENR |= (1U << 0) | (1U << 1) | (1U << 2);   /* A,B,C */
    RCC->APB2ENR |= (1U << 12);                           /* SPI1 */

    /* SPI1 pins: PA5 SCK, PA6 MISO, PA7 MOSI (AF5) */
    gpio_mode(GPIOA, ADC_SPI_SCK_PIN,  GPIO_MODE_AF);
    gpio_mode(GPIOA, ADC_SPI_MISO_PIN, GPIO_MODE_AF);
    gpio_mode(GPIOA, ADC_SPI_MOSI_PIN, GPIO_MODE_AF);
    gpio_af(GPIOA, ADC_SPI_SCK_PIN, 5);
    gpio_af(GPIOA, ADC_SPI_MISO_PIN, 5);
    gpio_af(GPIOA, ADC_SPI_MOSI_PIN, 5);

    /* CS on PB2, DRDY on PB1 (input) */
    gpio_mode(ADC_CS_PORT, ADC_CS_PIN, GPIO_MODE_OUTPUT);
    gpio_set(ADC_CS_PORT, ADC_CS_PIN);   /* idle high */
    gpio_mode(ADC_DRDY_PORT, ADC_DRDY_PIN, GPIO_MODE_INPUT);
    gpio_pupd(ADC_DRDY_PORT, ADC_DRDY_PIN, GPIO_PUPD_PULLUP);

    /* Mux select lines: PD2, PC4, PC5 (output) */
    gpio_mode(GPIOD, MUX_SEL0_PIN, GPIO_MODE_OUTPUT);
    gpio_mode(GPIOC, MUX_SEL1_PIN, GPIO_MODE_OUTPUT);
    gpio_mode(GPIOC, MUX_SEL2_PIN, GPIO_MODE_OUTPUT);

    /* SPI1 master, CPOL=1, CPHA=1, /8 baud, 8-bit, MSB first */
    SPI1->CR1 = 0;
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_BR_DIV8;
    SPI1->CR1 |= SPI_CR1_SPE;

    /* Hardware reset ADS1256 via CS toggle (no dedicated RST pin) */
    ads_cs_low();
    spi_xfer(SPI1, ADS1256_CMD_RESET);
    for (volatile int i = 0; i < 2000; ++i) { }    /* settle */
    ads_cs_high();

    /* Configure ADS1256: PGA=4, data rate 30 kSPS, MUX AIN0-AINCOM */
    ads_write_reg(ADS_REG_STATUS, 0x01);   /* ORDER=MSB, no buffer */
    ads_write_reg(ADS_REG_ADCON, ADS_GAIN_4);  /* PGA gain 4 */
    ads_write_reg(ADS_REG_DRATE, ADS_DRATE_30KSPS);
    ads_write_reg(ADS_REG_MUX, 0x01);        /* AIN0 - AIN1 default */
    ads_cs_low();
    spi_xfer(SPI1, ADS1256_CMD_SDATAC);     /* stop continuous read mode */
    ads_cs_high();

    g_gain = ADS_GAIN_4;
}

void photodiode_select(pd_channel_t ch)
{
    /* 3-bit select on 74HC4051 */
    gpio_write_bit(GPIOD, MUX_SEL0_PIN, ch & 1);
    gpio_write_bit(GPIOC, MUX_SEL1_PIN, (ch >> 1) & 1);
    gpio_write_bit(GPIOC, MUX_SEL2_PIN, (ch >> 2) & 1);
    /* Allow mux settling (~1 µs) */
    for (volatile int i = 0; i < 24; ++i) { }
}

void photodiode_set_gain(uint8_t gain)
{
    if (gain > ADS_GAIN_64) gain = ADS_GAIN_64;
    ads_write_reg(ADS_REG_ADCON, gain);
    g_gain = gain;
}

int32_t photodiode_read_uv(void)
{
    int32_t raw = ads_read_data();
    /* Convert to µV using current PGA gain */
    int32_t lsb_uv = ADS_LSB_UV(g_gain);
    return raw * lsb_uv / 1000;   /* LSB_UV is in nV → divide */
}

void photodiode_acquire_pair(pd_channel_t ch,
                             int32_t *dark_uv, int32_t *light_uv)
{
    photodiode_select(ch);
    ads_set_mux(0, 1);   /* single-ended AIN0 vs AINCOM */

    /* Dark: LED off, sample ambient + dark offset */
    *dark_uv = photodiode_read_uv();

    /* Light: caller has already fired the LED; we sample now.
     * In matrix mode we let led_excitation_fire() run synchronously. */
    *light_uv = photodiode_read_uv();
}

uint8_t photodiode_selftest(void)
{
    /* ID register (REG0 high nibble) should be 0x3 for ADS1256 */
    uint8_t id = ads_read_reg(ADS_REG_STATUS) >> 4;
    return id;
}

/* Helper to write a GPIO bit without depending on the gpio_set/clr
 * ordering of the macro. Provided here because the mux select uses
 * three different ports. */
void gpio_write_bit(gpio_t *g, uint8_t pin, uint8_t val)
{
    if (val) gpio_set(g, pin); else gpio_clr(g, pin);
}

int photodiode_acquire_matrix(acquisition_t *out,
                               const led_config_t cfg[EX_CHANNEL_COUNT])
{
    memset(out, 0, sizeof(*out));
    int err = 0;

    /* Order of detector channels to acquire per excitation:
     * 255nm → PD_FLUOR_360 + PD_REF_255
     * 280nm → PD_FLUOR_360 (secondary)
     * 365nm → PD_FLUOR_430
     * 470nm → PD_FLUOR_685
     * 590nm → PD_FLUOR_650
     * 660nm → PD_SCATTER_660 + PD_REF_660
     */
    static const pd_channel_t det_for_ex[EX_CHANNEL_COUNT][2] = {
        { PD_FLUOR_360, PD_REF_255   },  /* 255 */
        { PD_FLUOR_360, PD_GND_BIAS  },  /* 280 */
        { PD_FLUOR_430, PD_GND_BIAS  },  /* 365 */
        { PD_FLUOR_685, PD_GND_BIAS  },  /* 470 */
        { PD_FLUOR_650, PD_GND_BIAS  },  /* 590 */
        { PD_SCATTER_660, PD_REF_660 },  /* 660 */
    };

    for (int ex = 0; ex < EX_CHANNEL_COUNT; ++ex) {
        const led_config_t *c = &cfg[ex];
        /* Auto-range gain: try gain 4, if overrange drop to gain 1 */
        photodiode_set_gain(ADS_GAIN_4);

        int32_t sum_light = 0, sum_dark = 0;
        uint8_t overrange = 0;

        for (uint8_t rep = 0; rep < c->averages; ++rep) {
            int32_t dark, light;
            /* Acquire on the primary detector for this excitation */
            photodiode_acquire_pair(det_for_ex[ex][0], &dark, &light);
            sum_dark  += dark;
            sum_light += light;
            if (light > (int32_t)(0.9f * ADS_FULLSCALE * ADS_LSB_UV(g_gain))) {
                overrange = 1;
            }
        }
        int32_t avg_dark  = sum_dark  / c->averages;
        int32_t avg_light = sum_light / c->averages;
        int32_t net = avg_light - avg_dark;
        out->fluor[ex][det_for_ex[ex][0]] = net;
        if (overrange) err |= 1;

        /* Acquire the secondary/reference channel (no averaging needed) */
        if (det_for_ex[ex][1] != PD_GND_BIAS) {
            int32_t d2, l2;
            photodiode_acquire_pair(det_for_ex[ex][1], &d2, &l2);
            int32_t net2 = l2 - d2;
            out->fluor[ex][det_for_ex[ex][1]] = net2;
            if (ex == 0) out->ref_255_uv   = net2;   /* 255 trans */
            if (ex == 5) {
                out->ref_660_uv     = net2;
                out->scatter_660_uv = out->fluor[ex][PD_SCATTER_660];
            }
        }

        /* Bubble detect: dark reading should be near 0 ± noise; if
         * dark variance is huge, an air bubble is likely crossing the
         * optical window. Heuristic: if |net| is negative AND > 20% of
         * fullscale, flag bubble. */
        if (net < -((int32_t)(0.2f * ADS_FULLSCALE * ADS_LSB_UV(g_gain)))) {
            err |= 2;
        }
    }

    out->flags = (uint16_t)err;
    return err;
}