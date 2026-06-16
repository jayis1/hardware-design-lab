// gpio_init.h — GPIO Initialization Header
// Pin configuration and GPIO access functions for Chronos Pulser
// Maps FPGA GPIO controller pins to physical functions

#ifndef GPIO_INIT_H
#define GPIO_INIT_H

#include <stdint.h>
#include <stdbool.h>

// GPIO pin numbers (matching board.h GPIO_* definitions)
#define GPIO_PIN_PULSE_TRIG         0
#define GPIO_PIN_SRD_BIAS_DAC       1
#define GPIO_PIN_LED_DATA           2
#define GPIO_PIN_SYNC_OUT           3
#define GPIO_PIN_SYNC_IN            4
#define GPIO_PIN_ADC_PDWN           5
#define GPIO_PIN_VGA_PDWN           6
#define GPIO_PIN_FT_RESET_N         7
#define GPIO_PIN_SPI_FLASH_CS       8
#define GPIO_PIN_VGA_SPI_CS         9
#define GPIO_PIN_ADC_SPI_CS         10
#define GPIO_PIN_STATUS_LED_R       11
#define GPIO_PIN_STATUS_LED_G       12
#define GPIO_PIN_STATUS_LED_B       13
#define GPIO_PIN_CFG_PROGRAMN       14
#define GPIO_PIN_CFG_INITN          15
#define GPIO_PIN_CFG_DONE           16
#define GPIO_PIN_COUNT              17

// GPIO direction
#define GPIO_DIR_INPUT              0
#define GPIO_DIR_OUTPUT             1

// GPIO pull configuration
#define GPIO_PULL_NONE              0
#define GPIO_PULL_UP                1
#define GPIO_PULL_DOWN              2

// GPIO interrupt configuration
#define GPIO_INT_NONE               0
#define GPIO_INT_LEVEL_LOW          1
#define GPIO_INT_LEVEL_HIGH         2
#define GPIO_INT_EDGE_FALLING       3
#define GPIO_INT_EDGE_RISING        4
#define GPIO_INT_EDGE_BOTH          5

// GPIO pin configuration structure
typedef struct {
    uint8_t  pin;
    uint8_t  direction;
    uint8_t  initial_val;
    uint8_t  pull_config;
    uint8_t  intr_config;
    const char *name;
} gpio_pin_config_t;

// Function prototypes
int     gpio_init(void);
int     gpio_configure_pin(uint8_t pin, uint8_t direction, uint8_t pull, uint8_t intr);
void    gpio_set(uint8_t pin, uint8_t value);
uint8_t gpio_get(uint8_t pin);
void    gpio_toggle(uint8_t pin);
void    gpio_write_port(uint32_t value);
uint32_t gpio_read_port(void);
void    gpio_set_direction(uint8_t pin, uint8_t direction);
int     gpio_get_interrupt_status(uint8_t pin);
void    gpio_clear_interrupt(uint8_t pin);
void    gpio_enable_interrupt(uint8_t pin, bool enable);
bool    gpio_is_output(uint8_t pin);
const char *gpio_get_pin_name(uint8_t pin);

#endif // GPIO_INIT_H
