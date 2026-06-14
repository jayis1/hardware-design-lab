/*
 * adf4351.h — ADF4351 PLL Synthesizer driver (SPI bit-bang)
 * 35 MHz – 4.4 GHz (up to 6 GHz with ADF5002 prescaler)
 */

#ifndef VORTEX_SDR_ADF4351_H
#define VORTEX_SDR_ADF4351_H

#include <stdint.h>

/* ADF4351 register definitions */
#define ADF4351_REG0         0
#define ADF4351_REG1         1
#define ADF4351_REG2         2
#define ADF4351_REG3         3
#define ADF4351_REG4         4
#define ADF4351_REG5         5

/* Register 0 bit fields */
#define ADF4351_R0_FRAC_POS     3
#define ADF4351_R0_INT_POS      15
#define ADF4351_R0_AUTOCAL      (1UL << 2)
#define ADF4351_R0_CP_GAIN_0    (0UL << 3)   /* 0 = no gain */
#define ADF4351_R0_CP_GAIN_1    (1UL << 3)   /* CP gain bit */

/* Register 1 bit fields */
#define ADF4351_R1_FRAC_POS     3
#define ADF4351_R1_INT_POS      15
#define ADF4351_R1_MOD_POS      3
#define ADF4351_R1_PHASE_POS    15
#define ADF4351_R1_PRESCALER_8_9  (1UL << 27) /* 8/9 prescaler */

/* Register 2 bit fields */
#define ADF4351_R2_R_POS        3
#define ADF4351_R2_CP_POS       12
#define ADF4351_R2_LDP_10ns     (0UL << 10)
#define ADF4351_R2_LDP_6ns      (1UL << 10)
#define ADF4351_R2_PD_POL_NEG   (0UL << 11)
#define ADF4351_R2_PD_POL_POS   (1UL << 11)
#define ADF4351_R2_MUX_THREE     (1UL << 28)  /* MUX = digital lock detect */
#define ADF4351_R2_LD_PIN_MODE  (3UL << 9)   /* Normal lock detect */

/* Register 3 bit fields */
#define ADF4351_R3_CSR          (1UL << 18)   /* Cycle slip reduction */
#define ADF4351_R3_DIV_SEL_POS  22
#define ADF4351_R3_CLK_DIV_POS  3
#define ADF4351_R3_CLK_DIV_MODE_POS  15

/* Register 4 bit fields */
#define ADF4351_R4_RF_DIV_POS   20
#define ADF4351_R4_FB_POS       23
#define ADF4351_R4_AUX_SEL_POS  6
#define ADF4351_R4_AUX_EN       (1UL << 9)
#define ADF4351_R4_B_SEL        (1UL << 8)
#define ADF4351_R4_MUX_LOGIC_CMOS (0UL << 2)
#define ADF4351_R4_MUX_LOGIC_OD   (1UL << 2)
#define ADF4351_R4_CP_THREE_POS   8
#define ADF4351_R4_CP_MODE_POS    11

/* Register 5 bit fields */
#define ADF4351_R5_LD_PIN_POS   18
#define ADF4351_R5_LD_PIN_DLD   (2UL << 18)  /* Digital lock detect */

/* RF output divider values */
#define ADF4351_RF_DIV_1     0
#define ADF4351_RF_DIV_2     1
#define ADF4351_RF_DIV_4     2
#define ADF4351_RF_DIV_8     3
#define ADF4351_RF_DIV_16    4
#define ADF4351_RF_DIV_32    5
#define ADF4351_RF_DIV_64    6

/* Reference frequency */
#define ADF4351_REF_FREQ_HZ   25000000UL   /* 25 MHz TCXO reference */

/* Function prototypes */
void adf4351_init(void);
void adf4351_set_frequency(uint64_t freq_hz);
uint8_t adf4351_lock_detect(void);
void adf4351_rf_output_disable(void);
void adf4351_power_down(void);
void adf4351_set_power(uint8_t power_level);

#endif /* VORTEX_SDR_ADF4351_H */