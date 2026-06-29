/*
 * drivers/sx1276.c — SX1276 LoRa transceiver driver (SPI2)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Minimal driver for LoRa mode TX/RX on the RFM95W module.
 * Uses SPI2 with bit-banged NSS on PB4. Configured for HiveVox
 * low-power uplinks: mostly TX, occasional RX for downlink commands.
 */
#include "sx1276.h"
#include "../registers.h"
#include "../board.h"

/* ---- SPI2 + GPIO helpers ------------------------------------------- */
#define LORA_NSS_PIN  4  /* PB4 */
#define LORA_RST_PIN  5  /* PB5 */
#define LORA_DIO0_PIN 6  /* PB6 */

static inline void nss_low(void)  { GPIOB->BRR  = (1U << LORA_NSS_PIN); }
static inline void nss_high(void) { GPIOB->BSRR = (1U << LORA_NSS_PIN); }
static inline void rst_low(void)  { GPIOB->BRR  = (1U << LORA_RST_PIN); }
static inline void rst_high(void) { GPIOB->BSRR = (1U << LORA_RST_PIN); }

static void spi2_init(void)
{
    /* Enable SPI2 clock */
    RCC_APB1ENR1 |= RCC_APB1ENR1_SPI2EN;

    /* PB13(SCK) PB14(MISO) PB15(MOSI) as AF5, PB4 as GPIO output (NSS) */
    GPIOB->MODER &= ~((3U<<(13*2)) | (3U<<(14*2)) | (3U<<(15*2)) | (3U<<(4*2)));
    GPIOB->MODER |=  (GPIO_MODE_AF<<(13*2)) | (GPIO_MODE_AF<<(14*2))
                   |  (GPIO_MODE_AF<<(15*2)) | (GPIO_MODE_OUTPUT<<(4*2));
    GPIOB->AFRH &= ~((0xFU<<(5*4)) | (0xFU<<(6*4)) | (0xFU<<(7*4)));
    GPIOB->AFRH |=  (5U<<(5*4)) | (5U<<(6*4)) | (5U<<(7*4));  /* AF5 = SPI2 */
    GPIOB->OSPEEDR |= (3U<<(13*2)) | (3U<<(15*2));  /* high speed for SCK/MOSI */

    nss_high();

    /* SPI2: master, CPOL=0 CPHA=0, 8-bit, baud PCLK/16 (~5 MHz at 80/16) */
    SPI2->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI
              | (3U << SPI_CR1_BR_SHIFT);  /* /16 */
    SPI2->CR2 = SPI_CR2_FRXTH | SPI_CR2_DS_8BIT;
    SPI2->CR1 |= SPI_CR1_SPE;
}

static uint8_t spi2_xfer(uint8_t tx)
{
    while (!(SPI2->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI2->DR = tx;
    while (!(SPI2->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI2->DR;
}

/* ---- SX1276 register access ---------------------------------------- */
static uint8_t sx1276_read(uint8_t reg)
{
    nss_low();
    spi2_xfer(reg & 0x7F);     /* bit7=0 → read */
    uint8_t val = spi2_xfer(0x00);
    nss_high();
    return val;
}

static void sx1276_write(uint8_t reg, uint8_t val)
{
    nss_low();
    spi2_xfer(reg | 0x80);     /* bit7=1 → write */
    spi2_xfer(val);
    nss_high();
}

static void sx1276_write_burst(uint8_t reg, const uint8_t *buf, uint8_t n)
{
    nss_low();
    spi2_xfer(reg | 0x80);
    for (uint8_t i = 0; i < n; i++) spi2_xfer(buf[i]);
    nss_high();
}

static void sx1276_read_burst(uint8_t reg, uint8_t *buf, uint8_t n)
{
    nss_low();
    spi2_xfer(reg & 0x7F);
    for (uint8_t i = 0; i < n; i++) buf[i] = spi2_xfer(0x00);
    nss_high();
}

/* ---- SX1276 register addresses ------------------------------------- */
#define REG_FIFO             0x00
#define REG_OP_MODE          0x01
#define REG_FRF_MSB          0x06
#define REG_FRF_MID          0x07
#define REG_FRF_LSB          0x08
#define REG_PA_CONFIG        0x09
#define REG_FIFO_ADDR_PTR    0x0D
#define REG_FIFO_TX_BASE     0x0E
#define REG_FIFO_RX_BASE     0x0F
#define REG_FIFO_RX_CURRENT  0x10
#define REG_RX_NB_BYTES      0x13
#define REG_PKT_SNR          0x19
#define REG_PKT_RSSI         0x1A
#define REG_MODEM_CONFIG1    0x1D
#define REG_MODEM_CONFIG2    0x1E
#define REG_MODEM_CONFIG3    0x26
#define REG_PREAMBLE_LSB     0x20
#define REG_PAYLOAD_LENGTH   0x22
#define REG_SYNC_WORD        0x39
#define REG_DIO_MAPPING1     0x40
#define REG_VERSION          0x42

/* Modes */
#define MODE_SLEEP          0x00
#define MODE_STDBY          0x01
#define MODE_TX             0x03
#define MODE_RX_CONT        0x05
#define MODE_LORA           0x80   /* long range mode bit */

/* ---- Public API ---------------------------------------------------- */

int sx1276_init(void)
{
    spi2_init();

    /* Hardware reset */
    rst_low();
    for (volatile int i = 0; i < 100000; i++) { }  /* ~10 ms */
    rst_high();
    for (volatile int i = 0; i < 100000; i++) { }

    /* Check chip version (should be 0x12 for SX1276) */
    if (sx1276_read(REG_VERSION) != 0x12) return -1;

    /* Enter sleep mode to set LoRa mode (can only switch in sleep) */
    sx1276_write(REG_OP_MODE, MODE_SLEEP);
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_SLEEP);
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);

    /* Set frequency: frf = freq * 2^19 / 32MHz */
    uint64_t frf = ((uint64_t)LORA_FREQ_HZ << 19) / 32000000ULL;
    sx1276_write(REG_FRF_MSB, (uint8_t)(frf >> 16));
    sx1276_write(REG_FRF_MID, (uint8_t)(frf >> 8));
    sx1276_write(REG_FRF_LSB, (uint8_t)(frf));

    /* PA config: PA_BOOST, max power +20 dBm */
    sx1276_write(REG_PA_CONFIG, 0xFF);  /* 0xF0 | 0x0F */

    /* Lora modem config 1: BW 125 kHz (0x70), CR 4/5 (0x02), implicit hdr off */
    sx1276_write(REG_MODEM_CONFIG1, 0x72);  /* BW125k, CR4/5, explicit header */

    /* Modem config 2: SF7, CRC on, TX mode */
    sx1276_write(REG_MODEM_CONFIG2, (LORA_SF_DEFAULT << 4) | 0x04);

    /* Modem config 3: LNA boost, AGC auto on */
    sx1276_write(REG_MODEM_CONFIG3, 0x04);

    /* Preamble length */
    sx1276_write(REG_PREAMBLE_LSB, LORA_PREAMBLE_LEN);

    /* Sync word (private network) */
    sx1276_write(REG_SYNC_WORD, LORA_SYNC_WORD);

    /* DIO0 = TxDone / RxDone mapping */
    sx1276_write(REG_DIO_MAPPING1, 0x00);

    return 0;
}

int sx1276_tx(const uint8_t *payload, uint8_t len)
{
    /* Set standby */
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);

    /* FIFO pointer at TX base */
    sx1276_write(REG_FIFO_ADDR_PTR, sx1276_read(REG_FIFO_TX_BASE));

    /* Write payload to FIFO */
    sx1276_write_burst(REG_FIFO, payload, len);

    /* Set payload length */
    sx1276_write(REG_PAYLOAD_LENGTH, len);

    /* Clear any pending TxDone flag by reading IRQ status (RegIrqFlags 0x12) */
    sx1276_read(0x12);

    /* DIO0 mapping: 0x40 for TxDone in TX mode */
    sx1276_write(REG_DIO_MAPPING1, 0x40);

    /* Enter TX mode */
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_TX);

    /* Wait for DIO0 to go high (TxDone) with timeout */
    uint32_t timeout = 5000000;  /* generous; ~5 s worst case at SF12 */
    while (!((GPIOB->IDR >> LORA_DIO0_PIN) & 1) && timeout--) { }
    if (!timeout) return -1;

    /* Clear IRQ flags */
    sx1276_write(0x12, 0xFF);

    /* Back to standby */
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    return 0;
}

int sx1276_rx(uint8_t *buf, uint8_t max_len, uint32_t timeout_ms)
{
    /* Set RX continuous */
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    sx1276_write(REG_DIO_MAPPING1, 0x00);  /* DIO0 = RxDone */
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_RX_CONT);

    uint32_t t = timeout_ms;
    while (!((GPIOB->IDR >> LORA_DIO0_PIN) & 1) && t--) {
        delay_ms(1);
    }
    if (!t) {
        sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
        return -1;
    }

    uint8_t irq = sx1276_read(0x12);
    sx1276_write(0x12, 0xFF);  /* clear flags */

    if (irq & 0x20) {
        /* CRC error */
        sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
        return -2;
    }

    uint8_t len = sx1276_read(REG_RX_NB_BYTES);
    uint8_t addr = sx1276_read(REG_FIFO_RX_CURRENT);
    sx1276_write(REG_FIFO_ADDR_PTR, addr);
    if (len > max_len) len = max_len;
    sx1276_read_burst(REG_FIFO, buf, len);

    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    return len;
}

void sx1276_sleep(void)
{
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_SLEEP);
}

int8_t sx1276_last_rssi(void)
{
    uint8_t r = sx1276_read(REG_PKT_RSSI);
    return (int8_t)(r - 157);  /* RSSI = value - 157 dBm (HF port) */
}

void sx1276_set_sf(uint8_t sf)
{
    if (sf < 7) sf = 7;
    if (sf > 12) sf = 12;
    sx1276_write(REG_OP_MODE, MODE_LORA | MODE_STDBY);
    uint8_t cfg2 = sx1276_read(REG_MODEM_CONFIG2);
    cfg2 = (cfg2 & 0x0F) | (sf << 4);
    sx1276_write(REG_MODEM_CONFIG2, cfg2);
}