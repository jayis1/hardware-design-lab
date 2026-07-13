/*
 * startup_stm32h733.s — CMSIS startup file and vector table for HivePulse
 *
 * Target MCU: STM32H733VGT6 (Cortex-M7 @ 280 MHz)
 * Author: jayis1
 * Copyright (c) 2026 jayis1. All rights reserved.
 * License: GPL-3.0
 */

    .syntax unified
    .cpu cortex-m7
    .fpu softvfp
    .thumb

/* Stack and heap configuration */
.global g_pfnVectors
.global Default_Handler

.word _estack           /* Initial stack pointer (defined in linker script) */
.word _sidata           /* Start of .data init values in flash */
.word _sdata            /* Start of .data in RAM */
.word _edata            /* End of .data in RAM */
.word _sbss             /* Start of .bss in RAM */
.word _ebss             /* End of .bss in RAM */

/* ---- Reset Handler ---- */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Set stack pointer */
    ldr r0, =_estack
    mov sp, r0

    /* Copy .data section from flash to RAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata
copy_data:
    cmp r1, r2
    bcs copy_data_done
    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data
copy_data_done:

    /* Zero .bss section */
    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0
zero_bss:
    cmp r0, r1
    bcs zero_bss_done
    str r2, [r0], #4
    b zero_bss
zero_bss_done:

    /* Enable FPU (CP10, CP11) */
    ldr r0, =0xE000ED88
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]
    dsb
    isb

    /* Configure vector table address */
    ldr r0, =g_pfnVectors
    ldr r1, =0xE000ED08  /* SCB->VTOR */
    str r0, [r1]
    dsb

    /* Call board_init (C function) */
    bl board_init

    /* Call main (C function) */
    bl main

    /* If main returns, loop forever */
halt_loop:
    b halt_loop

    .size Reset_Handler, .-Reset_Handler

/* ---- Default Handler (weak alias for all unimplemented IRQs) ---- */
    .section .text.Default_Handler
    .weak Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b Default_Handler
    .size Default_Handler, .-Default_Handler

/* ---- Vector Table ---- */
    .section .isr_vector
    .type g_pfnVectors, %object
g_pfnVectors:
    .word _estack                    /* 0: Initial stack pointer */
    .word Reset_Handler              /* 1: Reset */
    .word NMI_Handler                /* 2: NMI */
    .word HardFault_Handler          /* 3: HardFault */
    .word MemManage_Handler          /* 4: MemManage */
    .word BusFault_Handler           /* 5: BusFault */
    .word UsageFault_Handler         /* 6: UsageFault */
    .word 0                          /* 7: Reserved */
    .word 0                          /* 8: Reserved */
    .word 0                          /* 9: Reserved */
    .word 0                          /* 10: Reserved */
    .word SVC_Handler                /* 11: SVC */
    .word DebugMon_Handler           /* 12: DebugMon */
    .word 0                          /* 13: Reserved */
    .word PendSV_Handler             /* 14: PendSV */
    .word SysTick_Handler            /* 15: SysTick */

    /* External interrupts (STM32H733) */
    .word WWDG_IRQHandler            /* 16: Window watchdog */
    .word PVD_IRQHandler             /* 17: PVD */
    .word TAMPER_IRQHandler          /* 18: Tamper */
    .word RTC_WKUP_IRQHandler        /* 19: RTC wakeup */
    .word FLASH_IRQHandler           /* 20: Flash */
    .word RCC_IRQHandler             /* 21: RCC */
    .word EXTI0_IRQHandler           /* 22: EXTI0 */
    .word EXTI1_IRQHandler           /* 23: EXTI1 */
    .word EXTI2_IRQHandler           /* 24: EXTI2 (shared: LoRa DIO1 + Bee ch2) */
    .word EXTI3_IRQHandler           /* 25: EXTI3 */
    .word EXTI4_IRQHandler           /* 26: EXTI4 */
    .word DMA1_Stream0_IRQHandler    /* 27: DMA1 Stream0 (LoRa SPI RX) */
    .word DMA1_Stream1_IRQHandler    /* 28: DMA1 Stream1 (LoRa SPI TX) */
    .word DMA1_Stream2_IRQHandler    /* 29: DMA1 Stream2 (BLE UART RX) */
    .word DMA1_Stream3_IRQHandler    /* 30: DMA1 Stream3 (Audio I2S) */
    .word DMA1_Stream4_IRQHandler    /* 31: DMA1 Stream4 */
    .word DMA1_Stream5_IRQHandler    /* 32: DMA1 Stream5 */
    .word DMA1_Stream6_IRQHandler    /* 33: DMA1 Stream6 */
    .word ADC_IRQHandler             /* 34: ADC1/2 */
    .word FDCAN1_IT0_IRQHandler      /* 35: FDCAN1 IT0 */
    .word FDCAN2_IT0_IRQHandler      /* 36: FDCAN2 IT0 */
    .word FDCAN1_IT1_IRQHandler      /* 37: FDCAN1 IT1 */
    .word FDCAN2_IT1_IRQHandler      /* 38: FDCAN2 IT1 */
    .word EXTI9_5_IRQHandler         /* 39: EXTI 9-5 (Bee counter + BLE RDY) */
    .word TIM1_BRK_IRQHandler        /* 40: TIM1 break */
    .word TIM1_UP_IRQHandler         /* 41: TIM1 update */
    .word TIM1_TRG_COM_IRQHandler    /* 42: TIM1 trigger */
    .word TIM1_CC_IRQHandler         /* 43: TIM1 capture/compare */
    .word TIM2_IRQHandler            /* 44: TIM2 */
    .word TIM3_IRQHandler            /* 45: TIM3 */
    .word TIM4_IRQHandler            /* 46: TIM4 */
    .word I2C1_EV_IRQHandler         /* 47: I2C1 event */
    .word I2C1_ER_IRQHandler         /* 48: I2C1 error */
    .word I2C2_EV_IRQHandler         /* 49: I2C2 event */
    .word I2C2_ER_IRQHandler         /* 50: I2C2 error */
    .word SPI1_IRQHandler            /* 51: SPI1 (LoRa) */
    .word SPI2_IRQHandler            /* 52: SPI2 (Audio I2S) */
    .word USART1_IRQHandler          /* 53: USART1 */
    .word USART2_IRQHandler          /* 54: USART2 */
    .word USART3_IRQHandler          /* 55: USART3 (1-Wire) */
    .word EXTI15_10_IRQHandler       /* 56: EXTI 15-10 (Bee counter upper) */
    .word RTC_Alarm_IRQHandler       /* 57: RTC alarm */
    .word 0                          /* 58: Reserved */
    .word TIM8_BRK_IRQHandler        /* 59: TIM8 break */
    .word TIM8_UP_IRQHandler         /* 60: TIM8 update */
    .word TIM8_TRG_COM_IRQHandler    /* 61: TIM8 trigger */
    .word TIM8_CC_IRQHandler         /* 62: TIM8 capture/compare */
    .word DMA1_Stream7_IRQHandler    /* 63: DMA1 Stream7 */
    .word FMC_IRQHandler             /* 64: FMC */
    .word SDMMC1_IRQHandler          /* 65: SDMMC1 */
    .word TIM5_IRQHandler            /* 66: TIM5 */
    .word SPI3_IRQHandler            /* 67: SPI3 (IMU) */
    .word UART4_IRQHandler           /* 68: UART4 (BLE) */
    .word UART5_IRQHandler           /* 69: UART5 */
    .word TIM6_DAC_IRQHandler        /* 70: TIM6 / DAC */
    .word TIM7_IRQHandler            /* 71: TIM7 */
    .word DMA2_Stream0_IRQHandler    /* 72: DMA2 Stream0 */
    .word DMA2_Stream1_IRQHandler    /* 73: DMA2 Stream1 */
    .word DMA2_Stream2_IRQHandler    /* 74: DMA2 Stream2 */
    .word DMA2_Stream3_IRQHandler    /* 75: DMA2 Stream3 */
    .word DMA2_Stream4_IRQHandler    /* 76: DMA2 Stream4 */
    .word ETH_IRQHandler             /* 77: Ethernet */
    .word ETH_WKUP_IRQHandler        /* 78: Ethernet wakeup */
    .word FDCAN_CAL_IRQHandler       /* 79: FDCAN calibration */
    .word 0                          /* 80: Reserved */
    .word 0                          /* 81: Reserved */
    .word 0                          /* 82: Reserved */
    .word 0                          /* 83: Reserved */
    .word DMA2_Stream5_IRQHandler    /* 84: DMA2 Stream5 */
    .word DMA2_Stream6_IRQHandler    /* 85: DMA2 Stream6 */
    .word DMA2_Stream7_IRQHandler    /* 86: DMA2 Stream7 */
    .word USART6_IRQHandler          /* 87: USART6 */
    .word I2C3_EV_IRQHandler         /* 88: I2C3 event */
    .word I2C3_ER_IRQHandler         /* 89: I2C3 error */
    .word OTG_HS_EP1_OUT_IRQHandler  /* 90: USB OTG HS EP1 out */
    .word OTG_HS_EP1_IN_IRQHandler   /* 91: USB OTG HS EP1 in */
    .word OTG_HS_WKUP_IRQHandler     /* 92: USB OTG HS wakeup */
    .word OTG_HS_IRQHandler          /* 93: USB OTG HS */
    .word DCMI_IRQHandler            /* 94: DCMI */
    .word 0                          /* 95: Reserved */
    .word RNG_IRQHandler             /* 96: RNG */
    .word FPU_IRQHandler             /* 97: FPU */
    .word UART7_IRQHandler           /* 98: UART7 */
    .word UART8_IRQHandler           /* 99: UART8 */
    .word SPI4_IRQHandler            /* 100: SPI4 */
    .word SPI5_IRQHandler            /* 101: SPI5 */
    .word SPI6_IRQHandler            /* 102: SPI6 */
    .word SAI1_IRQHandler            /* 103: SAI1 */
    .word LTDC_IRQHandler            /* 104: LTDC */
    .word LTDC_ER_IRQHandler         /* 105: LTDC error */
    .word DMA2D_IRQHandler           /* 106: DMA2D */
    .word SAI2_IRQHandler            /* 107: SAI2 */
    .word QUADSPI_IRQHandler         /* 108: QSPI */
    .word LPTIM1_IRQHandler          /* 109: LPTIM1 */
    .word CEC_IRQHandler             /* 110: CEC */
    .word I2C4_EV_IRQHandler         /* 111: I2C4 event */
    .word I2C4_ER_IRQHandler         /* 112: I2C4 error */
    .word SPDIF_RX_IRQHandler        /* 113: SPDIF RX */
    .word OTG_FS_EP1_OUT_IRQHandler  /* 114: USB OTG FS EP1 out */
    .word OTG_FS_EP1_IN_IRQHandler   /* 115: USB OTG FS EP1 in */
    .word OTG_FS_WKUP_IRQHandler     /* 116: USB OTG FS wakeup */
    .word OTG_FS_IRQHandler          /* 117: USB OTG FS */
    .word DMAMUX1_OVR_IRQHandler     /* 118: DMAMUX1 overrun */
    .word HRTIM1_Master_IRQHandler   /* 119: HRTIM1 master */
    .word HRTIM1_TIMA_IRQHandler     /* 120: HRTIM1 timer A */
    .word HRTIM1_TIMB_IRQHandler     /* 121: HRTIM1 timer B */
    .word HRTIM1_TIMC_IRQHandler     /* 122: HRTIM1 timer C */
    .word HRTIM1_TIMD_IRQHandler     /* 123: HRTIM1 timer D */
    .word HRTIM1_TIME_IRQHandler     /* 124: HRTIM1 timer E */
    .word HRTIM1_FLT_IRQHandler      /* 125: HRTIM1 fault */
    .word DFSDM1_FLT0_IRQHandler     /* 126: DFSDM1 filter 0 */
    .word DFSDM1_FLT1_IRQHandler     /* 127: DFSDM1 filter 1 */
    .word DFSDM1_FLT2_IRQHandler     /* 128: DFSDM1 filter 2 */
    .word DFSDM1_FLT3_IRQHandler     /* 129: DFSDM1 filter 3 */
    .word SAI3_IRQHandler            /* 130: SAI3 */
    .word SWPMI1_IRQHandler          /* 131: SWPMI1 */
    .word TIM15_IRQHandler           /* 132: TIM15 */
    .word TIM16_IRQHandler           /* 133: TIM16 */
    .word TIM17_IRQHandler           /* 134: TIM17 */
    .word MDIOS_WKUP_IRQHandler      /* 135: MDIOS wakeup */
    .word MDIOS_IRQHandler           /* 136: MDIOS */
    .word JPEG_IRQHandler            /* 137: JPEG */
    .word MDMA_IRQHandler            /* 138: MDMA */
    .word 0                          /* 139: Reserved */
    .word SDMMC2_IRQHandler          /* 140: SDMMC2 */
    .word HSEM1_IRQHandler           /* 141: HSEM1 */
    .word 0                          /* 142: Reserved */
    .word ADC3_IRQHandler            /* 143: ADC3 */
    .word DMAMUX2_OVR_IRQHandler     /* 144: DMAMUX2 overrun */
    .word BDMA_Channel0_IRQHandler   /* 145: BDMA channel 0 */
    .word BDMA_Channel1_IRQHandler   /* 146: BDMA channel 1 */
    .word BDMA_Channel2_IRQHandler   /* 147: BDMA channel 2 */
    .word BDMA_Channel3_IRQHandler   /* 148: BDMA channel 3 */
    .word BDMA_Channel4_IRQHandler   /* 149: BDMA channel 4 */
    .word BDMA_Channel5_IRQHandler   /* 150: BDMA channel 5 */
    .word BDMA_Channel6_IRQHandler   /* 151: BDMA channel 6 */
    .word BDMA_Channel7_IRQHandler   /* 152: BDMA channel 7 */
    .word COMP_IRQHandler            /* 153: COMP */
    .word LPTIM2_IRQHandler          /* 154: LPTIM2 */
    .word LPTIM3_IRQHandler          /* 155: LPTIM3 */
    .word LPTIM4_IRQHandler          /* 156: LPTIM4 */
    .word LPTIM5_IRQHandler          /* 157: LPTIM5 */
    .word LPUART1_IRQHandler         /* 158: LPUART1 (debug) */
    .word 0                          /* 159: Reserved */
    .word CRS_IRQHandler             /* 160: CRS */
    .word ECC_IRQHandler             /* 161: ECC */
    .word SAI4_IRQHandler            /* 162: SAI4 */
    .word DTS_IRQHandler             /* 163: DTS */
    .word 0                          /* 164: Reserved */
    .word WAKEUP_PIN_IRQHandler      /* 165: Wakeup pin */
    .word OCTOSPI1_IRQHandler        /* 166: OCTOSPI1 */
    .word OTFDEC1_IRQHandler         /* 167: OTFDEC1 */
    .word OTFDEC2_IRQHandler         /* 168: OTFDEC2 */
    .word FMAC_IRQHandler            /* 169: FMAC */
    .word CORDIC_IRQHandler          /* 170: CORDIC */
    .word UART9_IRQHandler           /* 171: UART9 */
    .word USART10_IRQHandler         /* 172: USART10 */
    .word I2C5_EV_IRQHandler         /* 173: I2C5 event */
    .word I2C5_ER_IRQHandler         /* 174: I2C5 error */
    .word FDCAN3_IT0_IRQHandler      /* 175: FDCAN3 IT0 */
    .word FDCAN3_IT1_IRQHandler      /* 176: FDCAN3 IT1 */
    .word TIM23_IRQHandler           /* 177: TIM23 */
    .word TIM24_IRQHandler           /* 178: TIM24 */

    .size g_pfnVectors, .-g_pfnVectors

/* ---- Weak aliases for standard exception handlers ---- */
    .weak NMI_Handler
    .thumb_set NMI_Handler, Default_Handler
    .weak HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler
    .weak MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler
    .weak BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler
    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler
    .weak SVC_Handler
    .thumb_set SVC_Handler, Default_Handler
    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler
    .weak PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler

/* ---- DMA1 Stream3 handler -> audio DMA (shared with SPI2) ---- */
    .weak DMA1_Stream3_IRQHandler
    .thumb_set DMA1_Stream3_IRQHandler, audio_dma_irq_handler

/* ---- UART4 handler -> BLE UART ---- */
    .weak UART4_IRQHandler
    .thumb_set UART4_IRQHandler, UART4_IRQHandler