// pulse_driver.c — Pulse Generator Driver
// Controls the avalanche transistor + SRD pulse generation circuit
// BFR92A avalanche transistor triggered by FPGA GPIO, MA44769 SRD for fast edge
// Sub-nanosecond pulse generation with programmable amplitude and repetition rate

#include "pulse_driver.h"
#include "../registers.h"
#include "../board.h"
#include <string.h>

static volatile uint32_t *pulse_base = (volatile uint32_t *)PULSE_BASE;
static bool driver_initialized = false;
static pulse_config_t current_config;
static uint32_t total_pulses_emitted = 0;
static uint32_t pulse_error_count = 0;

// SRD bias DAC lookup table: DAC value → approximate pulse amplitude (mV)
// MA44769 SRD: bias 0–25V maps to 100–500 mV output pulse amplitude
// DAC is 8-bit PWM filtered to 0–25V analog
static const uint16_t srd_amplitude_table[256] = {
    100, 102, 104, 106, 108, 110, 112, 114, 116, 118,
    120, 122, 124, 126, 128, 130, 132, 134, 136, 138,
    140, 142, 144, 146, 148, 150, 152, 154, 156, 158,
    160, 162, 164, 166, 168, 170, 172, 174, 176, 178,
    180, 182, 184, 186, 188, 190, 192, 194, 196, 198,
    200, 202, 204, 206, 208, 210, 212, 214, 216, 218,
    220, 222, 224, 226, 228, 230, 232, 234, 236, 238,
    240, 242, 244, 246, 248, 250, 252, 254, 256, 258,
    260, 262, 264, 266, 268, 270, 272, 274, 276, 278,
    280, 282, 284, 286, 288, 290, 292, 294, 296, 298,
    300, 302, 304, 306, 308, 310, 312, 314, 316, 318,
    320, 322, 324, 326, 328, 330, 332, 334, 336, 338,
    340, 342, 344, 346, 348, 350, 352, 354, 356, 358,
    360, 362, 364, 366, 368, 370, 372, 374, 376, 378,
    380, 382, 384, 386, 388, 390, 392, 394, 396, 398,
    400, 402, 404, 406, 408, 410, 412, 414, 416, 418,
    420, 422, 424, 426, 428, 430, 432, 434, 436, 438,
    440, 442, 444, 446, 448, 450, 452, 454, 456, 458,
    460, 462, 464, 466, 468, 470, 472, 474, 476, 478,
    480, 482, 484, 486, 488, 490, 492, 494, 496, 498,
    500, 502, 504, 506, 508, 510, 512, 514, 516, 518,
    520, 522, 524, 526, 528, 530, 532, 534, 536, 538,
    540, 542, 544, 546, 548, 550, 552, 554, 556, 558,
    560, 562, 564, 566, 568, 570, 572, 574, 576, 578,
    580, 582, 584, 586, 588, 590, 592, 594, 596, 598,
    600, 602, 604, 606, 608, 610
};

// Temperature compensation coefficients for SRD bias
// SRD transition time varies with temperature (~0.1% per °C)
// These coefficients adjust the bias DAC to maintain constant amplitude
#define SRD_TEMP_COEFF_PPM    1000    // 1000 ppm/°C = 0.1%/°C
#define SRD_TEMP_REF_C        25.0f   // Reference temperature for calibration

int pulse_driver_init(void) {
    if (driver_initialized) return 0;

    // Reset pulse generator core
    REG_SET_BITS(SYS_RESET_REG, SYS_RESET_PULSE);
    for (volatile int d = 0; d < 100; d++) NOP();
    REG_CLR_BITS(SYS_RESET_REG, SYS_RESET_PULSE);

    // Set safe defaults
    REG_WRITE(PULSE_CTRL_REG, 0);
    REG_WRITE(PULSE_PERIOD_REG, PULSE_PERIOD_TICKS(PULSE_DEFAULT_FREQ_HZ));
    REG_WRITE(PULSE_AMPLITUDE_REG, PULSE_DEFAULT_AMPLITUDE);
    REG_WRITE(PULSE_DELAY_REG, 0);
    REG_WRITE(PULSE_WIDTH_REG, 0);
    REG_WRITE(PULSE_CAL_REG, 0);

    // Initialize GPIO for pulse trigger and SRD bias
    gpio_set(GPIO_PULSE_TRIG, 0);
    gpio_set(GPIO_SRD_BIAS_DAC, 0);

    // Store default config
    current_config.frequency_hz = PULSE_DEFAULT_FREQ_HZ;
    current_config.amplitude_dac = PULSE_DEFAULT_AMPLITUDE;
    current_config.continuous = false;
    current_config.burst_count = 0;
    current_config.external_trigger = false;

    total_pulses_emitted = 0;
    pulse_error_count = 0;

    driver_initialized = true;
    return 0;
}

int pulse_configure(const pulse_config_t *config) {
    if (!driver_initialized) return -1;

    // Cannot reconfigure while running
    if (REG_READ(PULSE_STATUS_REG) & PULSE_STATUS_BUSY) {
        return -2;
    }

    // Validate frequency
    if (config->frequency_hz < PULSE_MIN_FREQ_HZ ||
        config->frequency_hz > PULSE_MAX_FREQ_HZ) {
        return -3;
    }

    // Validate amplitude
    if (config->amplitude_dac > 255) {
        return -4;
    }

    // Calculate period in 4 ns ticks (250 MHz ADC clock)
    uint32_t period_ticks = PULSE_PERIOD_TICKS(config->frequency_hz);

    // Minimum period: 250 ticks = 1 µs = 1 MHz max
    if (period_ticks < 250) {
        period_ticks = 250;
    }

    REG_WRITE(PULSE_PERIOD_REG, period_ticks);
    REG_WRITE(PULSE_AMPLITUDE_REG, config->amplitude_dac);

    // Build control word
    uint32_t ctrl = 0;
    if (config->continuous) {
        ctrl |= PULSE_CTRL_CONTINUOUS;
    }
    if (config->external_trigger) {
        ctrl |= PULSE_CTRL_EXT_TRIG;
    }
    if (config->burst_count > 0) {
        ctrl |= PULSE_CTRL_BURST;
        ctrl |= ((config->burst_count & 0xFF) << 8);
    }
    REG_WRITE(PULSE_CTRL_REG, ctrl);

    // Set SRD bias DAC via GPIO PWM
    gpio_set(GPIO_SRD_BIAS_DAC, 0);  // PWM duty controlled by pulse core

    // Store configuration
    memcpy(&current_config, config, sizeof(pulse_config_t));

    return 0;
}

int pulse_start(void) {
    if (!driver_initialized) return -1;

    // Ensure pulse trigger is low before enabling
    gpio_set(GPIO_PULSE_TRIG, 0);

    REG_SET_BITS(PULSE_CTRL_REG, PULSE_CTRL_ENABLE);
    return 0;
}

int pulse_stop(void) {
    if (!driver_initialized) return -1;

    REG_CLR_BITS(PULSE_CTRL_REG, PULSE_CTRL_ENABLE);

    // Ensure trigger is low after stopping
    gpio_set(GPIO_PULSE_TRIG, 0);

    return 0;
}

int pulse_single_shot(void) {
    if (!driver_initialized) return -1;

    // Check if already busy
    if (REG_READ(PULSE_STATUS_REG) & PULSE_STATUS_BUSY) {
        return -2;
    }

    // Fire single pulse
    REG_SET_BITS(PULSE_CTRL_REG, PULSE_CTRL_SINGLE);

    // Wait for completion (pulse is ~300 ps wide, but allow settling time)
    uint32_t timeout = 100000;  // ~1 ms at 100 MHz
    while (timeout > 0) {
        uint32_t status = REG_READ(PULSE_STATUS_REG);
        if (!(status & PULSE_STATUS_BUSY)) {
            total_pulses_emitted++;
            return 0;
        }
        timeout--;
    }

    pulse_error_count++;
    return -3;  // Timeout
}

int pulse_get_status(uint32_t *pulses_emitted, bool *busy) {
    if (!driver_initialized) return -1;

    uint32_t status = REG_READ(PULSE_STATUS_REG);
    *busy = (status & PULSE_STATUS_BUSY) ? true : false;
    *pulses_emitted = (status >> 16) & 0xFFFF;

    return 0;
}

void pulse_set_amplitude(uint8_t dac_value) {
    if (dac_value > 255) dac_value = 255;
    REG_WRITE(PULSE_AMPLITUDE_REG, dac_value);
    current_config.amplitude_dac = dac_value;
}

void pulse_set_frequency(uint32_t frequency_hz) {
    if (frequency_hz < PULSE_MIN_FREQ_HZ) frequency_hz = PULSE_MIN_FREQ_HZ;
    if (frequency_hz > PULSE_MAX_FREQ_HZ) frequency_hz = PULSE_MAX_FREQ_HZ;

    uint32_t period_ticks = PULSE_PERIOD_TICKS(frequency_hz);
    if (period_ticks < 250) period_ticks = 250;

    REG_WRITE(PULSE_PERIOD_REG, period_ticks);
    current_config.frequency_hz = frequency_hz;
}

// Get approximate pulse amplitude in mV for a given DAC value
uint16_t pulse_dac_to_mv(uint8_t dac_value) {
    return srd_amplitude_table[dac_value];
}

// Get DAC value for a target amplitude in mV (closest match)
uint8_t pulse_mv_to_dac(uint16_t target_mv) {
    if (target_mv <= 100) return 0;
    if (target_mv >= 610) return 255;

    // Binary search the amplitude table
    int lo = 0, hi = 255;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        if (srd_amplitude_table[mid] < target_mv) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    // Check which is closer: lo or lo-1
    if (lo > 0) {
        uint16_t diff_lo = (srd_amplitude_table[lo] > target_mv)
                         ? (srd_amplitude_table[lo] - target_mv)
                         : (target_mv - srd_amplitude_table[lo]);
        uint16_t diff_prev = (srd_amplitude_table[lo-1] > target_mv)
                           ? (srd_amplitude_table[lo-1] - target_mv)
                           : (target_mv - srd_amplitude_table[lo-1]);
        if (diff_prev < diff_lo) return lo - 1;
    }
    return lo;
}

// Apply temperature compensation to SRD bias
// As temperature rises, SRD transition slows; increase bias to compensate
uint8_t pulse_temp_compensate(uint8_t base_dac, float temperature_c) {
    float delta_t = temperature_c - SRD_TEMP_REF_C;
    float correction_factor = 1.0f + (delta_t * SRD_TEMP_COEFF_PPM / 1000000.0f);

    int16_t adjusted = (int16_t)((float)base_dac * correction_factor);
    if (adjusted < 0) adjusted = 0;
    if (adjusted > 255) adjusted = 255;

    return (uint8_t)adjusted;
}

// Get total pulses emitted since init
uint32_t pulse_get_total_count(void) {
    return total_pulses_emitted;
}

// Get pulse error count
uint32_t pulse_get_error_count(void) {
    return pulse_error_count;
}

// Reset pulse statistics
void pulse_reset_statistics(void) {
    total_pulses_emitted = 0;
    pulse_error_count = 0;
}

// Set propagation delay compensation (in picoseconds)
void pulse_set_delay_compensation(uint16_t delay_ps) {
    REG_WRITE(PULSE_CAL_REG,
        (REG_READ(PULSE_CAL_REG) & 0xFFFF0000) | (delay_ps & 0xFFFF));
}

// Set amplitude calibration factor (16.16 fixed point, 1.0 = 0x10000)
void pulse_set_amplitude_cal(uint32_t cal_factor) {
    REG_WRITE(PULSE_CAL_REG,
        (REG_READ(PULSE_CAL_REG) & 0x0000FFFF) | ((cal_factor & 0xFFFF) << 16));
}

// Burst mode: fire N pulses with minimal gap
int pulse_burst(uint8_t count) {
    if (!driver_initialized) return -1;
    if (count == 0) return -2;
    if (REG_READ(PULSE_STATUS_REG) & PULSE_STATUS_BUSY) return -3;

    pulse_config_t burst_cfg = current_config;
    burst_cfg.continuous = false;
    burst_cfg.burst_count = count;
    burst_cfg.external_trigger = false;

    int ret = pulse_configure(&burst_cfg);
    if (ret != 0) return ret;

    ret = pulse_start();
    if (ret != 0) return ret;

    // Wait for burst completion
    uint32_t timeout = count * 100000;  // ~1 ms per pulse
    while (timeout > 0) {
        uint32_t status = REG_READ(PULSE_STATUS_REG);
        if (!(status & PULSE_STATUS_BUSY) && !(status & PULSE_STATUS_BURST_ACTIVE)) {
            total_pulses_emitted += count;
            return 0;
        }
        timeout--;
    }

    pulse_stop();
    pulse_error_count += count;
    return -4;
}

// Self-test: verify pulse generator responds
int pulse_self_test(void) {
    if (!driver_initialized) return -1;

    // Test 1: Single shot
    int ret = pulse_single_shot();
    if (ret != 0) return -10;

    // Test 2: Verify status register is readable
    uint32_t pulses;
    bool busy;
    ret = pulse_get_status(&pulses, &busy);
    if (ret != 0) return -11;
    if (busy) return -12;  // Should not be busy after single shot

    // Test 3: Amplitude table integrity
    for (int i = 0; i < 255; i++) {
        if (srd_amplitude_table[i] >= srd_amplitude_table[i+1]) {
            return -13;  // Table not monotonically increasing
        }
    }

    // Test 4: Frequency validation
    pulse_set_frequency(1000);
    if (current_config.frequency_hz != 1000) return -14;

    pulse_set_frequency(1000000);
    if (current_config.frequency_hz != 1000000) return -15;

    // Restore default
    pulse_set_frequency(PULSE_DEFAULT_FREQ_HZ);

    return 0;
}
