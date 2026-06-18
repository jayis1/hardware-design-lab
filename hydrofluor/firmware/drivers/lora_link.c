/*
 * lora_link.c — HydroFluor LoRa (RFM95W / SX1276) telemetry uplink
 * Author: jayis1
 * License: MIT
 *
 * Drives the RFM95W module over SPI2. Used for fixed deployments where the
 * sonde is out of BLE range (e.g. lake buoy, intake) and a LoRaWAN-like
 * star topology delivers periodic summaries to a gateway up to ~15 km away.
 *
 * Implements the subset of SX1276 registers needed for LoRa modulation:
 * frequency synthesis, spreading factor, bandwidth, TX with CRC, and
 * DIO0 TxDone interrupt handling.
 */
#include "lora_link.h"
#include "../registers.h"
#include <string.h>

/* SX1276 register map (Semtech DS 0x3) */
#define REG_FIFO                 0x00
#define REG_OP_MODE              0x01
#define REG_FRF_MSB              0x06
#define REG_FRF_MID              0x07
#define REG_FRF_LSB              0x08
#define REG_PA_CONFIG            0x09
#define REG_FIFO_ADDR_PTR        0x0D
#define REG_FIFO_TX_BASE         0x0E
#define REG_MODEM_CONFIG1        0x1D
#define REG_MODEM_CONFIG2        0x1E
#define REG_MODEM_CONFIG3        0x26
#define REG_SYMB_TIMEOUT_LSB      0x1F
#define REG_PREAMBLE_MSB          0x20
#define REG_PREAMBLE_LSB          0x21
#define REG_PAYLOAD_LENGTH       0x22
#define REG_FIFO_RX_CURRENT       0x10
#define REG_IRQ_FLAGS            0x12
#define REG_IRQ_MASK             0x11
#define REG_VERSION              0x42

/* OP_MODE bits */
#define MODE_LORA               (1U << 7)
#define MODE_SLEEP               0x00
#define MODE_STDBY              0x01
#define MODE_TX                 0x03
#define MODE_RX_CONT            0x05

/* IRQ bits */
#define IRQ_TX_DONE             (1U << 3)
#define IRQ_RX_DONE             (1U << 6)
#define IRQ_CRC_ERR             (1U << 5)

static uint16_t g_interval_s = 0;
static uint32_t g_last_tx    = 0;

static inline void lora_cs_low(void)  { gpio_clr(LORA_CS_PORT, LORA_CS_PIN); }
static inline void lora_cs_high(void) { gpio_set(LORA_CS_PORT, LORA_CS_PIN); }

static uint8_t lora_read(uint8_t reg)
{
    lora_cs_low();
    spi_xfer(SPI2, reg & 0x7F);
    uint8_t v = spi_xfer(SPI2, 0xFF);
    lora_cs_high();
    return v;
}

static void lora_write(uint8_t reg, uint8_t val)
{
    lora_cs_low();
    spi_xfer(SPI2, 0x80 | reg);
    spi_xfer(SPI2, val);
    lora_cs_high();
}

static void lora_write_fifo(const uint8_t *buf, uint8_t len)
{
    lora_cs_low();
    spi_xfer(SPI2, 0x80 | REG_FIFO);
    for (uint8_t i = 0; i < len; ++i) spi_xfer(SPI2, buf[i]);
    lora_cs_high();
}

int lora_init(uint32_t freq_hz, uint8_t sf, uint32_t bw_hz)
{
    /* Enable SPI2 (APB1ENR1 bit 14) + GPIOB */
    RCC->APB1ENR1 |= (1U << 14);
    RCC->AHB2ENR  |= (1U << 1);

    /* SPI2 pins: PB13 SCK, PB14 MISO, PB15 MOSI (AF5) */
    gpio_mode(GPIOB, LORA_SCK_PIN,  GPIO_MODE_AF); gpio_af(GPIOB, LORA_SCK_PIN, 5);
    gpio_mode(GPIOB, LORA_MISO_PIN, GPIO_MODE_AF); gpio_af(GPIOB, LORA_MISO_PIN, 5);
    gpio_mode(GPIOB, LORA_MOSI_PIN, GPIO_MODE_AF); gpio_af(GPIOB, LORA_MOSI_PIN, 5);
    gpio_mode(LORA_CS_PORT,  LORA_CS_PIN,  GPIO_MODE_OUTPUT); gpio_set(LORA_CS_PORT, LORA_CS_PIN);
    gpio_mode(LORA_RST_PORT, LORA_RST_PIN, GPIO_MODE_OUTPUT); gpio_set(LORA_RST_PORT, LORA_RST_PIN);
    gpio_mode(LORA_DIO0_PORT, LORA_DIO0_PIN, GPIO_MODE_INPUT);
    gpio_mode(LORA_DIO1_PORT, LORA_DIO1_PIN, GPIO_MODE_INPUT);

    SPI2->CR1 = 0;
    SPI2->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH;
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_BR_DIV8;
    SPI2->CR1 |= SPI_CR1_SPE;

    /* Hardware reset RFM95W (active low) */
    gpio_clr(LORA_RST_PORT, LORA_RST_PIN);
    for (volatile int i = 0; i < 10000; ++i) { }
    gpio_set(LORA_RST_PORT, LORA_RST_PIN);
    for (volatile int i = 0; i < 10000; ++i) { }

    /* Check chip version (0x12 expected for SX1276) */
    if (lora_read(REG_VERSION) != 0x12) return -1;

    /* Sleep mode to configure */
    lora_write(REG_OP_MODE, MODE_LORA | MODE_SLEEP);
    for (volatile int i = 0; i < 1000; ++i) { }

    /* Frequency: frf = freq * 2^19 / 32 MHz */
    uint64_t frf = ((uint64_t)freq_hz << 19) / 32000000ULL;
    lora_write(REG_FRF_MSB, (uint8_t)(frf >> 16));
    lora_write(REG_FRF_MID, (uint8_t)(frf >> 8));
    lora_write(REG_FRF_LSB, (uint8_t)(frf >> 0));

    /* PA_CONFIG: PA_BOOST, max output 17 dBm */
    lora_write(REG_PA_CONFIG, 0x80 | 0x70 | 0x0F);  /* PA_BOOST | max */

    /* Modem config 1: BW + CR + implicit header off
     * BW table: 0=7.8k,1=10.4k,2=15.6k,3=20.8k,4=31.25k,5=41.7k,6=62.5k,7=125k
     * CR table: 1=4/5, 2=4/6, 3=4/7, 4=4/8 */
    uint8_t bw_bits = 7;  /* 125 kHz default */
    if      (bw_hz <=  7800)  bw_bits = 0;
    else if (bw_hz <= 10400)  bw_bits = 1;
    else if (bw_hz <= 15600)  bw_bits = 2;
    else if (bw_hz <= 20800)  bw_bits = 3;
    else if (bw_hz <= 31250)  bw_bits = 4;
    else if (bw_hz <= 41700)  bw_bits = 5;
    else if (bw_hz <= 62500)  bw_bits = 6;
    else                      bw_bits = 7;
    lora_write(REG_MODEM_CONFIG1, (bw_bits << 4) | 0x02 /* CR 4/6 */ | 0x00 /* explicit header */);

    /* Modem config 2: spreading factor + CRC on */
    if (sf < 7)  sf = 7;
    if (sf > 12) sf = 12;
    lora_write(REG_MODEM_CONFIG2, (sf << 4) | 0x04 /* CRC on */);

    /* Modem config 3: LNA gain, auto AGC on */
    lora_write(REG_MODEM_CONFIG3, 0x04 /* AGC auto on */);

    /* Preamble length 8 */
    lora_write(REG_PREAMBLE_MSB, 0x00);
    lora_write(REG_PREAMBLE_LSB, 0x08);

    /* Standby */
    lora_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    return 0;
}

int lora_uplink_sample(const sample_record_t *rec)
{
    /* Compact 16-byte summary for LoRa (keep airtime short) */
    uint8_t pkt[16];
    pkt[0]  = (uint8_t)(rec->seq & 0xFF);
    pkt[1]  = (uint8_t)(rec->seq >> 8);
    pkt[2]  = (uint8_t)(rec->depth_cm & 0xFF);
    pkt[3]  = (uint8_t)(rec->depth_cm >> 8);
    pkt[4]  = (uint8_t)(rec->temp_c01 & 0xFF);
    pkt[5]  = (uint8_t)(rec->temp_c01 >> 8);
    pkt[6]  = (uint8_t)(rec->battery_mv & 0xFF);
    pkt[7]  = (uint8_t)(rec->battery_mv >> 8);
    pkt[8]  = (uint8_t)(rec->cdom_ppb & 0xFF);
    pkt[9]  = (uint8_t)(rec->cdom_ppb >> 8);
    pkt[10] = (uint8_t)(rec->chla_ugl & 0xFF);
    pkt[11] = (uint8_t)(rec->chla_ugl >> 8);
    pkt[12] = (uint8_t)(rec->phyc_ugl & 0xFF);
    pkt[13] = (uint8_t)(rec->phyc_ugl >> 8);
    pkt[14] = (uint8_t)(rec->oil_ppb & 0xFF);
    pkt[15] = (uint8_t)(rec->oil_ppb >> 8);

    /* Standby, set FIFO base, write payload, set length, TX */
    lora_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    lora_write(REG_FIFO_ADDR_PTR, 0x00);
    lora_write(REG_FIFO_TX_BASE,  0x00);
    lora_write_fifo(pkt, sizeof(pkt));
    lora_write(REG_PAYLOAD_LENGTH, sizeof(pkt));
    lora_write(REG_IRQ_FLAGS, 0xFF);   /* clear any pending */
    lora_write(REG_OP_MODE, MODE_LORA | MODE_TX);
    g_last_tx = 0;   /* caller sets via lora_poll */
    return 0;
}

void lora_poll(void)
{
    uint8_t irq = lora_read(REG_IRQ_FLAGS);
    if (irq & IRQ_TX_DONE) {
        lora_write(REG_IRQ_FLAGS, IRQ_TX_DONE);
        lora_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    }
}

void lora_set_interval(uint16_t seconds)
{
    g_interval_s = seconds;
}