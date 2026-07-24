/*
 * flash_drv.c — W25Q128 SPI flash driver implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * Drives the W25Q128 16 MB SPI flash via SPI3 on the STM32G474.
 * Used for storing the alloy reference database, calibration data,
 * and the scan log (up to 100,000 scan records).
 */

#include "flash_drv.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static bool flash_initialized = false;

/* ---- Low-level SPI3 helpers ---- */

static void spi3_select(void)
{
    GPIOA->BRR = (1UL << FLASH_CS_PIN);  /* CS low */
}

static void spi3_deselect(void)
{
    GPIOA->BSRR = (1UL << FLASH_CS_PIN);  /* CS high */
}

static uint8_t spi3_transfer(uint8_t data)
{
    while (!(SPI3->SR & SPI_SR_TXE))
        ;
    *(volatile uint8_t *)&SPI3->DR = data;
    while (!(SPI3->SR & SPI_SR_RXNE))
        ;
    return (uint8_t)SPI3->DR;
}

static void spi3_write_buf(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        while (!(SPI3->SR & SPI_SR_TXE))
            ;
        *(volatile uint8_t *)&SPI3->DR = data[i];
    }
    while (SPI3->SR & SPI_SR_BSY)
        ;
    while (SPI3->SR & SPI_SR_RXNE)
        (void)SPI3->DR;
}

static void spi3_read_buf(uint8_t *data, uint32_t len)
{
    /* Send dummy bytes to clock out data */
    for (uint32_t i = 0; i < len; i++) {
        while (!(SPI3->SR & SPI_SR_TXE))
            ;
        *(volatile uint8_t *)&SPI3->DR = 0xFF;
        while (!(SPI3->SR & SPI_SR_RXNE))
            ;
        data[i] = (uint8_t)SPI3->DR;
    }
}

/* ---- Public API ---- */

bool flash_drv_init(void)
{
    if (flash_initialized)
        return true;

    /* Enable clocks */
    RCC_APB2ENR |= RCC_APB2ENR_SPI1;  /* SPI3 is on APB1 in some configs */
    /* Actually SPI3 is on APB1 on G474 */
    RCC_APB1ENR1 |= (1UL << 15);  /* SPI3EN bit */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOA | RCC_AHB2ENR_GPIOC;

    /* Configure PC10 (SCK), PC11 (MISO), PC12 (MOSI) as AF6 (SPI3) */
    GPIOC->MODER &= ~(3UL << (FLASH_SCK_PIN * 2));
    GPIOC->MODER |= (GPIO_MODE_AF << (FLASH_SCK_PIN * 2));
    GPIOC->AFRH &= ~(0xFUL << ((FLASH_SCK_PIN - 8) * 4));
    GPIOC->AFRH |= (FLASH_AF << ((FLASH_SCK_PIN - 8) * 4));
    GPIOC->OSPEEDR |= (GPIO_OSPEED_VHIGH << (FLASH_SCK_PIN * 2));

    GPIOC->MODER &= ~(3UL << (FLASH_MISO_PIN * 2));
    GPIOC->MODER |= (GPIO_MODE_AF << (FLASH_MISO_PIN * 2));
    GPIOC->AFRH &= ~(0xFUL << ((FLASH_MISO_PIN - 8) * 4));
    GPIOC->AFRH |= (FLASH_AF << ((FLASH_MISO_PIN - 8) * 4));

    GPIOC->MODER &= ~(3UL << (FLASH_MOSI_PIN * 2));
    GPIOC->MODER |= (GPIO_MODE_AF << (FLASH_MOSI_PIN * 2));
    GPIOC->AFRH &= ~(0xFUL << ((FLASH_MOSI_PIN - 8) * 4));
    GPIOC->AFRH |= (FLASH_AF << ((FLASH_MOSI_PIN - 8) * 4));

    /* Configure PA4 (CS) as output, default high */
    GPIOA->MODER &= ~(3UL << (FLASH_CS_PIN * 2));
    GPIOA->MODER |= (GPIO_MODE_OUTPUT << (FLASH_CS_PIN * 2));
    GPIOA->BSRR = (1UL << FLASH_CS_PIN);

    /* Configure SPI3: Master, Mode 0, 8-bit, /4 baud */
    SPI3->CR1 = SPI_CR1_MSTR | SPI_CR1_SSI | SPI_CR1_SSM
              | SPI_CR1_BR_DIV4 | SPI_CR1_CPOL_LOW | SPI_CR1_CPHA_1EDGE;
    SPI3->CR2 = SPI_CR2_DS_8B | SPI_CR2_SSOE;
    SPI3->CR1 |= (1UL << 6);  /* SPE */

    /* Verify flash is present */
    uint32_t id = flash_drv_read_id();
    if ((id & 0xFFFFFF) != W25Q128_JEDEC_ID)
        return false;  /* Flash not detected */

    flash_initialized = true;
    return true;
}

uint32_t flash_drv_read_id(void)
{
    uint32_t id = 0;
    spi3_select();
    spi3_transfer(FLASH_CMD_READ_ID);
    id = ((uint32_t)spi3_transfer(0xFF) << 16);
    id |= ((uint32_t)spi3_transfer(0xFF) << 8);
    id |= (uint32_t)spi3_transfer(0xFF);
    spi3_deselect();
    return id;
}

uint8_t flash_drv_read_status(void)
{
    spi3_select();
    spi3_transfer(FLASH_CMD_READ_STATUS);
    uint8_t status = spi3_transfer(0xFF);
    spi3_deselect();
    return status;
}

bool flash_drv_wait_idle(uint32_t timeout_ms)
{
    uint32_t count = 0;
    while (flash_drv_read_status() & FLASH_SR_BUSY) {
        count++;
        if (count > timeout_ms * 170)
            return false;  /* Timeout */
    }
    return true;
}

bool flash_drv_read(uint32_t addr, uint8_t *data, uint32_t len)
{
    if (!flash_initialized || addr + len > FLASH_TOTAL_SIZE)
        return false;

    spi3_select();
    spi3_transfer(FLASH_CMD_READ);
    spi3_transfer((addr >> 16) & 0xFF);
    spi3_transfer((addr >> 8) & 0xFF);
    spi3_transfer(addr & 0xFF);
    spi3_read_buf(data, len);
    spi3_deselect();
    return true;
}

bool flash_drv_write_page(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (!flash_initialized || len > FLASH_PAGE_SIZE)
        return false;

    /* Enable write */
    spi3_select();
    spi3_transfer(FLASH_CMD_WRITE_EN);
    spi3_deselect();

    /* Page program */
    spi3_select();
    spi3_transfer(FLASH_CMD_PAGE_PROG);
    spi3_transfer((addr >> 16) & 0xFF);
    spi3_transfer((addr >> 8) & 0xFF);
    spi3_transfer(addr & 0xFF);
    spi3_write_buf(data, len);
    spi3_deselect();

    return flash_drv_wait_idle(100);
}

bool flash_drv_erase_sector(uint32_t addr)
{
    spi3_select();
    spi3_transfer(FLASH_CMD_WRITE_EN);
    spi3_deselect();

    spi3_select();
    spi3_transfer(FLASH_CMD_SECTOR_ERASE);
    spi3_transfer((addr >> 16) & 0xFF);
    spi3_transfer((addr >> 8) & 0xFF);
    spi3_transfer(addr & 0xFF);
    spi3_deselect();

    return flash_drv_wait_idle(500);  /* 400 ms typ, 500 ms max */
}

bool flash_drv_erase_block_64k(uint32_t addr)
{
    spi3_select();
    spi3_transfer(FLASH_CMD_WRITE_EN);
    spi3_deselect();

    spi3_select();
    spi3_transfer(FLASH_CMD_BLOCK_ERASE_64K);
    spi3_transfer((addr >> 16) & 0xFF);
    spi3_transfer((addr >> 8) & 0xFF);
    spi3_transfer(addr & 0xFF);
    spi3_deselect();

    return flash_drv_wait_idle(2000);  /* 2 sec max */
}

bool flash_drv_write(uint32_t addr, const uint8_t *data, uint32_t len)
{
    if (!flash_initialized)
        return false;

    uint32_t offset = 0;
    while (offset < len) {
        uint32_t page_remaining = FLASH_PAGE_SIZE - ((addr + offset) % FLASH_PAGE_SIZE);
        uint32_t chunk = (len - offset < page_remaining) ? (len - offset) : page_remaining;

        if (!flash_drv_write_page(addr + offset, data + offset, chunk))
            return false;

        offset += chunk;
    }
    return true;
}

void flash_drv_power_down(void)
{
    spi3_select();
    spi3_transfer(FLASH_CMD_POWER_DOWN);
    spi3_deselect();
}

void flash_drv_power_up(void)
{
    spi3_select();
    spi3_transfer(FLASH_CMD_POWER_UP);
    spi3_deselect();
    /* Wait for wake-up (~3 us) */
    for (volatile int i = 0; i < 100; i++)
        ;
}