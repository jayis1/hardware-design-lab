/*
 * pmic.h - STPMIC1APQR Power Management IC Driver Header
 * CyberGuard Hardware Security Token
 */

#ifndef PMIC_H
#define PMIC_H

#include <stdint.h>

/* STPMIC1 I2C address (7-bit) */
#define STPMIC1_ADDR          0x08

/* STPMIC1 Register Map */
#define STPMIC1_REG_MAIN_CTRL   0x10
#define STPMIC1_REG_PONKEY     0x11
#define STPMIC1_REG_BUCK_CTRL   0x12
#define STPMIC1_REG_BUCK_OUT_MR 0x13
#define STPMIC1_REG_BUCK_OUT_VR 0x14
#define STPMIC1_REG_LDO1_CTRL   0x20
#define STPMIC1_REG_LDO1_OUT_MR 0x21
#define STPMIC1_REG_LDO1_OUT_VR 0x22
#define STPMIC1_REG_LDO2_CTRL   0x23
#define STPMIC1_REG_LDO2_OUT_MR 0x24
#define STPMIC1_REG_LDO2_OUT_VR 0x25
#define STPMIC1_REG_LDO3_CTRL   0x26
#define STPMIC1_REG_LDO3_OUT_MR 0x27
#define STPMIC1_REG_LDO3_OUT_VR 0x28
#define STPMIC1_REG_LDO4_CTRL   0x29
#define STPMIC1_REG_LDO4_OUT_MR 0x2A
#define STPMIC1_REG_LDO4_OUT_VR 0x2B
#define STPMIC1_REG_VREF_DDR    0x2C
#define STPMIC1_REG_PWRCTRL     0x30
#define STPMIC1_REG_INT_STATUS  0x40
#define STPMIC1_REG_INT_CLEAR   0x41
#define STPMIC1_REG_INT_MASK    0x42
#define STPMIC1_REG_INT_RISE    0x43
#define STPMIC1_REG_INT_FALL    0x44

/* STPMIC1 Control Bits */
#define STPMIC1_BUCK_EN         (1U << 0)
#define STPMIC1_BUCK_LP         (1U << 1)  /* Low power mode */
#define STPMIC1_BUCK_VOUT_SEL   (1U << 0)  /* 0=VR, 1=MR */

#define STPMIC1_LDO1_EN         (1U << 0)
#define STPMIC1_LDO1_LP         (1U << 1)

#define STPMIC1_LDO2_EN         (1U << 0)
#define STPMIC1_LDO2_LP         (1U << 1)

/* Voltage settings (mV) */
#define STPMIC1_BUCK_3V3        0x1E  /* 3.3V */
#define STPMIC1_LDO1_1V8        0x0C  /* 1.8V */
#define STPMIC1_LDO2_1V1        0x02  /* 1.1V */

/* Interrupt sources */
#define STPMIC1_INT_VBUS_OTG    (1U << 0)
#define STPMIC1_INT_SWOUT       (1U << 1)
#define STPMIC1_INT_SWIN        (1U << 2)
#define STPMIC1_INT_VBUS_TH     (1U << 3)
#define STPMIC1_INT_VBUS_ULVO   (1U << 4)
#define STPMIC1_INT_VBUS_OVP    (1U << 5)
#define STPMIC1_INT_VBUS_DISCONNECT (1U << 6)
#define STPMIC1_INT_VBUS_CONNECT   (1U << 7)

/**
 * Initialize STPMIC1 and enable all power rails
 * @return 0 on success, negative on error
 */
int pmic_init(void);

/**
 * Enable buck converter (3.3V rail)
 * @return 0 on success, negative on error
 */
int pmic_enable_buck(void);

/**
 * Enable LDO1 (1.8V rail)
 * @return 0 on success, negative on error
 */
int pmic_enable_ldo1(void);

/**
 * Enable LDO2 (1.1V rail)
 * @return 0 on success, negative on error
 */
int pmic_enable_ldo2(void);

/**
 * Disable buck converter
 * @return 0 on success, negative on error
 */
int pmic_disable_buck(void);

/**
 * Disable LDO1
 * @return 0 on success, negative on error
 */
int pmic_disable_ldo1(void);

/**
 * Disable LDO2
 * @return 0 on success, negative on error
 */
int pmic_disable_ldo2(void);

/**
 * Enter low-power mode (disable all non-essential rails)
 * @return 0 on success, negative on error
 */
int pmic_enter_lowpower(void);

/**
 * Exit low-power mode (re-enable all rails)
 * @return 0 on success, negative on error
 */
int pmic_exit_lowpower(void);

/**
 * Check VBUS status (USB connected?)
 * @return 1 if VBUS present, 0 if not, negative on error
 */
int pmic_vbus_detected(void);

/**
 * Read PMIC register
 * @param reg Register address
 * @param val Pointer to store value
 * @return 0 on success, negative on error
 */
int pmic_read_reg(uint8_t reg, uint8_t *val);

/**
 * Write PMIC register
 * @param reg Register address
 * @param val Value to write
 * @return 0 on success, negative on error
 */
int pmic_write_reg(uint8_t reg, uint8_t val);

#endif /* PMIC_H */