/*
 * ov5640.c - OV5640 camera driver for Pollen Scout
 * Uses DCMI peripheral with DMA to capture binned RGB565 frames.
 * Strobe LEDs (white / UV 365 nm) are strobed via TIM4 PWM.
 *
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * SPDX-License-Identifier: GPL-3.0
 */
#include "ov5640.h"
#include "board.h"
#include "registers.h"
#include <string.h>

static uint8_t  s_last_frame[IMG_WIDTH * IMG_HEIGHT * IMG_BPP_BYTES]
    __attribute__((aligned(32)));
static ov5640_strobe_t s_strobe_sel = OV5640_STROBE_WHITE;
static volatile int s_capture_busy = 0;

/* ---- SCCB (I2C4) write ---- */
static int sccb_write(uint16_t reg, uint8_t val)
{
    I2C_CR2(I2C4_BASE) = (OV5640_I2C_ADDR) | (3U << 16) | BIT(13); /* AUTOEND */
    while (!(I2C_ISR(I2C4_BASE) & I2C_ISR_TXIS)) {
        if (I2C_ISR(I2C4_BASE) & I2C_ISR_NACKF) return -1;
    }
    I2C_TXDR(I2C4_BASE) = (reg >> 8) & 0xFF;
    while (!(I2C_ISR(I2C4_BASE) & I2C_ISR_TXIS)) { }
    I2C_TXDR(I2C4_BASE) = reg & 0xFF;
    while (!(I2C_ISR(I2C4_BASE) & I2C_ISR_TXIS)) { }
    I2C_TXDR(I2C4_BASE) = val;
    while (!(I2C_ISR(I2C4_BASE) & I2C_ISR_STOPF)) { }
    I2C_ISR(I2C4_BASE) = I2C_ISR_STOPF;  /* clear */
    return 0;
}

/* ---- Strobe pulse via TIM4 ---- */
static void strobe_fire(ov5640_strobe_t sel)
{
    uint32_t ccr = (SYSTEM_CLOCK / 1000000U) * STROBE_PULSE_US; /* ticks */
    if (sel == OV5640_STROBE_WHITE) {
        TIM_CCR1(TIM4_BASE) = ccr;
        TIM_CR1(TIM4_BASE) |= 1U;            /* enable */
        while (TIM_CR1(TIM4_BASE) & 1U) { }  /* one-shot via update */
    } else {
        TIM_CCR2(TIM4_BASE) = ccr;
        TIM_CR1(TIM4_BASE) |= 1U;
        while (TIM_CR1(TIM4_BASE) & 1U) { }
    }
}

int ov5640_init(void)
{
    /* Power-down then release */
    GPIO_REG(OV5640_PWDN_PORT, GPIO_BSRR_OFFSET) = (1U << (OV5640_PWDN_PIN + 16));
    for (volatile int i = 0; i < 100000; i++) { }
    GPIO_REG(OV5640_PWDN_PORT, GPIO_BSRR_OFFSET) = (1U << OV5640_PWDN_PIN);
    for (volatile int i = 0; i < 100000; i++) { }

    /* Reset low then high */
    GPIO_REG(OV5640_RESET_PORT, GPIO_BSRR_OFFSET) = (1U << (OV5640_RESET_PIN + 16));
    for (volatile int i = 0; i < 50000; i++) { }
    GPIO_REG(OV5640_RESET_PORT, GPIO_BSRR_OFFSET) = (1U << OV5640_RESET_PIN);

    /* Configure I2C4 timing for 400 kHz (simplified) */
    I2C_TIMINGR(I2C4_BASE) = 0x10909CEC;
    I2C_CR1(I2C4_BASE) = 1U;

    /* OV5640 banked register writes for binning + RGB565 */
    sccb_write(0x3008, 0x82);   /* reset */
    for (volatile int i = 0; i < 200000; i++) { }
    sccb_write(0x3008, 0x02);   /* power up */
    sccb_write(0x3503, 0x00);   /* AGC/AEC manual off */
    sccb_write(0x4300, 0x6F);   /* RGB565 */
    sccb_write(0x501F, 0x01);   /* ISP enabled */
    sccb_write(0x4201, 0x00);   /* enable AWB */
    /* Bin 2x2 -> 1280x960 from 2592x1944 */
    sccb_write(0x3A00, 0x78);   /* binning on */
    sccb_write(0x401E, 0x20);   /* H/V bin */
    sccb_write(0x3808, 0x05);   /* DVP HSTART high */
    sccb_write(0x3809, 0x00);
    sccb_write(0x380A, 0x03);
    sccb_write(0x380B, 0xC0);
    /* 60 fps timing */
    sccb_write(0x3802, 0x00);
    sccb_write(0x3803, 0x08);

    /* Configure DCMI: 8-bit, VSYNC/HSYNC, capture enable */
    DCMI_CR = (1U << 5) | (1U << 8);   /* ENABLE, EDIE (DMA) */

    /* TIM4 for strobe: one-shot, ~1 MHz tick */
    TIM_PSC(TIM4_BASE)  = (SYSTEM_CLOCK / 1000000U) - 1;
    TIM_ARR(TIM4_BASE)  = 0xFFFF;
    TIM_CCMR1(TIM4_BASE) = 0x6060;     /* PWM mode 1 both channels */
    TIM_CCER(TIM4_BASE)  = 0x0011;     /* CC1E + CC2E active high */
    TIM_EGR(TIM4_BASE)   = 1U;         /* update */

    s_capture_busy = 0;
    return 0;
}

void ov5640_strobe_select(ov5640_strobe_t sel)
{
    s_strobe_sel = sel;
}

int ov5640_capture(uint8_t *buf, size_t len)
{
    if (s_capture_busy) return -2;
    if (len < (size_t)IMG_WIDTH * IMG_HEIGHT * IMG_BPP_BYTES) return -3;
    s_capture_busy = 1;

    /* Arm DMA1 Stream0: peripheral=DCMI_DR, memory=buf, length=frame/4 */
    size_t ndtr = (IMG_WIDTH * IMG_HEIGHT * IMG_BPP_BYTES) / 4;
    DMA_SPAR(DMA1_Stream0_BASE) = (uint32_t)&DCMI_DR;
    DMA_SM0AR(DMA1_Stream0_BASE) = (uint32_t)buf;
    DMA_SNDTR(DMA1_Stream0_BASE) = ndtr;
    /* Channel select = DCMI (ch5), 16-bit->32-bit, MINC, PFCTRL */
    DMA_SCR(DMA1_Stream0_BASE) = (5U << 25) | (1U << 10) | (0b01 << 13) |
                                 (1U << 8) | (0b00 << 16);
    DMA_SCR(DMA1_Stream0_BASE) |= 1U;   /* EN */

    strobe_fire(s_strobe_sel);
    DCMI_CR |= 1U;                        /* capture enable */
    while (DMA_SCR(DMA1_Stream0_BASE) & 1U) { }  /* wait DMA done */
    DCMI_CR &= ~1U;

    memcpy(s_last_frame, buf,
           IMG_WIDTH * IMG_HEIGHT * IMG_BPP_BYTES);
    s_capture_busy = 0;
    return 0;
}

const uint8_t *ov5640_last_frame(void)
{
    return s_last_frame;
}