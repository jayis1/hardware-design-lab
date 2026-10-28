/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef PF_REGISTERS_H
#define PF_REGISTERS_H
#include <stdint.h>
/* STM32F401 RM0368 addresses; reset HSI clock, no vendor HAL. */
#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define RCC_AHB1ENR REG(0x40023830u)
#define RCC_APB1ENR REG(0x40023840u)
#define RCC_CSR REG(0x40023874u)
#define GPIOA 0x40020000u
#define GPIOB 0x40020400u
#define MODER(b) REG((b)+0x00u)
#define PUPDR(b) REG((b)+0x0cu)
#define IDR(b) REG((b)+0x10u)
#define AFRL(b) REG((b)+0x20u)
#define USART_SR REG(0x40004400u)
#define USART_DR REG(0x40004404u)
#define USART_BRR REG(0x40004408u)
#define USART_CR1 REG(0x4000440cu)
#define USART_CR2 REG(0x40004410u)
#define USART_CR3 REG(0x40004414u)
#define IWDG_KR REG(0x40003000u)
#define IWDG_PR REG(0x40003004u)
#define IWDG_RLR REG(0x40003008u)
#define IWDG_SR REG(0x4000300cu)
#define SYST_CSR REG(0xe000e010u)
#define SYST_RVR REG(0xe000e014u)
#define SYST_CVR REG(0xe000e018u)
#endif
