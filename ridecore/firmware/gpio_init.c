// ============================================================================
// gpio_init.c — RideCore STM32G474 GPIO Configuration
// ============================================================================

#include "registers.h"
#include "board.h"

#define GPIO_MODE_INPUT    0
#define GPIO_MODE_OUTPUT   1
#define GPIO_MODE_AF       2
#define GPIO_MODE_ANALOG   3

#define GPIO_NOPULL   0
#define GPIO_PULLUP   1
#define GPIO_PULLDOWN 2

#define GPIO_SPEED_LOW    0
#define GPIO_SPEED_MED    1
#define GPIO_SPEED_HIGH   2
#define GPIO_SPEED_VHIGH  3

static void gpio_set_mode(GPIO_TypeDef *port, uint8_t pin, uint8_t mode) {
    port->MODER = (port->MODER & ~(3 << (pin*2))) | (mode << (pin*2));
}

static void gpio_set_af(GPIO_TypeDef *port, uint8_t pin, uint8_t af) {
    if (pin < 8)
        port->AFRL = (port->AFRL & ~(0xF << (pin*4))) | (af << (pin*4));
    else
        port->AFRH = (port->AFRH & ~(0xF << ((pin-8)*4))) | (af << ((pin-8)*4));
}

static void gpio_set_pull(GPIO_TypeDef *port, uint8_t pin, uint8_t pull) {
    port->PUPDR = (port->PUPDR & ~(3 << (pin*2))) | (pull << (pin*2));
}

static void gpio_set_speed(GPIO_TypeDef *port, uint8_t pin, uint8_t speed) {
    port->OSPEEDR = (port->OSPEEDR & ~(3 << (pin*2))) | (speed << (pin*2));
}

static void gpio_set_od(GPIO_TypeDef *port, uint8_t pin) {
    port->OTYPER |= (1 << pin);
}

void GPIO_Init(void) {
    // PWM outputs (TIM1)
    gpio_set_mode(GPIOA, 8, GPIO_MODE_AF);  gpio_set_af(GPIOA, 8, 6);  gpio_set_speed(GPIOA, 8, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOA, 9, GPIO_MODE_AF);  gpio_set_af(GPIOA, 9, 6);  gpio_set_speed(GPIOA, 9, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOA, 10, GPIO_MODE_AF); gpio_set_af(GPIOA, 10, 6); gpio_set_speed(GPIOA, 10, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOA, 11, GPIO_MODE_AF); gpio_set_af(GPIOA, 11, 6); gpio_set_speed(GPIOA, 11, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOB, 14, GPIO_MODE_AF); gpio_set_af(GPIOB, 14, 6); gpio_set_speed(GPIOB, 14, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOB, 15, GPIO_MODE_AF); gpio_set_af(GPIOB, 15, 6); gpio_set_speed(GPIOB, 15, GPIO_SPEED_VHIGH);

    // ADC inputs
    gpio_set_mode(GPIOC, 0, GPIO_MODE_ANALOG);
    gpio_set_mode(GPIOC, 1, GPIO_MODE_ANALOG);
    gpio_set_mode(GPIOC, 2, GPIO_MODE_ANALOG);
    gpio_set_mode(GPIOC, 3, GPIO_MODE_ANALOG);

    // SPI1
    gpio_set_mode(GPIOA, 4, GPIO_MODE_OUTPUT);  gpio_set_speed(GPIOA, 4, GPIO_SPEED_VHIGH); GPIOA->BSRR = (1 << 4);
    gpio_set_mode(GPIOA, 5, GPIO_MODE_AF);       gpio_set_af(GPIOA, 5, 5); gpio_set_speed(GPIOA, 5, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOA, 6, GPIO_MODE_AF);       gpio_set_af(GPIOA, 6, 5);
    gpio_set_mode(GPIOA, 7, GPIO_MODE_AF);       gpio_set_af(GPIOA, 7, 5); gpio_set_speed(GPIOA, 7, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOB, 2, GPIO_MODE_OUTPUT);    gpio_set_speed(GPIOB, 2, GPIO_SPEED_VHIGH); GPIOB->BSRR = (1 << 2);

    // I2C1 (open-drain)
    gpio_set_mode(GPIOB, 10, GPIO_MODE_AF); gpio_set_af(GPIOB, 10, 4); gpio_set_od(GPIOB, 10); gpio_set_speed(GPIOB, 10, GPIO_SPEED_VHIGH);
    gpio_set_mode(GPIOB, 11, GPIO_MODE_AF); gpio_set_af(GPIOB, 11, 4); gpio_set_od(GPIOB, 11); gpio_set_speed(GPIOB, 11, GPIO_SPEED_VHIGH);

    // USART2 (BLE)
    gpio_set_mode(GPIOA, 2, GPIO_MODE_AF); gpio_set_af(GPIOA, 2, 7); gpio_set_speed(GPIOA, 2, GPIO_SPEED_HIGH);
    gpio_set_mode(GPIOA, 3, GPIO_MODE_AF); gpio_set_af(GPIOA, 3, 7);

    // LEDs
    gpio_set_mode(GPIOC, 13, GPIO_MODE_OUTPUT); GPIOC->BSRR = (1 << (13+16));
    gpio_set_mode(GPIOC, 14, GPIO_MODE_OUTPUT); GPIOC->BSRR = (1 << (14+16));

    // Hall inputs
    gpio_set_mode(GPIOC, 15, GPIO_MODE_INPUT); gpio_set_pull(GPIOC, 15, GPIO_PULLUP);
    gpio_set_mode(GPIOB, 5, GPIO_MODE_INPUT);  gpio_set_pull(GPIOB, 5, GPIO_PULLUP);
    gpio_set_mode(GPIOB, 6, GPIO_MODE_INPUT);  gpio_set_pull(GPIOB, 6, GPIO_PULLUP);

    // Gate enable
    gpio_set_mode(GPIOB, 0, GPIO_MODE_OUTPUT);

    // DESAT fault
    gpio_set_mode(GPIOB, 1, GPIO_MODE_INPUT); gpio_set_pull(GPIOB, 1, GPIO_PULLUP);

    // PMIC IRQ
    gpio_set_mode(GPIOB, 7, GPIO_MODE_INPUT); gpio_set_pull(GPIOB, 7, GPIO_PULLUP);

    // BLE RST
    gpio_set_mode(GPIOB, 4, GPIO_MODE_OUTPUT); GPIOB->BSRR = (1 << 4);

    // Flash WP/HOLD
    gpio_set_mode(GPIOA, 15, GPIO_MODE_OUTPUT); GPIOA->BSRR = (1 << 15);
    gpio_set_mode(GPIOB, 3, GPIO_MODE_OUTPUT);  GPIOB->BSRR = (1 << 3);

    // CAN RST
    gpio_set_mode(GPIOB, 13, GPIO_MODE_OUTPUT); GPIOB->BSRR = (1 << 13);
}

void PWM_Output_Enable(void) {
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC1NE |
                  TIM_CCER_CC2E | TIM_CCER_CC2NE |
                  TIM_CCER_CC3E | TIM_CCER_CC3NE;
}