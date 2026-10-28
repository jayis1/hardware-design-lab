/* Author: jayis1. SPDX-License-Identifier: MIT */
#ifndef TILT_REGISTERS_H
#define TILT_REGISTERS_H
#include <stdint.h>
/* STM32F401 RM0368 Rev 6: memory map, RCC, GPIO, USART2, ADC chapter 11.
 * ADC1 base 0x40012000; common CCR at ADC1 + 0x304. */
#define REG(a) (*(volatile uint32_t *)(uintptr_t)(a))
#define RCC_CR REG(0x40023800u)
#define RCC_CFGR REG(0x40023808u)
#define RCC_AHB1ENR REG(0x40023830u)
#define RCC_APB1ENR REG(0x40023840u)
#define RCC_APB2ENR REG(0x40023844u)
#define RCC_CSR REG(0x40023874u)
#define FLASH_ACR REG(0x40023c00u)
#define GPIOA 0x40020000u
#define GPIOB 0x40020400u
#define MODER(b) REG((b)+0x00u)
#define PUPDR(b) REG((b)+0x0cu)
#define AFRL(b) REG((b)+0x20u)
#define USART_SR REG(0x40004400u)
#define USART_DR REG(0x40004404u)
#define USART_BRR REG(0x40004408u)
#define USART_CR1 REG(0x4000440cu)
#define USART_CR2 REG(0x40004410u)
#define USART_CR3 REG(0x40004414u)
#define ADC_SR REG(0x40012000u)
#define ADC_CR1 REG(0x40012004u)
#define ADC_CR2 REG(0x40012008u)
#define ADC_SMPR1 REG(0x4001200cu)
#define ADC_SMPR2 REG(0x40012010u)
#define ADC_SQR1 REG(0x4001202cu)
#define ADC_SQR2 REG(0x40012030u)
#define ADC_SQR3 REG(0x40012034u)
#define ADC_DR REG(0x4001204cu)
#define ADC_CCR REG(0x40012304u)
#define IWDG_KR REG(0x40003000u)
#define IWDG_PR REG(0x40003004u)
#define IWDG_RLR REG(0x40003008u)
#define IWDG_SR REG(0x4000300cu)
#define SYST_CSR REG(0xe000e010u)
#define SYST_RVR REG(0xe000e014u)
#define SYST_CVR REG(0xe000e018u)
#endif
