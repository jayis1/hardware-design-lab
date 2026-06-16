// led_controller.c — WS2812 RGB LED Controller
// Controls 3 SK6812-EC15 RGB LEDs in a daisy chain
// Uses bit-banged timing on GPIO for WS2812 protocol
// 800 kHz data rate: T0H=0.4µs, T0L=0.85µs, T1H=0.8µs, T1L=0.45µs, RES>50µs

#include "led_controller.h"
#include "../registers.h"
#include "../board.h"
#include "gpio_init.h"

#define LED_COUNT               3
#define WS2812_RESET_US         60

// Timing constants at 100 MHz (10 ns per cycle)
#define WS2812_T0H_CYCLES       40   // 0.4 µs
#define WS2812_T0L_CYCLES       85   // 0.85 µs
#define WS2812_T1H_CYCLES       80   // 0.8 µs
#define WS2812_T1L_CYCLES       45   // 0.45 µs
#define WS2812_RESET_CYCLES     6000 // 60 µs

static uint32_t led_colors[LED_COUNT];
static bool initialized = false;

// Sequence state
static const uint32_t *seq_colors = NULL;
static uint8_t seq_count = 0;
static uint32_t seq_period_ms = 0;
static uint8_t seq_index = 0;
static uint32_t seq_next_tick = 0;
static bool seq_active = false;

// Send one bit to WS2812
static inline void ws2812_send_bit(uint8_t bit) {
    if (bit) {
        gpio_set(GPIO_PIN_LED_DATA, 1);
        for (volatile int d = 0; d < WS2812_T1H_CYCLES; d++) NOP();
        gpio_set(GPIO_PIN_LED_DATA, 0);
        for (volatile int d = 0; d < WS2812_T1L_CYCLES; d++) NOP();
    } else {
        gpio_set(GPIO_PIN_LED_DATA, 1);
        for (volatile int d = 0; d < WS2812_T0H_CYCLES; d++) NOP();
        gpio_set(GPIO_PIN_LED_DATA, 0);
        for (volatile int d = 0; d < WS2812_T0L_CYCLES; d++) NOP();
    }
}

// Send one byte (MSB first per WS2812 spec)
static void ws2812_send_byte(uint8_t byte) {
    for (int i = 7; i >= 0; i--) {
        ws2812_send_bit((byte >> i) & 1);
    }
}

// Send GRB color (WS2812 byte order: G, R, B)
static void ws2812_send_color(uint32_t color) {
    uint8_t g = (color >> 16) & 0xFF;
    uint8_t r = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;
    ws2812_send_byte(g);
    ws2812_send_byte(r);
    ws2812_send_byte(b);
}

// Send reset pulse
static void ws2812_reset(void) {
    gpio_set(GPIO_PIN_LED_DATA, 0);
    for (volatile int d = 0; d < WS2812_RESET_CYCLES; d++) NOP();
}

// Refresh all LEDs
static void ws2812_refresh(void) {
    // Disable interrupts during timing-critical transmission
    uint32_t saved_mstatus = CSR_READ(mstatus);
    CSR_WRITE(mstatus, saved_mstatus & ~MSTATUS_MIE);

    for (int i = 0; i < LED_COUNT; i++) {
        ws2812_send_color(led_colors[i]);
    }
    ws2812_reset();

    CSR_WRITE(mstatus, saved_mstatus);
}

int led_controller_init(void) {
    if (initialized) return 0;

    // Ensure LED data pin is output and low
    gpio_set(GPIO_PIN_LED_DATA, 0);

    // Initialize all LEDs to off
    for (int i = 0; i < LED_COUNT; i++) {
        led_colors[i] = LED_COLOR_OFF;
    }
    ws2812_refresh();

    initialized = true;
    return 0;
}

void led_set_all(uint32_t color) {
    for (int i = 0; i < LED_COUNT; i++) {
        led_colors[i] = color;
    }
    ws2812_refresh();
}

void led_set_single(uint8_t index, uint32_t color) {
    if (index >= LED_COUNT) return;
    led_colors[index] = color;
    ws2812_refresh();
}

void led_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    led_set_all(color);
}

void led_off(void) {
    led_set_all(LED_COLOR_OFF);
}

// Start a color sequence animation
void led_sequence_start(const uint32_t *colors, uint8_t count, uint32_t period_ms) {
    if (!colors || count == 0) return;
    seq_colors = colors;
    seq_count = count;
    seq_period_ms = period_ms;
    seq_index = 0;
    seq_next_tick = REG_READ(SYS_UPTIME_LO_REG) + (period_ms * 100000);
    seq_active = true;
}

void led_sequence_stop(void) {
    seq_active = false;
    seq_colors = NULL;
}

// Called periodically from main loop to advance sequence
void led_sequence_tick(void) {
    if (!seq_active || !seq_colors) return;

    uint32_t now = REG_READ(SYS_UPTIME_LO_REG);
    if (now >= seq_next_tick) {
        led_set_all(seq_colors[seq_index]);
        seq_index = (seq_index + 1) % seq_count;
        seq_next_tick = now + (seq_period_ms * 100000);
    }
}

// Predefined sequences
static const uint32_t boot_sequence[] = {
    LED_COLOR_OFF, LED_COLOR_DIM_RED, LED_COLOR_RED,
    LED_COLOR_OFF, LED_COLOR_DIM_GREEN, LED_COLOR_GREEN,
    LED_COLOR_OFF, LED_COLOR_DIM_BLUE, LED_COLOR_BLUE,
    LED_COLOR_WHITE
};

static const uint32_t error_blink[] = {
    LED_COLOR_RED, LED_COLOR_OFF
};

static const uint32_t activity_pulse[] = {
    LED_COLOR_CYAN, LED_COLOR_DIM_BLUE
};

void led_boot_animation(void) {
    led_sequence_start(boot_sequence, 10, 100);
}

void led_error_blink(void) {
    led_sequence_start(error_blink, 2, 500);
}

void led_activity_pulse(void) {
    led_sequence_start(activity_pulse, 2, 200);
}

// Gamma correction table (gamma 2.2, 256 entries)
static const uint8_t gamma8[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5,
    5, 6, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10,
    10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16,
    17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 23, 24, 24, 25,
    25, 26, 27, 27, 28, 29, 29, 30, 31, 32, 32, 33, 34, 35, 35, 36,
    37, 38, 39, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 50,
    51, 52, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 66, 67, 68,
    69, 70, 72, 73, 74, 75, 77, 78, 79, 81, 82, 83, 85, 86, 87, 89,
    90, 92, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114,
    115, 117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 135, 137, 138, 140, 142,
    144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 167, 169, 171, 173, 175,
    177, 180, 182, 184, 186, 189, 191, 193, 196, 198, 200, 203, 205, 208, 210, 213,
    215, 218, 220, 223, 225, 228, 231, 233, 236, 239, 241, 244, 247, 249, 252, 255
};

// Set gamma-corrected color
void led_set_gamma(uint8_t r, uint8_t g, uint8_t b) {
    led_set_rgb(gamma8[r], gamma8[g], gamma8[b]);
}
