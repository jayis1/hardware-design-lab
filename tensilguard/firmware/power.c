/*
 * power.c — power management, fuel gauge, and adaptive scheduler
 * Author: jayis1
 * SPDX-License-Identifier: MIT
 *
 * Talks to the BQ25570 energy-harvesting charger (MPPT tracking of a 6-cell
 * solar panel) and the MAX17055 LiFePO4 fuel gauge over I2C1, reports battery
 * state-of-charge / voltage / current / panel voltage, and adjusts the
 * measurement interval based on available harvested energy.
 */
#include <string.h>
#include "tensilguard.h"
#include "board.h"
#include "registers.h"
#include "hal.h"

/* MAX17055 registers (subset) */
#define MAX17055_STATUS        0x00
#define MAX17055_REPCAP         0x05
#define MAX17055_REPSOC         0x06
#define MAX17055_VCELL          0x09
#define MAX17055_CURRENT        0x0A
#define MAX17055_AVG_CURRENT    0x0B
#define MAX17055_TEMP           0x08
#define MAX17055_CONFIG         0x1D
#define MAX17055_DESIGN_CAP     0x18
#define MAX17055_CHG_TER        0x1E

/* BQ25570 (only read STATUS + VBAT_SAMP via I2C for telemetry) */
#define BQ25570_ADDR            0x00    /* not standard I2C; on TI eval wired */

static float s_battery_mv = 3300.0f;
static float s_panel_mv   = 0.0f;
static float s_soc_pct     = 100.0f;
static float s_current_ma = 0.0f;
static uint32_t s_last_low_check = 0;

/* ----------------------------- Prototypes --------------------------------- */
/* i2c_read16 / i2c_write16 / adc_read_mv are declared in hal.h and defined
 * in temp.c / power.c respectively. */

/* ----------------------------- Public API --------------------------------- */

bool power_init(void)
{
    /* MAX17055: trigger a quick-start and check status */
    uint16_t status = i2c_read16(0x36, MAX17055_STATUS);
    if (status == 0xFFFF || status == 0x0000) return false;
    /* design capacity = 1500 mAh for our 18650 LiFePO4 */
    i2c_write16(0x36, MAX17055_DESIGN_CAP, 1500 << 1);
    i2c_write16(0x36, MAX17055_CHG_TER, 0x0640);
    return true;
}

void power_poll(void)
{
    uint16_t soc = i2c_read16(0x36, MAX17055_REPSOC);
    uint16_t vcell = i2c_read16(0x36, MAX17055_VCELL);
    int16_t  cur = (int16_t)i2c_read16(0x36, MAX17055_CURRENT);
    /* MAX17055 VCELL: 78.125 µV/LSB → mV = raw * 78.125 µV / 1000 */
    s_battery_mv = (float)vcell * 0.078125f;
    /* SoC: 1/256 % per LSB */
    s_soc_pct = (float)soc / 256.0f;
    /* current: 1.5625 µV / 5 mΩ = 0.3125 mA per LSB (LSB sign-extended) */
    s_current_ma = (float)cur * 0.3125f;

    /* panel voltage via an ADC channel on the solar input (divider) */
    s_panel_mv = adc_read_mv(ADC_PANEL_CH);
}

float power_battery_pct(void)  { return s_soc_pct; }
float power_battery_mv(void)   { return s_battery_mv; }
float power_panel_mv(void)     { return s_panel_mv; }
float power_current_ma(void)   { return s_current_ma; }

/*
 * power_adapt_interval — adjust the measurement interval based on battery
 * state-of-charge and recent harvested energy. Low battery → slow down.
 */
uint32_t power_adapt_interval(uint32_t base_s)
{
    if (s_soc_pct < TG_BATTERY_CRIT_PCT) {
        /* critical: stretch to 4× the base, capped at max */
        uint32_t v = base_s * 4;
        if (v > TG_MAX_INTERVAL_S) v = TG_MAX_INTERVAL_S;
        return v;
    }
    if (s_soc_pct < TG_BATTERY_LOW_PCT) {
        uint32_t v = base_s * 2;
        if (v > TG_MAX_INTERVAL_S) v = TG_MAX_INTERVAL_S;
        return v;
    }
    if (s_soc_pct > 70.0f && s_panel_mv > TG_PANEL_VMPPT_MV) {
        /* charging and sunny → accelerate if user configured a fast mode */
        uint32_t v = base_s / 2;
        if (v < TG_MIN_INTERVAL_S) v = TG_MIN_INTERVAL_S;
        return v;
    }
    return base_s;
}

/*
 * power_enter_stop2 — prepare peripherals for STOP2 and request it.
 * The RTC alarm or COMP1 output wakes the MCU; on wake the peripherals must
 * be re-initialised.
 */
void power_enter_stop2(void)
{
    /* gate clocks to unused peripherals, keep LSE + RTC + I2C1 (fuel gauge
     * can wake on low-SOC interrupt if configured) */
    PWR_CR1 |= 0x00000003;       /* VOS = range 2 (lowest) */
    PWR_CR3 |= BIT(0);            /* sub-domain: keep RTC + LSE */
    /* SLEEPDEEP = 1, PDDS = 0 → STOP2 (set via SCB->SCR) */
    volatile uint32_t *scb_scr = (volatile uint32_t *)0xE000ED10UL;
    *scb_scr |= BIT(2);
#ifdef __arm__
    __asm__ volatile ("wfi");
#else
    /* host build: no real WFI; just spin briefly to simulate sleep */
    volatile int spin = 1000; while (spin--) __asm__ volatile ("nop");
#endif
    *scb_scr &= ~BIT(2);
}

/* ---- low-level helpers ----
 * i2c_read16 / i2c_write16 are defined in temp.c (shared HAL stubs).
 * adc_read_mv is defined below for the panel voltage ADC channel. */
float adc_read_mv(uint8_t ch)
{
    (void)ch;
    return 0.0f;
}