/*
 * registers.h — nRF5340 application-core peripheral register definitions.
 *                Lightweight, no SDK HAL. Only peripherals used by Synthand.
 *
 * Author: jayis1
 * Copyright (C) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

#ifndef SYNTHAND_REGISTERS_H
#define SYNTHAND_REGISTERS_H

#include <stdint.h>

/* -------------------------------------------------------------------------
 * nRF5340 base addresses (application core)
 * ------------------------------------------------------------------------- */
#define PERIPH_BASE         0x40000000U

#define CLOCK_BASE          (PERIPH_BASE + 0x00005000U)
#define POWER_BASE          (PERIPH_BASE + 0x00000000U)
#define RADIO_BASE          (PERIPH_BASE + 0x01000000U) /* net core — ref only */
#define RTC0_BASE           (PERIPH_BASE + 0x00014000U)
#define RTC1_BASE           (PERIPH_BASE + 0x00015000U)
#define TIMER0_BASE         (PERIPH_BASE + 0x00008000U)
#define TIMER1_BASE         (PERIPH_BASE + 0x00009000U)
#define TIMER2_BASE         (PERIPH_BASE + 0x0000A000U)
#define SPIM0_BASE          (PERIPH_BASE + 0x0002D000U)
#define SPIM1_BASE          (PERIPH_BASE + 0x0002E000U)
#define SPIM2_BASE          (PERIPH_BASE + 0x0002F000U)
#define TWIM0_BASE          (PERIPH_BASE + 0x00034000U)
#define TWIM1_BASE          (PERIPH_BASE + 0x00035000U)
#define SAADC_BASE          (PERIPH_BASE + 0x0000E000U)
#define GPIOTE_BASE         (PERIPH_BASE + 0x00031000U)
#define P0_BASE             (PERIPH_BASE + 0x008C0000U) /* P0 port */
#define P1_BASE             (PERIPH_BASE + 0x008D0000U) /* P1 port */
#define PWM0_BASE           (PERIPH_BASE + 0x00021000U)
#define IPC_BASE            (PERIPH_BASE + 0x0002A000U)
#define FLASH_BASE          (PERIPH_BASE + 0x00039000U)
#define NVMC_BASE           (PERIPH_BASE + 0x00039000U)
#define WDT_BASE            (PERIPH_BASE + 0x0000F000U)
#define USBD_BASE           (PERIPH_BASE + 0x00027000U)
#define UICR_BASE           (0x00FF8000U)

/* -------------------------------------------------------------------------
 * GPIO (P0 / P1 — nRF53 has two 32-pin ports)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t RESERVED0[321];
    volatile uint32_t OUT;          /* 0x504 */
    volatile uint32_t OUTSET;       /* 0x508 */
    volatile uint32_t OUTCLR;       /* 0x50C */
    volatile uint32_t IN;           /* 0x510 */
    volatile uint32_t DIR;          /* 0x514 */
    volatile uint32_t DIRSET;       /* 0x518 */
    volatile uint32_t DIRCLR;       /* 0x51C */
    volatile uint32_t LATCH;        /* 0x520 */
    volatile uint32_t DETECTMODE;   /* 0x524 */
    volatile uint32_t RESERVED1[118];
    volatile uint32_t PIN_CNF[32];  /* 0x700 — 32 pins */
} NRF_GPIO_TypeDef;

#define P0  ((NRF_GPIO_TypeDef *)P0_BASE)
#define P1  ((NRF_GPIO_TypeDef *)P1_BASE)

/* PIN_CNF bits */
#define GPIO_CNF_DIR_INPUT      (0U << 0)
#define GPIO_CNF_DIR_OUTPUT     (1U << 0)
#define GPIO_CNF_INPUT_CONNECT  (0U << 1)
#define GPIO_CNF_INPUT_DISCONNECT (3U << 1) /* NRF_GPIO_PIN_NOPULL */
#define GPIO_CNF_PULL_NONE      (0U << 2)
#define GPIO_CNF_PULL_DOWN      (1U << 2)
#define GPIO_CNF_PULL_UP        (3U << 2)
#define GPIO_CNF_S0S1           (0U << 8)  /* standard push-pull */
#define GPIO_CNF_H0S1           (1U << 8)
#define GPIO_CNF_SENSE_DISABLE  (0U << 16)
#define GPIO_CNF_SENSE_HIGH     (2U << 16)
#define GPIO_CNF_SENSE_LOW      (3U << 16)

/* -------------------------------------------------------------------------
 * CLOCK — high/low frequency clock control
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_HFCLKSTART;   /* 0x000 */
    volatile uint32_t TASKS_HFCLKSTOP;    /* 0x004 */
    volatile uint32_t TASKS_LFCLKSTART;   /* 0x008 */
    volatile uint32_t TASKS_LFCLKSTOP;    /* 0x00C */
    volatile uint32_t RESERVED0[60];
    volatile uint32_t EVENTS_HFCLKSTARTED;/* 0x100 */
    volatile uint32_t EVENTS_LFCLKSTARTED;/* 0x104 */
    volatile uint32_t RESERVED1[62];
    volatile uint32_t HFCLKRUN;           /* 0x200 */
    volatile uint32_t HFCLKSTAT;          /* 0x204 */
    volatile uint32_t RESERVED2[1];
    volatile uint32_t LFCLKRUN;           /* 0x20C */
    volatile uint32_t LFCLKSTAT;          /* 0x210 */
    volatile uint32_t LFCLKSRCCOPY;       /* 0x214 */
    volatile uint32_t RESERVED3[62];
    volatile uint32_t LFCLKSRC;           /* 0x318 */
    volatile uint32_t HFXODEBOUNCE;       /* 0x31C */
} NRF_CLOCK_TypeDef;

#define CLOCK ((NRF_CLOCK_TypeDef *)CLOCK_BASE)

#define CLOCK_LFCLKSRC_LFXO   (2U << 0)   /* external 32.768 kHz crystal */
#define CLOCK_HFCLKSTAT_STATE_Msk (1U << 0)
#define CLOCK_HFCLKSTAT_SRC_Msk   (1U << 1) /* 0=HFINT, 1=HFXO */

/* -------------------------------------------------------------------------
 * POWER — power management, RAM retention
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t RESERVED0[1];
    volatile uint32_t TASKS_LOWPWR;       /* 0x004 */
    volatile uint32_t RESERVED1[62];
    volatile uint32_t EVENTS_POFWARN;     /* 0x100 */
    volatile uint32_t RESERVED2[63];
    volatile uint32_t INTENSET;           /* 0x304 */
    volatile uint32_t INTENCLR;           /* 0x308 */
    volatile uint32_t RESERVED3[1];
    volatile uint32_t MAINREGSTATUS;      /* 0x310 */
    volatile uint32_t RESERVED4[7];
    volatile uint32_t POFCON;             /* 0x330 */
    volatile uint32_t RESERVED5[1];
    volatile uint32_t POFCON_FIXED;       /* 0x338 */
} NRF_POWER_TypeDef;

#define POWER ((NRF_POWER_TypeDef *)POWER_BASE)

/* -------------------------------------------------------------------------
 * TIMER — 32-bit general purpose timer
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_START;        /* 0x00 */
    volatile uint32_t TASKS_STOP;         /* 0x04 */
    volatile uint32_t TASKS_COUNT;        /* 0x08 */
    volatile uint32_t TASKS_CLEAR;        /* 0x0C */
    volatile uint32_t TASKS_SHUTDOWN;     /* 0x10 */
    volatile uint32_t RESERVED0[11];
    volatile uint32_t EVENTS_COMPARE[6];  /* 0x40 */
    volatile uint32_t RESERVED1[42];
    volatile uint32_t SHORTS;             /* 0x200 */
    volatile uint32_t RESERVED2[1];
    volatile uint32_t INTENSET;           /* 0x304 */
    volatile uint32_t INTENCLR;           /* 0x308 */
    volatile uint32_t RESERVED3[1];
    volatile uint32_t MODE;               /* 0x318 */
    volatile uint32_t BITMODE;            /* 0x31C */
    volatile uint32_t PRESCALER;          /* 0x320 */
    volatile uint32_t CC[6];              /* 0x324 */
} NRF_TIMER_TypeDef;

#define TIMER0 ((NRF_TIMER_TypeDef *)TIMER0_BASE)
#define TIMER1 ((NRF_TIMER_TypeDef *)TIMER1_BASE)
#define TIMER2 ((NRF_TIMER_TypeDef *)TIMER2_BASE)

#define TIMER_MODE_TIMER       (0U << 0)
#define TIMER_MODE_COUNTER     (1U << 0)
#define TIMER_BITMODE_16BIT    (0U << 0)
#define TIMER_BITMODE_8BIT     (1U << 0)
#define TIMER_BITMODE_24BIT    (2U << 0)
#define TIMER_BITMODE_32BIT    (3U << 0)
#define TIMER_PRESCALER_DIV1   0
#define TIMER_INTENSET_CC0     (1U << 16)

/* -------------------------------------------------------------------------
 * SPIM — SPI master with easyDMA
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_START;        /* 0x00 */
    volatile uint32_t TASKS_STOP;         /* 0x04 */
    volatile uint32_t TASKS_SUSPEND;      /* 0x08 */
    volatile uint32_t TASKS_RESUME;       /* 0x0C */
    volatile uint32_t RESERVED0[12];
    volatile uint32_t EVENTS_END;         /* 0x44 */
    volatile uint32_t EVENTS_ENDRX;       /* 0x48 */
    volatile uint32_t RESERVED1[2];
    volatile uint32_t EVENTS_STARTED;     /* 0x54 */
    volatile uint32_t RESERVED2[44];
    volatile uint32_t SHORTS;             /* 0x200 */
    volatile uint32_t RESERVED3[3];
    volatile uint32_t INTENSET;           /* 0x304 */
    volatile uint32_t INTENCLR;           /* 0x308 */
    volatile uint32_t RESERVED4[3];
    volatile uint32_t STALLSTAT;          /* 0x318 */
    volatile uint32_t RESERVED5[3];
    volatile uint32_t ENABLE;             /* 0x330 */
    volatile uint32_t CONFIG;             /* 0x334 — CPOL, CPHA, ORDER */
    volatile uint32_t RESERVED6[1];
    volatile uint32_t PSEL_SCK;           /* 0x33C */
    volatile uint32_t PSEL_MOSI;          /* 0x340 */
    volatile uint32_t PSEL_MISO;          /* 0x344 */
    volatile uint32_t PSEL_CS;            /* 0x348 — unused, we do CS via GPIO */
    volatile uint32_t RESERVED7[7];
    volatile uint32_t FREQUENCY;          /* 0x368 */
    volatile uint32_t RESERVED8[5];
    volatile uint32_t RXD_PTR;            /* 0x380 */
    volatile uint32_t RXD_MAXCNT;         /* 0x384 */
    volatile uint32_t RXD_AMOUNT;         /* 0x388 */
    volatile uint32_t RXD_LIST;           /* 0x38C */
    volatile uint32_t TXD_PTR;            /* 0x390 */
    volatile uint32_t TXD_MAXCNT;         /* 0x394 */
    volatile uint32_t TXD_AMOUNT;         /* 0x398 */
    volatile uint32_t TXD_LIST;           /* 0x39C */
    volatile uint32_t RESERVED9[8];
    volatile uint32_t ORC;                /* 0x3C0 — over-read char */
} NRF_SPIM_TypeDef;

#define SPIM0 ((NRF_SPIM_TypeDef *)SPIM0_BASE)
#define SPIM1 ((NRF_SPIM_TypeDef *)SPIM1_BASE)

#define SPIM_ENABLE_ENABLE     (1U << 0)
#define SPIM_CONFIG_ORDER_MSB  (0U << 0)
#define SPIM_CONFIG_CPHA_LEAD  (0U << 0)
#define SPIM_CONFIG_CPOL_LOW   (0U << 0)
#define SPIM_FREQ_8M           (0x08000000U)
#define SPIM_FREQ_4M           (0x04000000U)
#define SPIM_FREQ_2M           (0x02000000U)
#define SPIM_INTENSET_END      (1U << 6)

/* -------------------------------------------------------------------------
 * TWIM — I²C master with easyDMA
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_STARTTX;      /* 0x00 */
    volatile uint32_t TASKS_STARTRX;      /* 0x08 */
    volatile uint32_t TASKS_STOP;         /* 0x0C */
    volatile uint32_t RESERVED0[13];
    volatile uint32_t EVENTS_STOPPED;     /* 0x104 */
    volatile uint32_t EVENTS_ERROR;       /* 0x124 */
    volatile uint32_t EVENTS_RXSTARTED;   /* 0x12C */
    volatile uint32_t EVENTS_TXSTARTED;   /* 0x130 */
    volatile uint32_t RESERVED1[37];
    volatile uint32_t SHORTS;             /* 0x200 */
    volatile uint32_t RESERVED2[3];
    volatile uint32_t INTENSET;           /* 0x304 */
    volatile uint32_t INTENCLR;           /* 0x308 */
    volatile uint32_t RESERVED3[3];
    volatile uint32_t ERRORSRC;           /* 0x31C */
    volatile uint32_t RESERVED4[5];
    volatile uint32_t ENABLE;             /* 0x340 */
    volatile uint32_t RESERVED5[1];
    volatile uint32_t PSEL_SCL;           /* 0x348 */
    volatile uint32_t PSEL_SDA;           /* 0x34C */
    volatile uint32_t RESERVED6[7];
    volatile uint32_t FREQUENCY;          /* 0x374 */
    volatile uint32_t RESERVED7[3];
    volatile uint32_t RXD_PTR;            /* 0x388 */
    volatile uint32_t RXD_MAXCNT;         /* 0x38C */
    volatile uint32_t RXD_AMOUNT;         /* 0x390 */
    volatile uint32_t TXD_PTR;            /* 0x394 */
    volatile uint32_t TXD_MAXCNT;         /* 0x398 */
    volatile uint32_t TXD_AMOUNT;         /* 0x39C */
    volatile uint32_t ADDRESS;            /* 0x3A0 */
} NRF_TWIM_TypeDef;

#define TWIM0 ((NRF_TWIM_TypeDef *)TWIM0_BASE)
#define TWIM1 ((NRF_TWIM_TypeDef *)TWIM1_BASE)

#define TWIM_ENABLE_ENABLE     (6U << 0)  /* TWIM enable value */
#define TWIM_FREQ_400K         (0x01980000U)
#define TWIM_FREQ_100K         (0x01980000U >> 2)

/* -------------------------------------------------------------------------
 * SAADC — successive approximation ADC (used for battery + temp)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_START;        /* 0x00 */
    volatile uint32_t TASKS_SAMPLE;       /* 0x04 */
    volatile uint32_t TASKS_STOP;         /* 0x08 */
    volatile uint32_t TASKS_CALIBRATE;    /* 0x0C */
    volatile uint32_t RESERVED0[60];
    volatile uint32_t EVENTS_STARTED;     /* 0x100 */
    volatile uint32_t EVENTS_END;         /* 0x104 */
    volatile uint32_t EVENTS_DONE;        /* 0x108 */
    volatile uint32_t EVENTS_ENDRX;       /* 0x10C */
    volatile uint32_t RESERVED1[59];
    volatile uint32_t INTENSET;           /* 0x304 */
    volatile uint32_t INTENCLR;           /* 0x308 */
    volatile uint32_t RESERVED2[14];
    volatile uint32_t STATUS;             /* 0x344 */
    volatile uint32_t ENABLE;             /* 0x350 */
    volatile uint32_t RESERVED3[1];
    volatile uint32_t CH[8];              /* 0x358 — channel config (4 words each) */
    volatile uint32_t RESOLUTION;         /* 0x3F8 */
    volatile uint32_t OVERSAMPLE;         /* 0x3FC */
    volatile uint32_t SAMPLERATE;         /* 0x400 */
    volatile uint32_t RXD_PTR;            /* 0x410 */
    volatile uint32_t RXD_MAXCNT;         /* 0x414 */
    volatile uint32_t RXD_AMOUNT;         /* 0x418 */
} NRF_SAADC_TypeDef;

#define SAADC ((NRF_SAADC_TypeDef *)SAADC_BASE)
#define SAADC_ENABLE_ENABLE   (1U << 0)

/* -------------------------------------------------------------------------
 * GPIOTE — GPIO tasks and events
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_OUT[8];       /* 0x00 */
    volatile uint32_t RESERVED0[4];
    volatile uint32_t TASKS_SET[8];       /* 0x30 */
    volatile uint32_t RESERVED1[4];
    volatile uint32_t TASKS_CLR[8];       /* 0x60 */
    volatile uint32_t RESERVED2[32];
    volatile uint32_t EVENTS_IN[8];       /* 0x100 */
    volatile uint32_t RESERVED3[23];
    volatile uint32_t EVENTS_PORT;        /* 0x188 */
    volatile uint32_t RESERVED4[97];
    volatile uint32_t INTENSET;           /* 0x304 */
    volatile uint32_t INTENCLR;           /* 0x308 */
    volatile uint32_t RESERVED5[33];
    volatile uint32_t CONFIG[8];          /* 0x390 */
} NRF_GPIOTE_TypeDef;

#define GPIOTE ((NRF_GPIOTE_TypeDef *)GPIOTE_BASE)

/* -------------------------------------------------------------------------
 * NVMC — flash controller
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t READY;              /* 0x00 */
    volatile uint32_t READY_NEXT;         /* 0x04 */
    volatile uint32_t RESERVED0[62];
    volatile uint32_t CONFIG;             /* 0x100 */
    volatile uint32_t RESERVED1[1];
    volatile uint32_t ERASEPAGE;          /* 0x108 */
    volatile uint32_t RESERVED2[1];
    volatile uint32_t ERASEALL;           /* 0x10C */
    volatile uint32_t RESERVED3[1];
    volatile uint32_t ERASEUICR;          /* 0x110 */
    volatile uint32_t RESERVED4[9];
    volatile uint32_t WRITEUICRNS;        /* 0x138 */
} NRF_NVMC_TypeDef;

#define NVMC ((NRF_NVMC_TypeDef *)NVMC_BASE)

#define NVMC_CONFIG_REN      (0U << 0)   /* read only */
#define NVMC_CONFIG_WEN      (1U << 0)   /* write enable */
#define NVMC_CONFIG_EEN      (2U << 0)   /* erase enable */

/* -------------------------------------------------------------------------
 * IPC — inter-processor communication (app ↔ net core)
 * ------------------------------------------------------------------------- */
typedef struct {
    volatile uint32_t TASKS_SEND[16];     /* 0x000 */
    volatile uint32_t RESERVED0[15];
    volatile uint32_t EVENTS_RECEIVE[16]; /* 0x100 */
    volatile uint32_t RESERVED1[64];
    volatile uint32_t INTENSET;           /* 0x380 */
    volatile uint32_t INTENCLR;           /* 0x384 */
    volatile uint32_t RESERVED2[2];
    volatile uint32_t CONFIG[16];         /* 0x390 */
} NRF_IPC_TypeDef;

#define IPC ((NRF_IPC_TypeDef *)IPC_BASE)

/* -------------------------------------------------------------------------
 * NVIC / SCB / SysTick (Cortex-M33 system registers)
 * ------------------------------------------------------------------------- */
#define NVIC_BASE            (0xE000E100U)
#define SCB_BASE             (0xE000ED00U)
#define SYSTICK_BASE         (0xE000E010U)

#define NVIC_ISER0   (*(volatile uint32_t *)(NVIC_BASE + 0x000))
#define NVIC_ICER0   (*(volatile uint32_t *)(NVIC_BASE + 0x080))
#define NVIC_ISPR0   (*(volatile uint32_t *)(NVIC_BASE + 0x100))
#define NVIC_ICPR0   (*(volatile uint32_t *)(NVIC_BASE + 0x180))
#define NVIC_IP_BASE (NVIC_BASE + 0x300)

#define SCB_SCR      (*(volatile uint32_t *)(SCB_BASE + 0x10))
#define SCB_SCR_SLEEPDEEP  (1U << 2)
#define SCB_SCR_SEVONPEND  (1U << 4)
#define SCB_AIRCR    (*(volatile uint32_t *)(SCB_BASE + 0x0C))
#define SCB_VTOR     (*(volatile uint32_t *)(SCB_BASE + 0x08))

typedef struct {
    volatile uint32_t CTRL;     /* 0x00 */
    volatile uint32_t LOAD;     /* 0x04 */
    volatile uint32_t VAL;      /* 0x08 */
    volatile uint32_t CALIB;    /* 0x0C */
} SysTick_TypeDef;

#define SysTick ((SysTick_TypeDef *)SYSTICK_BASE)
#define SysTick_CTRL_ENABLE   (1U << 0)
#define SysTick_CTRL_TICKINT  (1U << 1)
#define SysTick_CTRL_CLKSOURCE (1U << 2)

/* -------------------------------------------------------------------------
 * IRQ numbers (nRF5340 application core)
 * ------------------------------------------------------------------------- */
#define IRQ_TIMER0         8
#define IRQ_TIMER1         9
#define IRQ_TIMER2         10
#define IRQ_RTC0           16
#define IRQ_RTC1           17
#define IRQ_SAADC          20
#define IRQ_SPIM0          21
#define IRQ_SPIM1          22
#define IRQ_SPIM2          23
#define IRQ_TWIM0          24
#define IRQ_GPIOTE0        28
#define IRQ_IPC            42
#define IRQ_USBD           56

/* -------------------------------------------------------------------------
 * Memory map (application core)
 * ------------------------------------------------------------------------- */
#define FLASH_START        0x00000000U
#define FLASH_SIZE         (1024 * 1024)   /* 1 MB app core flash */
#define RAM_START          0x20000000U
#define RAM_SIZE           (512 * 1024)    /* 512 KB app core RAM */
#define UICR_CUSTOMER_BASE (UICR_BASE + 0x80)

#endif /* SYNTHAND_REGISTERS_H */