// ============================================================================
// clock_config.c — STM32G474 Clock Tree Setup
// ============================================================================

#include "registers.h"
#include "board.h"

void SystemClock_Config(void) {
    // 1. Enable HSE (8 MHz external crystal)
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // 2. Configure PLL1: HSE 8 MHz → SYSCLK 170 MHz
    //    M=2, N=85, R=2: VCO = 8/2*85 = 340 MHz, SYSCLK = 340/2 = 170 MHz
    RCC->PLLCFGR = (2 << RCC_PLLCFGR_PLLM_Pos)    // PLLM = 2
                 | (85 << RCC_PLLCFGR_PLLN_Pos)    // PLLN = 85
                 | (0 << RCC_PLLCFGR_PLLR_Pos)      // PLLR = 2 (0=div2)
                 | RCC_PLLCFGR_PLLREN              // Enable PLLR output
                 | (2 << RCC_PLLCFGR_PLLSRC_Pos);  // HSE source

    // 3. Enable PLL1
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY));

    // 4. Set flash latency for 170 MHz (4 wait states)
    FLASH_ACR = FLASH_ACR_LATENCY_4WS | FLASH_ACR_PRFTEN | FLASH_ACR_ICEN | FLASH_ACR_DCEN;
    while ((FLASH_ACR & 0x07) != FLASH_ACR_LATENCY_4WS);

    // 5. Switch SYSCLK to PLL1R, AHB=1, APB1=1, APB2=1
    RCC->CFGR = RCC_CFGR_SW_PLL1 | (0 << RCC_CFGR_HPRE_Pos) |
                (0 << RCC_CFGR_PPRE1_Pos) | (0 << RCC_CFGR_PPRE2_Pos);
    while ((RCC->CFGR & RCC_CFGR_SWS_PLL1) != RCC_CFGR_SWS_PLL1);

    // 6. Enable HSI48 for USB (48 MHz)
    RCC->CRRCR |= RCC_CRRCR_HSI48ON;
    while (!(RCC->CRRCR & RCC_CRRCR_HSI48RDY));
}

void Peripheral_Clock_Enable(void) {
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN | RCC_APB2ENR_TIM1EN | RCC_APB2ENR_SYSCFGEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN | RCC_APB1ENR_I2C1EN;
    RCC->AHB2ENR |= RCC_AHB2ENR_ADC12EN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
}