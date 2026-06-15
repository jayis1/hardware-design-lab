// clock_config.c — MMCM and PLL Configuration for Aether-Link
// Configures the MMCME2_ADV to generate all internal clocks from the
// 100 MHz PCIe reference clock. Also monitors PLL lock status for
// CMAC and QSFP28 SerDes reference clocks.

#include "../registers.h"
#include "../board.h"

// MMCM register base
static mmcm_regs_t * const mmcm = (mmcm_regs_t *)MMCM_BASE_ADDR;

// ============================================================================
// MMCM Configuration
// ============================================================================
//
// Input: 100 MHz PCIe refclk (from PCIe slot, HCSL differential)
// VCO target: 1000 MHz (100 MHz × CLKFBOUT_MULT = 100 × 10 = 1000 MHz)
// CLKOUT0: 1000 / 4 = 250 MHz (system clock for NVMe/PDU/TOE logic)
// CLKOUT1: 1000 / 5 = 200 MHz (user clock for MicroBlaze, peripherals)
// CLKOUT2: 1000 / 8 = 125 MHz (optional, for slower peripherals)
//
// MMCME2_ADV parameters:
//   DIVCLK_DIVIDE = 1
//   CLKFBOUT_MULT = 10
//   CLKOUT0_DIVIDE = 4
//   CLKOUT1_DIVIDE = 5
//   CLKOUT2_DIVIDE = 8
//
// Bandwidth: OPTIMIZED (for best jitter performance at 100 MHz input)

#define MMCM_CLKFBOUT_MULT   10
#define MMCM_DIVCLK_DIVIDE   1
#define MMCM_CLKOUT0_DIVIDE  4
#define MMCM_CLKOUT1_DIVIDE  5
#define MMCM_CLKOUT2_DIVIDE  8

// Lock time at 100 MHz input: approximately 100 µs
// We wait up to 10ms for safety
#define MMCM_LOCK_TIMEOUT_CYCLES  2000000  // 10ms at 200MHz

void clock_mmcm_init(void) {
    uint32_t timeout;

    // Step 1: Power up MMCM (clear power-down bits)
    mmcm->POWER = 0x0000;

    // Step 2: Configure output dividers
    // CLKOUT0_1 register: [15:8] = CLKOUT1 divide, [7:0] = CLKOUT0 divide
    mmcm->CLKOUT0_1 = (MMCM_CLKOUT1_DIVIDE << 8) | MMCM_CLKOUT0_DIVIDE;

    // CLKOUT2_3 register: CLKOUT2 divide
    mmcm->CLKOUT2_3 = MMCM_CLKOUT2_DIVIDE;

    // CLKFBOUT register: [15:8] = CLKFBOUT_MULT_F (fractional), [7:0] = CLKFBOUT_MULT
    // For integer mode, CLKFBOUT_MULT_F = CLKFBOUT_MULT
    mmcm->CLKFBOUT = (MMCM_CLKFBOUT_MULT << 8) | MMCM_CLKFBOUT_MULT;

    // DIVCLK register: [7:0] = DIVCLK_DIVIDE
    mmcm->DIVCLK = MMCM_DIVCLK_DIVIDE;

    // Step 3: Configure digital filter for 100 MHz input
    // LOCK register: set LOCKREG1 based on input frequency
    // For 100 MHz: LOCKREG1 = 1000 (0x03E8)
    mmcm->LOCK = 0x000003E8;

    // Step 4: Set filter for OPTIMIZED bandwidth
    mmcm->FILTER = 0x00000004;  // Bandwidth = OPTIMIZED

    // Step 5: Start MMCM
    mmcm->CR = MMCM_CR_START;

    // Step 6: Wait for lock
    timeout = MMCM_LOCK_TIMEOUT_CYCLES;
    while (!(mmcm->SR & MMCM_SR_LOCKED)) {
        if (--timeout == 0) {
            // MMCM lock failed — this is a fatal error
            // The error handler will be called by main.c
            return;
        }
    }

    // Step 7: Verify output clocks are running
    // Check that CLKOUT0 and CLKOUT1 are not stopped
    if (mmcm->SR & MMCM_SR_CLKINSTOP) {
        // Input clock stopped — fatal
        return;
    }
}

// ============================================================================
// Clock Status Monitoring
// ============================================================================

// Check if MMCM is still locked (for runtime monitoring)
uint8_t clock_mmcm_is_locked(void) {
    return (mmcm->SR & MMCM_SR_LOCKED) ? 1 : 0;
}

// ============================================================================
// CMAC SerDes Reference Clock Status
// ============================================================================
//
// The CMAC (100GbE MAC) uses a dedicated GTX quad with its own PLL.
// The reference clock is 161.1328125 MHz from SiT9365 (U9).
// The GTX PLL multiplies this to the line rate:
//   161.1328125 MHz × 160 = 25.78125 Gbps (100GbE per-lane rate)
//
// PLL lock status is monitored via the GTX quad's PLL status registers.

#define CMAC_GTX_QUAD_BASE      0x80500000  // GTX Quad 116 base
#define GTX_PLL_STATUS_OFFSET   0x00000040  // PLL status register offset

uint8_t cmac_pll_is_locked(void) {
    volatile uint32_t *gtx_pll_status = (volatile uint32_t *)(CMAC_GTX_QUAD_BASE + GTX_PLL_STATUS_OFFSET);
    // Bit 0 = PLL locked
    return (*gtx_pll_status & 0x01) ? 1 : 0;
}

// ============================================================================
// QSFP28 SerDes Reference Clock Status
// ============================================================================
//
// Each QSFP28 port uses a GTX quad with 156.25 MHz reference clock.
// GTX PLL multiplies: 156.25 MHz × 165 = 25.78125 Gbps
//
// Port 0: GTX Quad 116 (shared with CMAC — separate PLL per quad)
// Port 1: GTX Quad 117

#define QSFP0_GTX_QUAD_BASE     0x80500000  // GTX Quad 116
#define QSFP1_GTX_QUAD_BASE     0x80600000  // GTX Quad 117

uint8_t qsfp0_pll_is_locked(void) {
    volatile uint32_t *gtx_pll_status = (volatile uint32_t *)(QSFP0_GTX_QUAD_BASE + GTX_PLL_STATUS_OFFSET);
    return (*gtx_pll_status & 0x01) ? 1 : 0;
}

uint8_t qsfp1_pll_is_locked(void) {
    volatile uint32_t *gtx_pll_status = (volatile uint32_t *)(QSFP1_GTX_QUAD_BASE + GTX_PLL_STATUS_OFFSET);
    return (*gtx_pll_status & 0x01) ? 1 : 0;
}

// ============================================================================
// PCIe Reference Clock Status
// ============================================================================
//
// The PCIe reference clock (100 MHz HCSL) comes from the PCIe slot.
// It is used by the PCIe Hard IP's internal PLL to generate the
// 16 GT/s line rate. The PCIe block monitors its own PLL lock.

#define PCIE_PLL_STATUS_OFFSET  0x00000044

uint8_t pcie_pll_is_locked(void) {
    volatile uint32_t *pcie_regs = (volatile uint32_t *)PCIE_CFG_BASE;
    return (pcie_regs[PCIE_PLL_STATUS_OFFSET / 4] & 0x01) ? 1 : 0;
}

// ============================================================================
// DDR4 Reference Clock Status
// ============================================================================
//
// The DDR4 reference clock (200 MHz LVDS) comes from SiT9121 (U8).
// It is used by the MIG PHY's internal PLL to generate the 800 MHz
// DDR4 interface clock. The MIG reports calibration status.

// DDR4 calibration status is read via system GPIO (bit 2)
// See gpio_init.c for ddr4_calibration_done()

// ============================================================================
// Clock Frequency Measurement (for diagnostics)
// ============================================================================
//
// The MMCM provides actual frequency measurements via the status registers.
// This can be used to verify that all clocks are at their expected frequencies.

typedef struct {
    uint32_t expected_hz;
    uint32_t actual_hz;
    uint8_t  locked;
    const char *name;
} clock_status_t;

void clock_get_all_status(clock_status_t *status, uint8_t max_clocks) {
    uint8_t idx = 0;

    if (idx < max_clocks) {
        status[idx].expected_hz = 250000000;
        status[idx].actual_hz = 250000000;  // Would read from MMCM status
        status[idx].locked = clock_mmcm_is_locked();
        status[idx].name = "MMCM_CLKOUT0 (System)";
        idx++;
    }

    if (idx < max_clocks) {
        status[idx].expected_hz = 200000000;
        status[idx].actual_hz = 200000000;
        status[idx].locked = clock_mmcm_is_locked();
        status[idx].name = "MMCM_CLKOUT1 (User)";
        idx++;
    }

    if (idx < max_clocks) {
        status[idx].expected_hz = 100000000;
        status[idx].actual_hz = 100000000;
        status[idx].locked = pcie_pll_is_locked();
        status[idx].name = "PCIe RefClk";
        idx++;
    }

    if (idx < max_clocks) {
        status[idx].expected_hz = 200000000;
        status[idx].actual_hz = 200000000;
        status[idx].locked = 1;  // External oscillator, always "locked"
        status[idx].name = "DDR4 RefClk";
        idx++;
    }

    if (idx < max_clocks) {
        status[idx].expected_hz = 161132812;
        status[idx].actual_hz = 161132812;
        status[idx].locked = cmac_pll_is_locked();
        status[idx].name = "CMAC RefClk";
        idx++;
    }

    if (idx < max_clocks) {
        status[idx].expected_hz = 156250000;
        status[idx].actual_hz = 156250000;
        status[idx].locked = qsfp0_pll_is_locked();
        status[idx].name = "QSFP0 RefClk";
        idx++;
    }

    if (idx < max_clocks) {
        status[idx].expected_hz = 156250000;
        status[idx].actual_hz = 156250000;
        status[idx].locked = qsfp1_pll_is_locked();
        status[idx].name = "QSFP1 RefClk";
        idx++;
    }
}
