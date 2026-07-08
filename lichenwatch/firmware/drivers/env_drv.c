/*
 * env_drv.c — Environmental sensor aggregation driver.
 *
 * Aggregates readings from:
 *   - SHT45 (T/RH) over I2C1 (shared with spectral sensor)
 *   - Senseair S8 (CO₂ NDIR) over USART3
 *   - Si1145 (UV index / ambient light) over I2C1
 *   - VEML6075 (UV-A / UV-B) over I2C1
 *
 * Author: jayis1
 * License: MIT
 */

#include "env_drv.h"
#include "../board.h"
#include "../registers.h"
#include <string.h>

/* ------------------------------------------------------------------------ */
/* SHT45 commands                                                            */
/* ------------------------------------------------------------------------ */
#define SHT45_CMD_MEASURE_HIGHREP 0xFD  /* clock stretching, high repeatability */

/* ------------------------------------------------------------------------ */
/* Si1145 register addresses                                                  */
/* ------------------------------------------------------------------------ */
#define SI1145_REG_PARTID   0x00
#define SI1145_REG_HWKEY    0x07  /* must write 0x17 after reset */
#define SI1145_REG_UVINDEX0 0x2C
#define SI1145_REG_MEASRATE0 0x08
#define SI1145_REG_MEASCONFIG0 0x18
#define SI1145_REG_COMMAND   0x06
#define SI1145_CMD_MEASUV     0x2C  /* measure UV index */

/* ------------------------------------------------------------------------ */
/* VEML6075 register addresses                                                */
/* ------------------------------------------------------------------------ */
#define VEML6075_REG_UV_CONF 0x00
#define VEML6075_REG_UVA     0x04
#define VEML6075_REG_UVB     0x09
#define VEML6075_REG_UV_COMP1 0x0A
#define VEML6075_REG_UV_COMP2 0x0B

/* ------------------------------------------------------------------------ */
/* I²C helpers (reuse spectral_drv's bus, which is already configured)        */
/* ------------------------------------------------------------------------ */
extern int i2c_write_reg(uint8_t addr, uint8_t reg, uint8_t val);
extern int i2c_read_burst(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t n);

static int s_initialized = 0;

/* ------------------------------------------------------------------------ */
/* USART3 setup for Senseair S8                                               */
/* ------------------------------------------------------------------------ */
static void uart3_init(void)
{
    /* Configure PC10 (TX) and PC11 (RX) as AF7 (USART3). */
    volatile uint32_t *pc = (volatile uint32_t *)GPIOC;
    /* PC10 → AFRH nibble 2 */
    pc[8] = (pc[8] & ~(0xFU << (2*4))) | (0x7U << (2*4));
    /* PC11 → AFRH nibble 3 */
    pc[8] = (pc[8] & ~(0xFU << (3*4))) | (0x7U << (3*4));
    pc[0] = (pc[0] & ~(0x3U << (10*2))) | (GPIO_MODE_AF << (10*2));
    pc[0] = (pc[0] & ~(0x3U << (11*2))) | (GPIO_MODE_AF << (11*2));

    /* Baud rate: APB1 = 40 MHz / 9600 → BRR = 4166 */
    USART3->BRR = 40000000UL / S8_UART_BAUD;
    USART3->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

static int uart3_tx(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) {
        while (!(USART3->ISR & USART_ISR_TXE)) { }
        USART3->TDR = buf[i];
    }
    while (!(USART3->ISR & USART_ISR_TC)) { }
    return len;
}

static int uart3_rx(uint8_t *buf, int maxlen, uint32_t timeout_ms)
{
    int n = 0;
    for (uint32_t t = 0; t < timeout_ms * 100; t++) {
        if (USART3->ISR & USART_ISR_RXNE) {
            if (n < maxlen) buf[n++] = (uint8_t)USART3->RDR;
            if (n >= maxlen) break;
        }
        for (volatile int d = 0; d < 100; d++) { }  /* ~1 ms rough */
    }
    return n;
}

/* ------------------------------------------------------------------------ */
/* S8 CO₂ read (modbus-style command)                                         */
/* ------------------------------------------------------------------------ */
static int s8_read_co2(int16_t *ppm)
{
    /* S8 command: read CO₂ (command 0x04, register 0x0001, 2 bytes) */
    static const uint8_t cmd[] = { 0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD4, 0xC5 };
    uint8_t resp[7];

    uart3_tx(cmd, (int)sizeof(cmd));
    int n = uart3_rx(resp, sizeof(resp), 200);
    if (n < 7 || resp[0] != 0xFE) return -1;

    *ppm = (int16_t)((resp[3] << 8) | resp[4]);
    return 0;
}

/* ------------------------------------------------------------------------ */
/* SHT45 read (T, RH)                                                         */
/* ------------------------------------------------------------------------ */
static int sht45_read(int8_t *temp_c, uint8_t *rh_pct)
{
    uint8_t cmd = SHT45_CMD_MEASURE_HIGHREP;
    /* The SHT4x uses a command-only transaction (no register). */
    I2C1->CR2 = ((uint32_t)SHT45_I2C_ADDR << 1)
              | I2C_CR2_NBYTES(1)
              | (0 << I2C_CR2_RD_WRN);
    I2C1->CR2 |= I2C_CR2_START;
    while (!(I2C1->ISR & I2C_ISR_TXIS)) { }
    I2C1->TXDR = cmd;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;

    /* Wait for measurement (~8 ms with clock stretching; add margin). */
    for (volatile int d = 0; d < 20000; d++) { }

    /* Read 6 bytes: 2 T, 1 CRC, 2 RH, 1 CRC. */
    uint8_t buf[6];
    I2C1->CR2 = ((uint32_t)SHT45_I2C_ADDR << 1)
              | I2C_CR2_NBYTES(6)
              | I2C_CR2_RD_WRN
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;
    for (int i = 0; i < 6; i++) {
        while (!(I2C1->ISR & I2C_ISR_RXNE)) { }
        buf[i] = (uint8_t)I2C1->RXDR;
    }
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { }
    I2C1->ICR = I2C_ISR_STOPF;

    uint16_t t_raw = (uint16_t)((buf[0] << 8) | buf[1]);
    uint16_t rh_raw = (uint16_t)((buf[3] << 8) | buf[4]);

    /* SHT4x conversion: T = -45 + 175 * (t_raw/65535)
     *                    RH = 100 * (rh_raw/65535) clamped to [0,100] */
    float t = -45.0f + 175.0f * (float)t_raw / 65535.0f;
    float rh = 100.0f * (float)rh_raw / 65535.0f;
    if (rh < 0.0f) rh = 0.0f;
    if (rh > 100.0f) rh = 100.0f;

    *temp_c = (int8_t)(t + 0.5f);
    *rh_pct = (uint8_t)(rh + 0.5f);
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Si1145 UV index read                                                       */
/* ------------------------------------------------------------------------ */
static int si1145_read_uv(uint16_t *uv_raw)
{
    /* Trigger a UV measurement. */
    i2c_write_reg(SI1145_I2C_ADDR, SI1145_REG_COMMAND, SI1145_CMD_MEASUV);
    /* Wait for completion (~few ms). */
    for (volatile int d = 0; d < 10000; d++) { }
    uint8_t buf[2];
    i2c_read_burst(SI1145_I2C_ADDR, SI1145_REG_UVINDEX0, buf, 2);
    *uv_raw = (uint16_t)(buf[0] | (buf[1] << 8));
    return 0;
}

/* ------------------------------------------------------------------------ */
/* VEML6075 UVA/UVB read and UV index computation                             */
/* ------------------------------------------------------------------------ */
static int veml6075_read_uv(uint16_t *uva, uint16_t *uvb)
{
    uint8_t buf[2];
    i2c_read_burst(VEML6075_I2C_ADDR, VEML6075_REG_UVA, buf, 2);
    *uva = (uint16_t)(buf[0] | (buf[1] << 8));
    i2c_read_burst(VEML6075_I2C_ADDR, VEML6075_REG_UVB, buf, 2);
    *uvb = (uint16_t)(buf[0] | (buf[1] << 8));
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Public API                                                                 */
/* ------------------------------------------------------------------------ */
int env_init(void)
{
    uart3_init();

    /* Si1145: write HWKEY to enable. */
    i2c_write_reg(SI1145_I2C_ADDR, SI1145_REG_HWKEY, 0x17);
    /* VEML6075: activate, set integration time 800 ms (0x00 + active). */
    i2c_write_reg(VEML6075_I2C_ADDR, VEML6075_REG_UV_CONF, 0x00);

    s_initialized = 1;
    return 0;
}

int env_read(env_result_t *res)
{
    if (!s_initialized || res == NULL) return -1;
    memset(res, 0, sizeof(*res));

    int16_t co2 = 0;
    int8_t  t = 0;
    uint8_t rh = 0;
    uint16_t si_uv = 0;
    uint16_t uva = 0, uvb = 0;

    if (s8_read_co2(&co2) == 0)  res->co2_ppm = co2;
    if (sht45_read(&t, &rh) == 0) { res->temp_c = t; res->rh_pct = rh; }
    if (si1145_read_uv(&si_uv) == 0) { /* combine with VEML6075 below */ }
    if (veml6075_read_uv(&uva, &uvb) == 0) { /* used below */ }

    /* Combined UV index: VEML6075 provides UVA/UVB; Si1145 provides a
     * calibrated UV index. We average the Si1145 reading with a coarse
     * estimate from VEML6075 (UVA + UVB scaled). */
    float uv_si = (float)si_uv / 100.0f;          /* Si1145 UV index (0..11+) */
    float uv_veml = ((float)uva + (float)uvb) * 0.025f; /* coarse */
    float uv_avg = (uv_si + uv_veml) * 0.5f;
    if (uv_avg < 0.0f) uv_avg = 0.0f;
    if (uv_avg > 12.0f) uv_avg = 12.0f;
    res->uv_index_x10 = (uint8_t)(uv_avg * 10.0f + 0.5f);

    return 0;
}