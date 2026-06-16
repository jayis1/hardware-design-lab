// clock_config.c — Clock Tree Initialization for Chronos Pulser
// Configures ECP5 PLLs for all clock domains
// PLL0: 250 MHz ref → 250 MHz ADC + 500 MHz TDC
// PLL1: 250 MHz ref → 100 MHz system + 100 MHz USB
// PLL2: 250 MHz ref → 400 MHz DDR3

#include "clock_config.h"
#include "../registers.h"
#include "../board.h"

#define PLL_LOCK_TIMEOUT  10000
#define PLL_LOCK_POLL_DELAY 100

typedef enum {
    CLOCK_SOURCE_REF_250MHZ = 0,
    CLOCK_SOURCE_PLL0_250MHZ = 1,
    CLOCK_SOURCE_PLL0_500MHZ = 2,
    CLOCK_SOURCE_PLL1_100MHZ = 3,
    CLOCK_SOURCE_PLL2_400MHZ = 4
} clock_source_t;

typedef struct {
    uint32_t frequency_hz;
    clock_source_t source;
    uint8_t  pll_instance;
    uint8_t  pll_output;
    uint8_t  divider;
    uint8_t  enabled;
} clock_domain_t;

static clock_domain_t clock_domains[CLOCK_DOMAIN_COUNT] = {
    [CLOCK_SYS]   = { .frequency_hz = 100000000, .source = CLOCK_SOURCE_PLL1_100MHZ,
                      .pll_instance = 1, .pll_output = 0, .divider = 1, .enabled = 0 },
    [CLOCK_ADC]   = { .frequency_hz = 250000000, .source = CLOCK_SOURCE_PLL0_250MHZ,
                      .pll_instance = 0, .pll_output = 0, .divider = 1, .enabled = 0 },
    [CLOCK_TDC]   = { .frequency_hz = 500000000, .source = CLOCK_SOURCE_PLL0_500MHZ,
                      .pll_instance = 0, .pll_output = 1, .divider = 1, .enabled = 0 },
    [CLOCK_USB]   = { .frequency_hz = 100000000, .source = CLOCK_SOURCE_PLL1_100MHZ,
                      .pll_instance = 1, .pll_output = 1, .divider = 1, .enabled = 0 },
    [CLOCK_DDR3]  = { .frequency_hz = 400000000, .source = CLOCK_SOURCE_PLL2_400MHZ,
                      .pll_instance = 2, .pll_output = 0, .divider = 1, .enabled = 0 },
};

#define PLL0_CTRL_OFFSET    0x40
#define PLL1_CTRL_OFFSET    0x48
#define PLL2_CTRL_OFFSET    0x50
#define PLL_STATUS_OFFSET   0x60

#define PLL_CTRL_RESET      (1 << 0)
#define PLL_CTRL_PWRDN      (1 << 1)
#define PLL_CTRL_BYPASS     (1 << 2)
#define PLL_CTRL_LOCK       (1 << 31)

#define PLL_DIV_M_MASK      0x3F
#define PLL_DIV_N_MASK      0x3F
#define PLL_DIV_M_SHIFT     0
#define PLL_DIV_N_SHIFT     8

#define PLL_CLKOP_DIV_MASK  0x7F
#define PLL_CLKOS_DIV_MASK  0x7F
#define PLL_CLKOS2_DIV_MASK 0x7F
#define PLL_CLKOS3_DIV_MASK 0x7F

static volatile uint32_t *sys_clk_base = (volatile uint32_t *)SYS_CLK_CTRL_REG;

static int pll_wait_lock(int pll_index, uint32_t timeout_cycles) {
    volatile uint32_t *pll_ctrl = sys_clk_base + (PLL0_CTRL_OFFSET / 4)
                                  + (pll_index * 2);
    uint32_t waited = 0;

    while (!(*pll_ctrl & PLL_CTRL_LOCK)) {
        for (volatile int d = 0; d < PLL_LOCK_POLL_DELAY; d++) NOP();
        waited += PLL_LOCK_POLL_DELAY;
        if (waited >= timeout_cycles) return -1;
    }
    return 0;
}

static int pll_configure(int pll_index, uint8_t divr, uint8_t divf,
                         uint8_t divq_op, uint8_t divq_os,
                         uint8_t divq_os2, uint8_t divq_os3) {
    volatile uint32_t *pll_base = sys_clk_base + (PLL0_CTRL_OFFSET / 4)
                                  + (pll_index * 2);

    pll_base[0] = PLL_CTRL_PWRDN | PLL_CTRL_RESET;
    pll_base[1] = ((divf & PLL_DIV_M_MASK) << PLL_DIV_M_SHIFT)
                | ((divr & PLL_DIV_N_MASK) << PLL_DIV_N_SHIFT);
    pll_base[2] = (divq_op & PLL_CLKOP_DIV_MASK)
                | ((divq_os & PLL_CLKOS_DIV_MASK) << 8)
                | ((divq_os2 & PLL_CLKOS2_DIV_MASK) << 16)
                | ((divq_os3 & PLL_CLKOS3_DIV_MASK) << 24);

    pll_base[0] = PLL_CTRL_PWRDN;
    for (volatile int d = 0; d < 100; d++) NOP();
    pll_base[0] = 0;

    return pll_wait_lock(pll_index, PLL_LOCK_TIMEOUT);
}

int clock_init(void) {
    int ret;

    // PLL0: 250 MHz → 250 MHz ADC + 500 MHz TDC
    // Fvco = 250 * 16 = 4000 MHz, CLKOP = 4000/16 = 250, CLKOS = 4000/8 = 500
    ret = pll_configure(0, 0, 15, 16, 8, 1, 1);
    if (ret != 0) return -1;

    // PLL1: 250 MHz → 100 MHz system + 100 MHz USB
    // Fvco = 250 * 20 = 5000 MHz, CLKOP = 5000/50 = 100, CLKOS = 5000/50 = 100
    ret = pll_configure(1, 0, 19, 50, 50, 1, 1);
    if (ret != 0) return -2;

    // PLL2: 250 MHz → 400 MHz DDR3
    // Fvco = 250 * 24 = 6000 MHz, CLKOP = 6000/15 = 400
    ret = pll_configure(2, 0, 23, 15, 1, 1, 1);
    if (ret != 0) return -3;

    uint32_t clk_ctrl = SYS_CLK_ENABLE_SYS | SYS_CLK_ENABLE_ADC
                      | SYS_CLK_ENABLE_TDC | SYS_CLK_ENABLE_USB
                      | SYS_CLK_ENABLE_DDR3;
    sys_clk_base[0] = clk_ctrl;

    for (int i = 0; i < CLOCK_DOMAIN_COUNT; i++) {
        clock_domains[i].enabled = 1;
    }

    return 0;
}

uint32_t clock_get_frequency(clock_domain_id_t domain) {
    if (domain >= CLOCK_DOMAIN_COUNT) return 0;
    return clock_domains[domain].frequency_hz;
}

int clock_domain_disable(clock_domain_id_t domain) {
    if (domain >= CLOCK_DOMAIN_COUNT) return -1;
    uint32_t mask;
    switch (domain) {
        case CLOCK_SYS:  mask = SYS_CLK_ENABLE_SYS;  break;
        case CLOCK_ADC:  mask = SYS_CLK_ENABLE_ADC;  break;
        case CLOCK_TDC:  mask = SYS_CLK_ENABLE_TDC;  break;
        case CLOCK_USB:  mask = SYS_CLK_ENABLE_USB;  break;
        case CLOCK_DDR3: mask = SYS_CLK_ENABLE_DDR3; break;
        default: return -1;
    }
    sys_clk_base[0] &= ~mask;
    clock_domains[domain].enabled = 0;
    return 0;
}

int clock_domain_enable(clock_domain_id_t domain) {
    if (domain >= CLOCK_DOMAIN_COUNT) return -1;
    uint32_t mask;
    switch (domain) {
        case CLOCK_SYS:  mask = SYS_CLK_ENABLE_SYS;  break;
        case CLOCK_ADC:  mask = SYS_CLK_ENABLE_ADC;  break;
        case CLOCK_TDC:  mask = SYS_CLK_ENABLE_TDC;  break;
        case CLOCK_USB:  mask = SYS_CLK_ENABLE_USB;  break;
        case CLOCK_DDR3: mask = SYS_CLK_ENABLE_DDR3; break;
        default: return -1;
    }
    sys_clk_base[0] |= mask;
    clock_domains[domain].enabled = 1;
    return 0;
}
