/*
 * adc_drv.c — ADS8866 16-bit SPI ADC driver implementation
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-2.0
 *
 * The ADS8866 is a 16-bit 1 MSPS SAR ADC with SPI interface.
 * Conversion is triggered by pulling CS low, then 16 bits of data
 * are clocked out on MISO. The ADC auto-converts on CS falling edge.
 *
 * For block reads, we use a timer-triggered CS toggle + DMA SPI RX
 * to achieve continuous 1 MSPS sampling.
 */

#include "adc_drv.h"
#include "registers.h"
#include "board.h"
#include <string.h>

static volatile bool block_done = false;
static uint32_t block_count = 0;
static uint16_t *block_buffer = NULL;

void adc_drv_init(void)
{
    /* Enable SPI1 clock */
    RCC_APB2ENR |= RCC_APB2ENR_SPI1;

    /* Enable GPIOB clock (already enabled by other drivers, but safe) */
    RCC_AHB2ENR |= RCC_AHB2ENR_GPIOB;

    /* Configure PB3 (SCK) as AF5 (SPI1_SCK) */
    GPIOB->MODER &= ~(3UL << (SPI1_SCK_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (SPI1_SCK_PIN * 2));
    GPIOB->AFRL &= ~(0xFUL << (SPI1_SCK_PIN * 4));
    GPIOB->AFRL |= (SPI1_AF << (SPI1_SCK_PIN * 4));
    GPIOB->OSPEEDR |= (GPIO_OSPEED_VHIGH << (SPI1_SCK_PIN * 2));

    /* Configure PB5 (MISO) as AF5 (SPI1_MISO) */
    GPIOB->MODER &= ~(3UL << (SPI1_MISO_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_AF << (SPI1_MISO_PIN * 2));
    GPIOB->AFRL &= ~(0xFUL << (SPI1_MISO_PIN * 4));
    GPIOB->AFRL |= (SPI1_AF << (SPI1_MISO_PIN * 4));
    GPIOB->PUPDR |= (GPIO_PUPD_PU << (SPI1_MISO_PIN * 2));

    /* Configure PB4 (CS) as output, default high */
    GPIOB->MODER &= ~(3UL << (SPI1_CS_PIN * 2));
    GPIOB->MODER |= (GPIO_MODE_OUTPUT << (SPI1_CS_PIN * 2));
    GPIOB->OTYPER &= ~(1UL << SPI1_CS_PIN);  /* Push-pull */
    GPIOB->BSRR = (1UL << SPI1_CS_PIN);     /* Set CS high (deselect) */

    /* Configure SPI1:
     * - Master mode
     * - CPOL=0, CPHA=0 (Mode 0)
     * - 16-bit data frame
     * - Baud rate: PCLK/4 = 170MHz/4 = 42.5 MHz (supports 1 MSPS with margin)
     * - MSB first
     * - Software NSS management */
    SPI1->CR1 = SPI_CR1_MSTR
              | SPI_CR1_SSI
              | SPI_CR1_SSM
              | SPI_CR1_BR_DIV4
              | SPI_CR1_CPOL_LOW
              | SPI_CR1_CPHA_1EDGE;

    SPI1->CR2 = SPI_CR2_DS_16B
              | SPI_CR2_SSOE
              | SPI_CR2_RXDMAEN;

    /* Enable DMA1 Channel 2 for SPI1 RX */
    RCC_AHB1ENR |= RCC_AHB1ENR_DMA1;
    DMAMUX1_Channel1 = DMAMUX_REQ_SPI1_RX;

    /* Enable SPI1 */
    SPI1->CR1 |= (1UL << 6);  /* SPE bit */
}

uint16_t adc_drv_read_single(void)
{
    /* Pull CS low to trigger conversion */
    GPIOB->BRR = (1UL << SPI1_CS_PIN);

    /* Write dummy data to generate clocks */
    SPI1->DR = 0x0000;

    /* Wait for RX data */
    uint32_t timeout = 10000;
    while (!(SPI1->SR & SPI_SR_RXNE) && timeout-- > 0)
        ;

    uint16_t result = (uint16_t)SPI1->DR;

    /* Pull CS high to end conversion */
    GPIOB->BSRR = (1UL << SPI1_CS_PIN);

    return result;
}

bool adc_drv_read_block(uint16_t *buffer, uint32_t count)
{
    if (!buffer || count == 0)
        return false;

    block_buffer = buffer;
    block_count = count;
    block_done = false;

    /* Configure DMA1 Channel 2 for SPI1 RX */
    DMA1_Ch2->CCR = 0;  /* Disable first */

    DMA1_Ch2->CCR = DMA_CCR_DIR_P2M
                  | DMA_CCR_MINC
                  | DMA_CCR_PSIZE_16B
                  | DMA_CCR_MSIZE_16B
                  | DMA_CCR_PL_VHIGH
                  | DMA_CCR_TCIE;  /* Transfer complete interrupt */

    DMA1_Ch2->CPAR = (uint32_t)&SPI1->DR;
    DMA1_Ch2->CM0AR = (uint32_t)buffer;
    DMA1_Ch2->CNDR = count;

    /* Clear any pending DMA flags */
    DMA1_IFCR = 0xFFFFFFFFUL;

    /* Pull CS low to start continuous conversion */
    GPIOB->BRR = (1UL << SPI1_CS_PIN);

    /* Enable DMA channel */
    DMA1_Ch2->CCR |= DMA_CCR_EN;

    /* Enable DMA interrupt in NVIC */
    NVIC_ISER(0) = (1UL << IRQ_DMA1_Ch2_3);

    return true;
}

bool adc_drv_block_complete(void)
{
    return block_done;
}

bool adc_drv_wait_block(uint32_t timeout_ms)
{
    uint32_t count = 0;
    while (!block_done && count < timeout_ms) {
        count++;
        /* Simple delay — in production use SysTick */
        for (volatile int i = 0; i < 17000; i++)
            ;
    }
    return block_done;
}

int16_t adc_drv_to_signed(uint16_t raw, uint16_t vref_mv)
{
    (void)vref_mv;  /* Voltage conversion not needed for lock-in */
    /* ADS8866 outputs 0-65535 for 0 to Vref.
     * Center at 32768 (midpoint) for AC-coupled signal. */
    return (int16_t)((int32_t)raw - 32768);
}

/* DMA1 Channel 2-3 interrupt handler */
void DMA1_Ch2_3_IRQHandler(void)
{
    if (DMA1_ISR & (1UL << 5)) {  /* Channel 2 transfer complete flag */
        DMA1_IFCR = (1UL << 5);   /* Clear flag */

        /* Pull CS high to stop conversion */
        GPIOB->BSRR = (1UL << SPI1_CS_PIN);

        block_done = true;

        /* Disable DMA channel */
        DMA1_Ch2->CCR &= ~DMA_CCR_EN;
    }
}