/*
 * startup.s — STM32L432KC startup code (minimal)
 * Author: jayis1
 * Copyright (c) 2026 jayis1 — MIT License
 *
 * Minimal Cortex-M4 startup: copies .data from flash to SRAM, zeros .bss,
 * sets up the vector table, and calls main(). The linker script
 * (STM32L432KCUX_FLASH.ld) defines the memory regions.
 */

    .syntax unified
    .cpu cortex-m4
    .thumb

    .section .isr_vector, "a", %progbits
    .align 2
    .global g_pfnVectors
g_pfnVectors:
    .word _estack              /* Initial stack pointer */
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word 0                    /* MemManage */
    .word 0                    /* BusFault */
    .word 0                    /* UsageFault */
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    /* EXTI4 (IRQ 10) and EXTI9_5 (IRQ 23), USART1 (IRQ 27) slots. */
    .rept 10
    .word Default_Handler
    .endr
    .word EXTI4_IRQHandler     /* position 10 */
    .rept 12
    .word Default_Handler
    .endr
    .word EXTI9_5_IRQHandler   /* position 23 */
    .rept 3
    .word Default_Handler
    .endr
    .word USART1_IRQHandler   /* position 27 */

    .section .text.Reset_Handler, "ax", %progbits
    .align 2
    .global Reset_Handler
    .thumb_func
Reset_Handler:
    ldr   r0, =_sidata
    ldr   r1, =_sdata
    ldr   r2, =_edata
copy_data:
    cmp   r1, r2
    bcc   1f
    b     2f
1:
    ldr   r3, [r0], #4
    str   r3, [r1], #4
    b     copy_data
2:
    ldr   r0, =_sbss
    ldr   r1, =_ebss
    movs  r2, #0
zero_bss:
    cmp   r0, r1
    bcc   1f
    b     2f
1:
    str   r2, [r0], #4
    b     zero_bss
2:
    bl    SystemInit
    bl    main
hang:
    b     hang

    .weak SystemInit
    .thumb_set SystemInit, Default_Handler
    .weak NMI_Handler
    .thumb_set NMI_Handler, Default_Handler
    .weak HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler
    .weak SVC_Handler
    .thumb_set SVC_Handler, Default_Handler
    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler
    .weak PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler
    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler
    .weak EXTI4_IRQHandler
    .thumb_set EXTI4_IRQHandler, Default_Handler
    .weak EXTI9_5_IRQHandler
    .thumb_set EXTI9_5_IRQHandler, Default_Handler
    .weak USART1_IRQHandler
    .thumb_set USART1_IRQHandler, Default_Handler

    .section .text.Default_Handler, "ax", %progbits
    .align 2
    .global Default_Handler
    .thumb_func
Default_Handler:
    b     Default_Handler
    .end