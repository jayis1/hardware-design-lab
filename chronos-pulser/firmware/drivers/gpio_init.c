// gpio_init.c — GPIO Initialization Implementation
// Configures all FPGA GPIO pins for Chronos Pulser peripherals
// Sets directions, pull-ups/downs, initial values, and interrupt configs

#include "gpio_init.h"
#include "../registers.h"
#include "../board.h"

static volatile uint32_t *gpio_base = (volatile uint32_t *)GPIO_BASE;
static bool initialized = false;

// Complete GPIO configuration table for all 17 pins
static const gpio_pin_config_t gpio_config[GPIO_PIN_COUNT] = {
    [GPIO_PIN_PULSE_TRIG] = {
        .pin = GPIO_PIN_PULSE_TRIG, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "PULSE_TRIG"
    },
    [GPIO_PIN_SRD_BIAS_DAC] = {
        .pin = GPIO_PIN_SRD_BIAS_DAC, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "SRD_BIAS_DAC"
    },
    [GPIO_PIN_LED_DATA] = {
        .pin = GPIO_PIN_LED_DATA, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "LED_DATA"
    },
    [GPIO_PIN_SYNC_OUT] = {
        .pin = GPIO_PIN_SYNC_OUT, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "SYNC_OUT"
    },
    [GPIO_PIN_SYNC_IN] = {
        .pin = GPIO_PIN_SYNC_IN, .direction = GPIO_DIR_INPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_DOWN,
        .intr_config = GPIO_INT_EDGE_RISING, .name = "SYNC_IN"
    },
    [GPIO_PIN_ADC_PDWN] = {
        .pin = GPIO_PIN_ADC_PDWN, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "ADC_PDWN"
    },
    [GPIO_PIN_VGA_PDWN] = {
        .pin = GPIO_PIN_VGA_PDWN, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "VGA_PDWN"
    },
    [GPIO_PIN_FT_RESET_N] = {
        .pin = GPIO_PIN_FT_RESET_N, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 1, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "FT_RESET_N"
    },
    [GPIO_PIN_SPI_FLASH_CS] = {
        .pin = GPIO_PIN_SPI_FLASH_CS, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 1, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "SPI_FLASH_CS"
    },
    [GPIO_PIN_VGA_SPI_CS] = {
        .pin = GPIO_PIN_VGA_SPI_CS, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 1, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "VGA_SPI_CS"
    },
    [GPIO_PIN_ADC_SPI_CS] = {
        .pin = GPIO_PIN_ADC_SPI_CS, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 1, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "ADC_SPI_CS"
    },
    [GPIO_PIN_STATUS_LED_R] = {
        .pin = GPIO_PIN_STATUS_LED_R, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "STATUS_LED_R"
    },
    [GPIO_PIN_STATUS_LED_G] = {
        .pin = GPIO_PIN_STATUS_LED_G, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "STATUS_LED_G"
    },
    [GPIO_PIN_STATUS_LED_B] = {
        .pin = GPIO_PIN_STATUS_LED_B, .direction = GPIO_DIR_OUTPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_NONE,
        .intr_config = GPIO_INT_NONE, .name = "STATUS_LED_B"
    },
    [GPIO_PIN_CFG_PROGRAMN] = {
        .pin = GPIO_PIN_CFG_PROGRAMN, .direction = GPIO_DIR_INPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_UP,
        .intr_config = GPIO_INT_NONE, .name = "CFG_PROGRAMN"
    },
    [GPIO_PIN_CFG_INITN] = {
        .pin = GPIO_PIN_CFG_INITN, .direction = GPIO_DIR_INPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_UP,
        .intr_config = GPIO_INT_NONE, .name = "CFG_INITN"
    },
    [GPIO_PIN_CFG_DONE] = {
        .pin = GPIO_PIN_CFG_DONE, .direction = GPIO_DIR_INPUT,
        .initial_val = 0, .pull_config = GPIO_PULL_UP,
        .intr_config = GPIO_INT_NONE, .name = "CFG_DONE"
    },
};

int gpio_init(void) {
    if (initialized) return 0;

    uint32_t oe_mask = 0;
    uint32_t out_mask = 0;
    uint32_t pull_en_mask = 0;
    uint32_t pull_sel_mask = 0;
    uint32_t intr_en_mask = 0;
    uint32_t intr_type_mask = 0;
    uint32_t intr_pol_mask = 0;

    for (int i = 0; i < GPIO_PIN_COUNT; i++) {
        const gpio_pin_config_t *cfg = &gpio_config[i];
        uint32_t pin_mask = (1u << cfg->pin);

        if (cfg->direction == GPIO_DIR_OUTPUT) {
            oe_mask |= pin_mask;
            if (cfg->initial_val) out_mask |= pin_mask;
        }

        if (cfg->pull_config == GPIO_PULL_UP) {
            pull_en_mask |= pin_mask;
            pull_sel_mask |= pin_mask;
        } else if (cfg->pull_config == GPIO_PULL_DOWN) {
            pull_en_mask |= pin_mask;
        }

        if (cfg->intr_config != GPIO_INT_NONE) {
            intr_en_mask |= pin_mask;
            switch (cfg->intr_config) {
                case GPIO_INT_EDGE_FALLING:
                case GPIO_INT_EDGE_RISING:
                case GPIO_INT_EDGE_BOTH:
                    intr_type_mask |= pin_mask;  // Edge-triggered
                    break;
                default:
                    break;  // Level-triggered
            }
            switch (cfg->intr_config) {
                case GPIO_INT_LEVEL_HIGH:
                case GPIO_INT_EDGE_RISING:
                    intr_pol_mask |= pin_mask;
                    break;
                default:
                    break;
            }
        }
    }

    // Apply in correct order to prevent glitches
    REG_WRITE(GPIO_OUT_REG, out_mask);
    REG_WRITE(GPIO_PULL_SEL_REG, pull_sel_mask);
    REG_WRITE(GPIO_PULL_EN_REG, pull_en_mask);
    REG_WRITE(GPIO_INT_TYPE_REG, intr_type_mask);
    REG_WRITE(GPIO_INT_POL_REG, intr_pol_mask);
    REG_WRITE(GPIO_INT_EN_REG, intr_en_mask);
    REG_WRITE(GPIO_OE_REG, oe_mask);

    initialized = true;
    return 0;
}

int gpio_configure_pin(uint8_t pin, uint8_t direction, uint8_t pull, uint8_t intr) {
    if (pin >= GPIO_PIN_COUNT) return -1;

    uint32_t pin_mask = (1u << pin);

    // Set output value before enabling output
    if (direction == GPIO_DIR_OUTPUT) {
        REG_CLR_BITS(GPIO_OUT_REG, pin_mask);  // Default low
    }

    // Configure pull
    REG_CLR_BITS(GPIO_PULL_EN_REG, pin_mask);
    REG_CLR_BITS(GPIO_PULL_SEL_REG, pin_mask);
    if (pull == GPIO_PULL_UP) {
        REG_SET_BITS(GPIO_PULL_EN_REG, pin_mask);
        REG_SET_BITS(GPIO_PULL_SEL_REG, pin_mask);
    } else if (pull == GPIO_PULL_DOWN) {
        REG_SET_BITS(GPIO_PULL_EN_REG, pin_mask);
    }

    // Configure interrupt
    REG_CLR_BITS(GPIO_INT_EN_REG, pin_mask);
    if (intr != GPIO_INT_NONE) {
        REG_SET_BITS(GPIO_INT_EN_REG, pin_mask);
        if (intr >= GPIO_INT_EDGE_FALLING) {
            REG_SET_BITS(GPIO_INT_TYPE_REG, pin_mask);
        } else {
            REG_CLR_BITS(GPIO_INT_TYPE_REG, pin_mask);
        }
        if (intr == GPIO_INT_LEVEL_HIGH || intr == GPIO_INT_EDGE_RISING) {
            REG_SET_BITS(GPIO_INT_POL_REG, pin_mask);
        } else {
            REG_CLR_BITS(GPIO_INT_POL_REG, pin_mask);
        }
    }

    // Set direction
    if (direction == GPIO_DIR_OUTPUT) {
        REG_SET_BITS(GPIO_OE_REG, pin_mask);
    } else {
        REG_CLR_BITS(GPIO_OE_REG, pin_mask);
    }

    return 0;
}

void gpio_set(uint8_t pin, uint8_t value) {
    uint32_t mask = (1u << pin);
    if (value) {
        REG_SET_BITS(GPIO_OUT_REG, mask);
    } else {
        REG_CLR_BITS(GPIO_OUT_REG, mask);
    }
}

uint8_t gpio_get(uint8_t pin) {
    uint32_t mask = (1u << pin);
    return (REG_READ(GPIO_IN_REG) & mask) ? 1 : 0;
}

void gpio_toggle(uint8_t pin) {
    uint32_t mask = (1u << pin);
    REG_TGL_BITS(GPIO_OUT_REG, mask);
}

void gpio_write_port(uint32_t value) {
    REG_WRITE(GPIO_OUT_REG, value);
}

uint32_t gpio_read_port(void) {
    return REG_READ(GPIO_IN_REG);
}

void gpio_set_direction(uint8_t pin, uint8_t direction) {
    uint32_t mask = (1u << pin);
    if (direction == GPIO_DIR_OUTPUT) {
        REG_SET_BITS(GPIO_OE_REG, mask);
    } else {
        REG_CLR_BITS(GPIO_OE_REG, mask);
    }
}

int gpio_get_interrupt_status(uint8_t pin) {
    uint32_t mask = (1u << pin);
    return (REG_READ(GPIO_INT_STATUS_REG) & mask) ? 1 : 0;
}

void gpio_clear_interrupt(uint8_t pin) {
    uint32_t mask = (1u << pin);
    REG_WRITE(GPIO_INT_STATUS_REG, mask);  // Write 1 to clear
}

void gpio_enable_interrupt(uint8_t pin, bool enable) {
    uint32_t mask = (1u << pin);
    if (enable) {
        REG_SET_BITS(GPIO_INT_EN_REG, mask);
    } else {
        REG_CLR_BITS(GPIO_INT_EN_REG, mask);
    }
}

bool gpio_is_output(uint8_t pin) {
    uint32_t mask = (1u << pin);
    return (REG_READ(GPIO_OE_REG) & mask) ? true : false;
}

const char *gpio_get_pin_name(uint8_t pin) {
    if (pin >= GPIO_PIN_COUNT) return "UNKNOWN";
    return gpio_config[pin].name;
}
