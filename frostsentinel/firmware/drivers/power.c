/*
 * drivers/power.c — Power management: solar MPPT, battery gauge, sleep
 *
 * Manages the SPV1050 solar MPPT/buck-boost charger, the LC709203F
 * battery fuel gauge (I²C), and the TPS62740 step-down regulator.
 * Implements the low-power entry sequence for Stop2 mode with RTC and
 * LoRa RX duty-cycle wake.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#include "power.h"
#include "../board.h"

/* ------------------------------------------------------------------ */
/*  I²C helper (shares I2C1 with other sensors)                        */
/* ------------------------------------------------------------------ */
static int i2c_write_reg(uint8_t dev, uint8_t reg, uint16_t val)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    i2c->CR2 = ((uint32_t)dev << 1) | (3u << 16) | I2C_CR2_START |
               I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = reg;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = (val >> 8) & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = val & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;
    return (to == 0) ? -1 : 0;
}

static int i2c_read_reg(uint8_t dev, uint8_t reg, uint16_t *out)
{
    I2C_TypeDef *i2c = I2C1;
    uint32_t to;

    /* Write register address */
    i2c->CR2 = ((uint32_t)dev << 1) | (1u << 16) | I2C_CR2_START;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TXE) && to--) ;
    i2c->TXDR = reg;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_TC) && to--) ;

    /* Repeated start, read 2 bytes */
    i2c->CR2 = ((uint32_t)dev << 1) | (2u << 16) | I2C_CR2_START |
               I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
    uint8_t hi = i2c->RXDR & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_RXNE) && to--) ;
    uint8_t lo = i2c->RXDR & 0xFF;
    to = 0x10000;
    while (!(i2c->ISR & I2C_ISR_STOPF) && to--) ;
    i2c->ICR = 0x20;

    *out = ((uint16_t)hi << 8) | lo;
    return (to == 0) ? -1 : 0;
}

/* ------------------------------------------------------------------ */
/*  LC709203F battery fuel gauge                                       */
/* ------------------------------------------------------------------ */
#define LC709203_REG_RSOC        0x0D   /* relative state of charge % */
#define LC709203_REG_VOLTAGE     0x09   /* cell voltage in mV */
#define LC709203_REG_TEMP        0x08   /* battery temperature */
#define LC709203_REG_APT         0x0C   /* adjustment parameter & thermistor */
#define LC709203_REG_STATUS      0x0A   /* status bit */

int power_gauge_init(void)
{
    /* Set thermistor mode (B-constant) and APA */
    i2c_write_reg(I2C_ADDR_LC709203, 0x12, 0x0001);  /* thermistor mode */
    delay_ms(5);
    i2c_write_reg(I2C_ADDR_LC709203, 0x0C, 0x000C);  /* APA for 1500mAh */
    delay_ms(5);
    i2c_write_reg(I2C_ADDR_LC709203, 0x0B, 0x0018);  /* B-constant 3950 */
    delay_ms(5);
    return 0;
}

int power_gauge_read(uint8_t *pct, uint16_t *mv)
{
    uint16_t rsoc = 0, volt = 0;
    if (i2c_read_reg(I2C_ADDR_LC709203, LC709203_REG_RSOC, &rsoc) < 0)
        return -1;
    if (i2c_read_reg(I2C_ADDR_LC709203, LC709203_REG_VOLTAGE, &volt) < 0)
        return -1;
    *pct = (uint8_t)(rsoc & 0xFF);
    *mv  = volt;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  SPV1050 solar MPPT — configured via analog pins, monitored via ADC */
/*  The SPV1050 is mostly autonomous; we just check solar voltage to   */
/*  set the SYS_FLAG_SOLAR_GOOD flag.                                  */
/* ------------------------------------------------------------------ */
int power_check_solar(uint8_t *solar_good)
{
    /* Read solar panel voltage via ADC channel 4 (PA4 not used elsewhere) */
    /* For simplicity, we check if battery is charging via LC709203 */
    uint16_t status = 0;
    if (i2c_read_reg(I2C_ADDR_LC709203, LC709203_REG_STATUS, &status) < 0) {
        *solar_good = 0;
        return -1;
    }
    /* Bit 0 of status indicates power direction (1 = discharging) */
    *solar_good = (status & 0x01) ? 0 : 1;
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Enter Stop2 low-power mode                                         */
/*                                                                    */
/*  Before entering Stop2:                                             */
/*    1. Flush flash metadata                                          */
/*    2. Set LoRa to RX duty-cycle                                     */
/*    3. Gate all unused peripheral clocks                             */
/*    4. Configure RTC alarm for next sample or slot wake              */
/*    5. Set PWR_CR1 LPMS = Stop2                                      */
/*    6. WFI (Wait For Interrupt)                                      */
/*                                                                    */
/*  On wake (RTC alarm or LoRa DIO1), the MCU returns to Run mode      */
/*  and the caller continues from after the WFI.                       */
/* ------------------------------------------------------------------ */
void power_enter_stop2(uint32_t wake_in_seconds)
{
    /* Flush flash */
    flashio_flush();

    /* Set LoRa duty-cycle RX */
    radio_sleep_duty();

    /* Gate unused clocks (keep I2C1 for RTC, LPUART for wake) */
    CLR_BITS(RCC->AHB2ENR, RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_AES1EN);
    CLR_BITS(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);

    /* Configure RTC alarm (handled by rtc driver) */
    /* rtc_set_alarm(wake_in_seconds); -- called by caller via rtc driver */

    /* Set LPMS = Stop2 */
    SET_FIELD(PWR->CR1, 0x7u, 0x2u);

    /* Ensure FLASH is in deep-power-down for Stop2 */
    /* (FLASH_ACR has a sleep-power-down bit; handled by HAL in production) */

    /* Wait for interrupt */
    __asm volatile ("wfi");

    /* --- Woken up --- */
    /* Restore clocks */
    SET_BITS(RCC->AHB2ENR, RCC_AHB2ENR_ADC12EN | RCC_AHB2ENR_AES1EN);
    SET_BITS(RCC->APB2ENR, RCC_APB2ENR_SPI1EN);

    /* Re-initialize SPI1 (it lost its config in Stop2) */
    /* In production this is done by radio_init's SPI config section;
     * for the bare-metal build we just re-enable. */
    SPI1->CR1 = 0;
    SPI1->CR2 = SPI_CR2_DS_8BIT | SPI_CR2_FRXTH | SPI_CR2_SSOE;
    SPI1->CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_DIV16;
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ------------------------------------------------------------------ */
/*  Public: update battery and solar status in g_sys                   */
/* ------------------------------------------------------------------ */
void power_update_status(void)
{
    uint8_t pct = 0, solar = 0;
    uint16_t mv = 0;

    power_gauge_read(&pct, &mv);
    power_check_solar(&solar);

    g_sys.battery_pct = pct;
    g_sys.battery_mv  = mv;

    if (pct < 20) {
        g_sys.flags |= SYS_FLAG_LOW_BATTERY;
    } else {
        g_sys.flags &= ~SYS_FLAG_LOW_BATTERY;
    }
    if (solar) {
        g_sys.flags |= SYS_FLAG_SOLAR_GOOD;
    } else {
        g_sys.flags &= ~SYS_FLAG_SOLAR_GOOD;
    }
}