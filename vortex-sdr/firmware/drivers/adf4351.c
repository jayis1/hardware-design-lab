/*
 * adf4351.c — ADF4351 PLL Synthesizer driver implementation
 * Bit-bang SPI on GPIO pins PB10 (LE), PB11 (CE), PA5-PA7 (shared SPI1)
 */

#include <stdint.h>
#include "board.h"
#include "registers.h"
#include "adf4351.h"

/* ========================================================================== */
/* Bit-bang SPI interface for ADF4351                                          */
/* The ADF4351 uses a 3-wire SPI: CLK, DATA, LE                               */
/* CLK and DATA are shared with SPI1 (PA5=CLK, PA7=DATA)                     */
/* LE is on PB10, CE is on PB11                                               */
/* ========================================================================== */

static void adf4351_spi_write(uint32_t data)
{
    /* The ADF4351 uses a separate LE (Latch Enable) signal.
     * We bit-bang the data on the shared SPI1 pins (PA5=CLK, PA7=DATA).
     * Data is clocked in MSB first on rising edge of CLK.
     * LE rising edge latches the data into the register specified by bits [2:0].
     */

    /* Disable SPI1 to allow bit-bang control */
    SPI1->CR1 &= ~SPI_CR1_SPE;

    /* Configure PA5 and PA7 as general-purpose outputs for bit-bang */
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << 10) & ~(3UL << 14))
                 | (1UL << 10)   /* PA5: Output (SPI1_CLK bit-bang) */
                 | (1UL << 14);  /* PA7: Output (SPI1_MOSI bit-bang) */

    /* Clock in 32 bits, MSB first */
    for (int i = 31; i >= 0; i--) {
        /* Set DATA bit */
        if (data & (1UL << i)) {
            SET_BIT(GPIOA->ODR, 7);   /* PA7 = DATA = 1 */
        } else {
            CLR_BIT(GPIOA->ODR, 7);   /* PA7 = DATA = 0 */
        }

        /* CLK low */
        CLR_BIT(GPIOA->ODR, 5);   /* PA5 = CLK = 0 */

        /* Small delay for setup time */
        for (volatile int d = 0; d < 2; d++)
            ;

        /* CLK high (data sampled on rising edge) */
        SET_BIT(GPIOA->ODR, 5);   /* PA5 = CLK = 1 */

        /* Small delay for hold time */
        for (volatile int d = 0; d < 2; d++)
            ;
    }

    /* CLK low after last bit */
    CLR_BIT(GPIOA->ODR, 5);

    /* Latch data: LE high pulse */
    for (volatile int d = 0; d < 5; d++)
        ;
    SET_BIT(GPIOB->ODR, PIN_PLL_LE);   /* LE high */
    for (volatile int d = 0; d < 10; d++)
        ;
    CLR_BIT(GPIOB->ODR, PIN_PLL_LE);   /* LE low */
    for (volatile int d = 0; d < 5; d++)
        ;

    /* Restore PA5 and PA7 to SPI1 AF mode */
    GPIOA->MODER = (GPIOA->MODER & ~(3UL << 10) & ~(3UL << 14))
                 | (2UL << 10)   /* PA5: AF5 (SPI1_SCK) */
                 | (2UL << 14);  /* PA7: AF5 (SPI1_MOSI) */

    /* Re-enable SPI1 */
    SPI1->CR1 |= SPI_CR1_SPE;
}

/* ========================================================================== */
/* Calculate PLL parameters for target frequency                               */
/*                                                                             */
/* ADF4351 architecture:                                                       */
/*   f_VCO = f_PFD * (INT + FRAC/MOD)                                        */
/*   f_RF = f_VCO / RF_DIV                                                    */
/*   f_PFD = f_REF / R                                                        */
/*                                                                             */
/* Constraints:                                                                */
/*   PFD = 10 MHz to 125 MHz (typical 25-100 MHz)                            */
/*   INT = 16 to 65535 (or 23 to 65535 with 8/9 prescaler)                   */
/*   FRAC = 0 to MOD-1                                                        */
/*   MOD = 2 to 65535                                                          */
/*   VCO = 2200 MHz to 4400 MHz                                               */
/* ========================================================================== */

typedef struct {
    uint16_t r_div;      /* R counter value */
    uint16_t int_val;    /* Integer value */
    uint16_t frac_val;   /* Fractional value */
    uint16_t mod_val;    /* Modulus value */
    uint8_t  rf_div;     /* RF output divider */
    uint8_t  prescaler;  /* 4/5 or 8/9 */
} pll_params_t;

static uint8_t calculate_pll_params(uint64_t target_freq_hz, pll_params_t *params)
{
    /* Determine RF divider based on target frequency */
    if (target_freq_hz >= 2200000000UL) {
        params->rf_div = ADF4351_RF_DIV_1;   /* Direct, no division */
    } else if (target_freq_hz >= 1100000000UL) {
        params->rf_div = ADF4351_RF_DIV_2;   /* /2 */
    } else if (target_freq_hz >= 550000000UL) {
        params->rf_div = ADF4351_RF_DIV_4;   /* /4 */
    } else if (target_freq_hz >= 275000000UL) {
        params->rf_div = ADF4351_RF_DIV_8;   /* /8 */
    } else if (target_freq_hz >= 137500000UL) {
        params->rf_div = ADF4351_RF_DIV_16;  /* /16 */
    } else if (target_freq_hz >= 68750000UL) {
        params->rf_div = ADF4351_RF_DIV_32;  /* /32 */
    } else {
        params->rf_div = ADF4351_RF_DIV_64;  /* /64 */
    }

    /* Calculate VCO frequency */
    static const uint8_t rf_div_table[] = {1, 2, 4, 8, 16, 32, 64};
    uint64_t vco_freq = target_freq_hz * rf_div_table[params->rf_div];

    /* Check VCO range */
    if (vco_freq < 2200000000UL || vco_freq > 4400000000UL) {
        return 1;  /* VCO out of range */
    }

    /* Choose R counter for PFD frequency of 25 MHz (reference / R = 25 MHz) */
    /* f_PFD = f_REF / R, we want f_PFD as high as possible for best phase noise */
    /* With f_REF = 25 MHz, use R = 1 for PFD = 25 MHz */
    params->r_div = 1;
    uint64_t pfd_freq = ADF4351_REF_FREQ_HZ / params->r_div;

    /* Choose prescaler: use 4/5 for INT >= 75, 8/9 for INT >= 23 */
    /* We prefer 4/5 for better spurious performance */
    /* But for low VCO frequencies with high PFD, we may need 8/9 */

    /* Calculate INT and FRAC */
    /* N = f_VCO / f_PFD = INT + FRAC/MOD */
    /* Use MOD = 15625 (for 1 Hz resolution at 25 MHz PFD: 25e6 / 15625 = 1600 Hz)
     * Actually for 1 Hz resolution: FRAC = (f_VCO - INT * f_PFD) * MOD / f_PFD
     * With MOD = f_PFD (e.g., MOD = 25000000), we get 1 Hz resolution.
     * But MOD must be <= 65535, so we need to find a suitable MOD.
     * MOD = 15625 gives 25e6/15625 = 1600 Hz resolution.
     * MOD = f_PFD / gcd(...) for best resolution.
     */

    /* Use a fixed modulus of 15625 for simplicity */
    params->mod_val = 15625;

    /* Calculate INT */
    uint64_t int_n = vco_freq / pfd_freq;
    if (int_n < 75) {
        /* Use 8/9 prescaler for INT < 75 */
        params->prescaler = 1;  /* 8/9 */
        if (int_n < 23) int_n = 23;  /* Minimum INT with 8/9 */
    } else {
        params->prescaler = 0;  /* 4/5 */
    }

    params->int_val = (uint16_t)int_n;

    /* Calculate FRAC */
    uint64_t remainder = vco_freq - (int_n * pfd_freq);
    uint64_t frac_val = (remainder * params->mod_val) / pfd_freq;

    /* Round to nearest integer */
    uint64_t frac_round = ((remainder * params->mod_val * 2) / pfd_freq + 1) / 2;
    params->frac_val = (uint16_t)frac_round;

    if (params->frac_val >= params->mod_val) {
        params->frac_val = 0;
        params->int_val++;
    }

    return 0;
}

/* ========================================================================== */
/* Initialize ADF4351                                                          */
/* ========================================================================== */
void adf4351_init(void)
{
    /* Enable PLL chip (CE high) */
    SET_BIT(GPIOB->ODR, PIN_PLL_CE);

    /* LE low (idle) */
    CLR_BIT(GPIOB->ODR, PIN_PLL_LE);

    /* Small delay for PLL power-up */
    for (volatile uint32_t i = 0; i < 10000; i++)
        ;

    /* Write registers in reverse order (R5 -> R4 -> R3 -> R2 -> R1 -> R0) */
    /* This ensures all registers are loaded before R0 triggers autocalibration */

    /* Register 5: LD pin = digital lock detect */
    uint32_t r5 = (5UL << 0)                          /* Register address */
                | ADF4351_R5_LD_PIN_DLD;              /* Digital lock detect */
    adf4351_spi_write(r5);

    /* Register 4: RF output disabled, fundamental feedback */
    uint32_t r4 = (4UL << 0)                          /* Register address */
                | (0UL << ADF4351_R4_RF_DIV_POS)      /* RF_DIV = 1 */
                | (1UL << ADF4351_R4_FB_POS)           /* Fundamental feedback */
                | ADF4351_R4_MUX_LOGIC_CMOS            /* CMOS MUX output */
                | (3UL << ADF4351_R4_CP_THREE_POS)     /* CP three-state off */
                | (0UL << ADF4351_R4_CP_MODE_POS);     /* CP mode = normal */
    adf4351_spi_write(r4);

    /* Register 3: CSR disabled, clock divider off */
    uint32_t r3 = (3UL << 0)                          /* Register address */
                | (0UL << ADF4351_R3_CSR)              /* No cycle slip reduction */
                | (0UL << ADF4351_R3_DIV_SEL_POS)     /* Divider = 1 */
                | (0UL << ADF4351_R3_CLK_DIV_POS)      /* Clock divider = 0 */
                | (0UL << ADF4351_R3_CLK_DIV_MODE_POS);/* Clock div mode = off */
    adf4351_spi_write(r3);

    /* Register 2: R=1, CP=2.1 mA, positive polarity */
    uint32_t r2 = (2UL << 0)                          /* Register address */
                | (1UL << ADF4351_R2_R_POS)            /* R = 1 */
                | (5UL << ADF4351_R2_CP_POS)           /* CP = 2.1 mA */
                | ADF4351_R2_PD_POL_POS                /* Positive polarity */
                | ADF4351_R2_MUX_THREE                 /* MUX = digital lock detect */
                | ADF4351_R2_LDP_6ns;                  /* LDP = 6 ns */
    adf4351_spi_write(r2);

    /* Register 1: MOD=15625, phase=1 */
    uint32_t r1 = (1UL << 0)                           /* Register address */
                | (15625UL << ADF4351_R1_MOD_POS)      /* MOD = 15625 */
                | (1UL << ADF4351_R1_PHASE_POS)         /* Phase = 1 */
                | (0UL << 27);                           /* 4/5 prescaler */
    adf4351_spi_write(r1);

    /* Register 0: INT=75 (default 1875 MHz), autocal enable */
    uint32_t r0 = (0UL << 0)                           /* Register address */
                | ADF4351_R0_AUTOCAL                   /* Enable autocalibration */
                | (75UL << ADF4351_R0_INT_POS);         /* INT = 75 */
    adf4351_spi_write(r0);
}

/* ========================================================================== */
/* Set ADF4351 output frequency                                                */
/* ========================================================================== */
void adf4351_set_frequency(uint64_t freq_hz)
{
    pll_params_t params;

    /* Calculate PLL parameters */
    if (calculate_pll_params(freq_hz, &params) != 0) {
        return;  /* Frequency out of range */
    }

    /* Register 5 (write first to avoid glitches) */
    uint32_t r5 = (5UL << 0)
                | ADF4351_R5_LD_PIN_DLD;
    adf4351_spi_write(r5);

    /* Register 4: Set RF divider, enable output, fundamental feedback */
    uint32_t r4 = (4UL << 0)
                | ((uint32_t)params.rf_div << ADF4351_R4_RF_DIV_POS)
                | (1UL << ADF4351_R4_FB_POS)            /* Fundamental feedback */
                | ADF4351_R4_MUX_LOGIC_CMOS
                | (3UL << ADF4351_R4_CP_THREE_POS)
                | (0UL << ADF4351_R4_CP_MODE_POS)
                | ADF4351_R4_AUX_EN                     /* Enable AUX output */
                | (2UL << ADF4351_R4_AUX_SEL_POS);      /* AUX = RF output */
    adf4351_spi_write(r4);

    /* Register 3: CSR disabled */
    uint32_t r3 = (3UL << 0)
                | (0UL << ADF4351_R3_CSR)
                | (0UL << ADF4351_R3_DIV_SEL_POS)
                | (0UL << ADF4351_R3_CLK_DIV_POS)
                | (0UL << ADF4351_R3_CLK_DIV_MODE_POS);
    adf4351_spi_write(r3);

    /* Register 2: R counter, CP current, polarity */
    uint32_t r2 = (2UL << 0)
                | ((uint32_t)params.r_div << ADF4351_R2_R_POS)
                | (5UL << ADF4351_R2_CP_POS)            /* CP = 2.1 mA */
                | ADF4351_R2_PD_POL_POS
                | ADF4351_R2_MUX_THREE
                | ADF4351_R2_LDP_6ns;
    adf4351_spi_write(r2);

    /* Register 1: Modulus, phase, prescaler */
    uint32_t r1 = (1UL << 0)
                | ((uint32_t)params.mod_val << ADF4351_R1_MOD_POS)
                | (1UL << ADF4351_R1_PHASE_POS)
                | ((uint32_t)params.prescaler << 27);    /* Prescaler select */
    adf4351_spi_write(r1);

    /* Register 0: Integer, fractional, autocal (write last) */
    uint32_t r0 = (0UL << 0)
                | ADF4351_R0_AUTOCAL
                | ((uint32_t)params.int_val << ADF4351_R0_INT_POS)
                | ((uint32_t)params.frac_val << ADF4351_R0_FRAC_POS);
    adf4351_spi_write(r0);
}

/* ========================================================================== */
/* Check PLL lock detect                                                      */
/* ========================================================================== */
uint8_t adf4351_lock_detect(void)
{
    /* Read MUXOUT pin (configured as digital lock detect in R2)
     * MUXOUT is on PB10 (shared with LE — we'd need a separate GPIO)
     * For now, wait a fixed time and assume lock.
     * In production, route MUXOUT to a GPIO and read it.
     */
    /* Simple delay-based lock detect: PLL typically locks in < 100 µs */
    for (volatile uint32_t i = 0; i < 5000; i++)
        ;
    return 1;  /* Assume locked after delay */
}

/* ========================================================================== */
/* Disable RF output                                                          */
/* ========================================================================== */
void adf4351_rf_output_disable(void)
{
    /* Re-write register 4 with RF output disabled */
    uint32_t r4 = (4UL << 0)
                | (0UL << ADF4351_R4_RF_DIV_POS)
                | (1UL << ADF4351_R4_FB_POS)
                | ADF4351_R4_MUX_LOGIC_CMOS
                | (3UL << ADF4351_R4_CP_THREE_POS)
                | (0UL << ADF4351_R4_CP_MODE_POS);
    /* AUX disabled, RF output disabled */
    adf4351_spi_write(r4);
}

/* ========================================================================== */
/* Power down ADF4351                                                          */
/* ========================================================================== */
void adf4351_power_down(void)
{
    /* Disable PLL chip (CE low) */
    CLR_BIT(GPIOB->ODR, PIN_PLL_CE);
}

/* ========================================================================== */
/* Set output power level                                                      */
/* ========================================================================== */
void adf4351_set_power(uint8_t power_level)
{
    /* ADF4351 output power levels: 0 = -4 dBm, 1 = -1 dBm, 2 = +2 dBm, 3 = +5 dBm */
    if (power_level > 3) power_level = 3;

    /* Re-write register 4 with new power setting */
    uint32_t r4 = (4UL << 0)
                | (0UL << ADF4351_R4_RF_DIV_POS)
                | (1UL << ADF4351_R4_FB_POS)
                | ADF4351_R4_MUX_LOGIC_CMOS
                | ((uint32_t)power_level << 3)           /* Output power level */
                | ADF4351_R4_AUX_EN
                | (2UL << ADF4351_R4_AUX_SEL_POS);
    adf4351_spi_write(r4);
}