/*
 * EAG-7000 — GPIO and IOMUX Initialization for Cortex-M4F
 *
 * Configures pin multiplexing, pad settings, and GPIO directions
 * based on Phase 2 pinout table and Phase 4 register definitions.
 *
 * Copyright (c) 2026 EAG-7000 Project
 * SPDX-License-Identifier: MIT
 */

#include "gpio.h"
#include "registers.h"

/* ============================================================================
 * IOMUXC Register Address Calculation
 * Each pin has: MUX_CTRL(offset), PAD_CTRL(offset + 0x200 region)
 * ============================================================================ */

/* Simplified IOMUXC MUX register addresses for EAG-7000 pins.
 * These offsets are from IOMUXC_BASE (0x30330000).
 * Full addresses computed as IOMUXC_BASE + offset. */

/* GPIO1 pins — PCIe control */
#define IOMUXC_GPIO1_IO10       (IOMUXC_BASE + 0x0060)  /* PCIe RST */
#define IOMUXC_GPIO1_IO11       (IOMUXC_BASE + 0x0064)  /* PCIe Wake */
#define IOMUXC_GPIO1_IO12       (IOMUXC_BASE + 0x0068)  /* PCIe CLKREQ */

/* GPIO3 pins — Sensor/CAN control */
#define IOMUXC_GPIO3_IO00       (IOMUXC_BASE + 0x00B0)  /* I2C mux reset */
#define IOMUXC_GPIO3_IO01       (IOMUXC_BASE + 0x00B4)  /* Camera reset */
#define IOMUXC_GPIO3_IO02       (IOMUXC_BASE + 0x00B8)  /* Camera PWDN */
#define IOMUXC_GPIO3_IO03       (IOMUXC_BASE + 0x00BC)  /* CAN1 enable */
#define IOMUXC_GPIO3_IO04       (IOMUXC_BASE + 0x00C0)  /* CAN2 enable */
#define IOMUXC_GPIO3_IO05       (IOMUXC_BASE + 0x00C4)  /* CAN1 standby */
#define IOMUXC_GPIO3_IO06       (IOMUXC_BASE + 0x00C8)  /* CAN2 standby */

/* GPIO4 pins — Status LEDs and control */
#define IOMUXC_GPIO4_IO00       (IOMUXC_BASE + 0x00D0)  /* LED status (green) */
#define IOMUXC_GPIO4_IO01       (IOMUXC_BASE + 0x00D4)  /* LED error (red) */
#define IOMUXC_GPIO4_IO02       (IOMUXC_BASE + 0x00D8)  /* LED activity (blue) */
#define IOMUXC_GPIO4_IO03       (IOMUXC_BASE + 0x00DC)  /* PoE+ status */
#define IOMUXC_GPIO4_IO04       (IOMUXC_BASE + 0x00E0)  /* RTC IRQ */
#define IOMUXC_GPIO4_IO05       (IOMUXC_BASE + 0x00E4)  /* NPU IRQ */
#define IOMUXC_GPIO4_IO06       (IOMUXC_BASE + 0x00E8)  /* Watchdog feed */

/* ============================================================================
 * GPIO Initialization
 * ============================================================================ */

int gpio_init(void)
{
    /* Configure GPIO1 — PCIe control pins (all output) */
    /* Set MUX mode to ALT3 (GPIO mode) for PCIe control pins */
    mmio_write32(IOMUXC_GPIO1_IO10, IOMUX_ALT3);  /* PCIE_RST → GPIO1_IO10 */
    mmio_write32(IOMUXC_GPIO1_IO11, IOMUX_ALT3);  /* PCIE_WAKE → GPIO1_IO11 */
    mmio_write32(IOMUXC_GPIO1_IO12, IOMUX_ALT3);  /* PCIE_CLKREQ → GPIO1_IO12 */

    /* Configure pad settings: 260Ω drive, 100K pull-up, hysteresis */
    mmio_write32(IOMUXC_GPIO1_IO10 + 0x200,
                 PAD_CTRL_DSE_260OHM | PAD_CTRL_PUS_100K_PU | PAD_CTRL_HYS);
    mmio_write32(IOMUXC_GPIO1_IO11 + 0x200,
                 PAD_CTRL_DSE_260OHM | PAD_CTRL_PUS_100K_PU | PAD_CTRL_HYS);
    mmio_write32(IOMUXC_GPIO1_IO12 + 0x200,
                 PAD_CTRL_DSE_260OHM | PAD_CTRL_PUS_100K_PU | PAD_CTRL_HYS);

    /* Set GPIO1 direction: outputs for RST, CLKREQ; input for WAKE */
    mmio_write32(GPIO1_BASE + GPIO_GDIR, 0x00000000);  /* All inputs first */
    mmio_write32(GPIO1_BASE + GPIO_GDIR,
                 (1U << 10) |   /* GPIO1_IO10 — PCIe RST (output) */
                 (1U << 12));    /* GPIO1_IO12 — PCIe CLKREQ (output) */

    /* PCIe reset active (HIGH = inactive, per Phase 2) */
    mmio_write32(GPIO1_BASE + GPIO_DR, (1U << 10));

    /* Configure GPIO3 — Sensor/CAN control (all outputs) */
    mmio_write32(IOMUXC_GPIO3_IO00, IOMUX_ALT3);  /* I2C mux RST → GPIO3_IO00 */
    mmio_write32(IOMUXC_GPIO3_IO01, IOMUX_ALT3);  /* CAM RST → GPIO3_IO01 */
    mmio_write32(IOMUXC_GPIO3_IO02, IOMUX_ALT3);  /* CAM PWDN → GPIO3_IO02 */
    mmio_write32(IOMUXC_GPIO3_IO03, IOMUX_ALT3);  /* CAN1 EN → GPIO3_IO03 */
    mmio_write32(IOMUXC_GPIO3_IO04, IOMUX_ALT3);  /* CAN2 EN → GPIO3_IO04 */
    mmio_write32(IOMUXC_GPIO3_IO05, IOMUX_ALT3);  /* CAN1 STB → GPIO3_IO05 */
    mmio_write32(IOMUXC_GPIO3_IO06, IOMUX_ALT3);  /* CAN2 STB → GPIO3_IO06 */

    /* Pad settings: 520Ω drive for control pins */
    for (int i = 0; i < 7; i++) {
        mmio_write32(IOMUXC_GPIO3_IO00 + 0x200 + (i * 4),
                     PAD_CTRL_DSE_520OHM | PAD_CTRL_SPEED(1));
    }

    /* GPIO3 direction: all outputs */
    mmio_write32(GPIO3_BASE + GPIO_GDIR,
                 (1U << 0) |   /* GPIO3_IO00 — I2C mux reset */
                 (1U << 1) |   /* GPIO3_IO01 — Camera reset */
                 (1U << 2) |   /* GPIO3_IO02 — Camera power-down */
                 (1U << 3) |   /* GPIO3_IO03 — CAN1 enable/reset */
                 (1U << 4) |   /* GPIO3_IO04 — CAN2 enable/reset */
                 (1U << 5) |   /* GPIO3_IO05 — CAN1 standby */
                 (1U << 6));   /* GPIO3_IO06 — CAN2 standby */

    /* Initial state: CAN controllers in reset, standby, camera powered down */
    mmio_write32(GPIO3_BASE + GPIO_DR,
                 (0U << 0) |    /* I2C mux in reset (active-low) */
                 (0U << 1) |    /* Camera in reset */
                 (1U << 2) |    /* Camera power-down active */
                 (0U << 3) |    /* CAN1 reset (active-low, deasserted later) */
                 (0U << 4) |    /* CAN2 reset (active-low, deasserted later) */
                 (1U << 5) |    /* CAN1 standby active */
                 (1U << 6));    /* CAN2 standby active */

    /* Configure GPIO4 — Status LEDs and control */
    mmio_write32(IOMUXC_GPIO4_IO00, IOMUX_ALT3);  /* LED green → GPIO4_IO00 */
    mmio_write32(IOMUXC_GPIO4_IO01, IOMUX_ALT3);  /* LED red → GPIO4_IO01 */
    mmio_write32(IOMUXC_GPIO4_IO02, IOMUX_ALT3);  /* LED blue → GPIO4_IO02 */
    mmio_write32(IOMUXC_GPIO4_IO03, IOMUX_ALT3);  /* PoE+ status → GPIO4_IO03 */
    mmio_write32(IOMUXC_GPIO4_IO04, IOMUX_ALT3);  /* RTC IRQ → GPIO4_IO04 */
    mmio_write32(IOMUXC_GPIO4_IO05, IOMUX_ALT3);  /* NPU IRQ → GPIO4_IO05 */
    mmio_write32(IOMUXC_GPIO4_IO06, IOMUX_ALT3);  /* Watchdog → GPIO4_IO06 */

    /* Pad settings */
    mmio_write32(IOMUXC_GPIO4_IO00 + 0x200, PAD_CTRL_DSE_520OHM);
    mmio_write32(IOMUXC_GPIO4_IO01 + 0x200, PAD_CTRL_DSE_520OHM);
    mmio_write32(IOMUXC_GPIO4_IO02 + 0x200, PAD_CTRL_DSE_520OHM);
    mmio_write32(IOMUXC_GPIO4_IO03 + 0x200,
                 PAD_CTRL_DSE_260OHM | PAD_CTRL_HYS);
    mmio_write32(IOMUXC_GPIO4_IO04 + 0x200,
                 PAD_CTRL_DSE_260OHM | PAD_CTRL_PUS_100K_PU | PAD_CTRL_HYS);
    mmio_write32(IOMUXC_GPIO4_IO05 + 0x200,
                 PAD_CTRL_DSE_260OHM | PAD_CTRL_PUS_100K_PU | PAD_CTRL_HYS);
    mmio_write32(IOMUXC_GPIO4_IO06 + 0x200, PAD_CTRL_DSE_520OHM);

    /* GPIO4 direction: LEDs and watchdog = output, PoE/RTC/NPU = input */
    mmio_write32(GPIO4_BASE + GPIO_GDIR,
                 (1U << 0) |   /* LED green (output) */
                 (1U << 1) |   /* LED red (output) */
                 (1U << 2) |   /* LED blue (output) */
                 (1U << 6));   /* Watchdog (output) */

    /* All LEDs off initially */
    mmio_write32(GPIO4_BASE + GPIO_DR,
                 mmio_read32(GPIO4_BASE + GPIO_DR) & ~(7U << 0));

    /* Configure GPIO interrupts */
    /* NPU IRQ — falling edge (active-low) on GPIO4_IO05 */
    mmio_write32(GPIO4_BASE + GPIO_ICR2,
                 (GPIO_ICR_FALLING_EDGE << ((5 - 16) * 2)));
    mmio_write32(GPIO4_BASE + GPIO_IMR,
                 mmio_read32(GPIO4_BASE + GPIO_IMR) | (1U << 5));

    /* RTC IRQ — active-low level on GPIO4_IO04 */
    mmio_write32(GPIO4_BASE + GPIO_ICR1,
                 (GPIO_ICR_LOW_LEVEL << (4 * 2)));
    mmio_write32(GPIO4_BASE + GPIO_IMR,
                 mmio_read32(GPIO4_BASE + GPIO_IMR) | (1U << 4));

    return 0;
}

/**
 * Set a GPIO pin value.
 * @param gpio_base  GPIO port base address (GPIO1_BASE, etc.)
 * @param pin         Pin number (0-31)
 * @param value       0 or 1
 */
void gpio_set_pin(uintptr_t gpio_base, uint8_t pin, uint8_t value)
{
    if (value) {
        mmio_write32(gpio_base + GPIO_DR,
                     mmio_read32(gpio_base + GPIO_DR) | (1U << pin));
    } else {
        mmio_write32(gpio_base + GPIO_DR,
                     mmio_read32(gpio_base + GPIO_DR) & ~(1U << pin));
    }
}

/**
 * Read a GPIO pin value.
 * @param gpio_base  GPIO port base address
 * @param pin         Pin number (0-31)
 * @return 0 or 1
 */
uint8_t gpio_get_pin(uintptr_t gpio_base, uint8_t pin)
{
    return (mmio_read32(gpio_base + GPIO_PSR) >> pin) & 1;
}

/**
 * Toggle a GPIO pin.
 */
void gpio_toggle_pin(uintptr_t gpio_base, uint8_t pin)
{
    mmio_write32(gpio_base + GPIO_DR,
                 mmio_read32(gpio_base + GPIO_DR) ^ (1U << pin));
}

/**
 * Configure a GPIO pin direction.
 * @param gpio_base  GPIO port base address
 * @param pin         Pin number (0-31)
 * @param output      1 for output, 0 for input
 */
void gpio_set_dir(uintptr_t gpio_base, uint8_t pin, uint8_t output)
{
    uint32_t gdir = mmio_read32(gpio_base + GPIO_GDIR);
    if (output) {
        gdir |= (1U << pin);
    } else {
        gdir &= ~(1U << pin);
    }
    mmio_write32(gpio_base + GPIO_GDIR, gdir);
}