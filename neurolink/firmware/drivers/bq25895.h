/*
 * NeuroLink — BQ25895 Battery Charger / Power Path Management Driver
 * I2C2 interface (7-bit address: 0x6A when PRO pin = LOW)
 */

#ifndef BQ25895_H
#define BQ25895_H

#include <stdint.h>

/* BQ25895 I2C Address (7-bit) */
#define BQ25895_I2C_ADDR        0x6A

/* Register Map */
#define BQ25895_REG_00          0x00  /* ILLIM_VINDPM: Input current limit, VINDPM threshold */
#define BQ25895_REG_01          0x01  /* Input current limit for ICO */
#define BQ25895_REG_02          0x02  /* Input voltage limit for ICO */
#define BQ25895_REG_03          0x03  /* WD_RST, CHG_CONFIG, SYS_MIN, MIN_VBAT_SEL */
#define BQ25895_REG_04          0x04  /* ICHG: Fast charge current limit */
#define BQ25895_REG_05          0x05  /* IPRECHG, ITERM: Pre-charge & termination current */
#define BQ25895_REG_06          0x06  /* VREG: Regulation voltage */
#define BQ25895_REG_07          0x07  /* TIMER, BATLOWV, VRECHG: Safety timer, bat low threshold */
#define BQ25895_REG_08          0x08  /* IR_COMP, VCLAMP: IR compensation, thermal regulation */
#define BQ25895_REG_09          0x09  /* REG_RST, ICO_EN, STAT_DIS, WD */
#define BQ25895_REG_0A          0x0A  /* BOOST_REG, BOOST_LIM: Boost mode voltage & current */
#define BQ25895_REG_0B          0x0B  /* VINDPM: Input voltage limit offset */
#define BQ25895_REG_0C          0x0C  /* TERM_ENABLE, ICOEN, CHG_EN */
#define BQ25895_REG_0D          0x0D  /* FORCE_DPDM, TMR2X_EN, BATFET_DIS */
#define BQ25895_REG_0E          0x0E  /* CHG_STATUS: Charge status */
#define BQ25895_REG_0F          0x0F  /* FAULT: Fault status */
#define BQ25895_REG_10          0x10  /* VBUS_STATUS: VBUS detection */
#define BQ25895_REG_11          0x11  /* IINDET_EN, TMR2X_EN */
#define BQ25895_REG_12          0x12  /* ICO status */
#define BQ25895_REG_13          0x13  /* ADC control */
#define BQ25895_REG_14          0x14  /* Battery voltage ADC */
#define BQ25895_REG_15          0x15  /* System voltage ADC */
#define BQ25895_REG_16          0x16  /* TS ADC */
#define BQ25895_REG_17          0x17  /* VBUS voltage ADC */
#define BQ25895_REG_18          0x18  /* IBUS current ADC */
#define BQ25895_REG_19          0x19  /* Die temperature ADC */

/* Charge Status values (REG_0E bits 5:3) */
typedef enum {
    BQ25895_CHG_NOT_CHARGING   = 0,
    BQ25895_CHG_PRECHARGE      = 1,
    BQ25895_CHG_FAST_CHARGE    = 2,
    BQ25895_CHG_CHARGE_DONE    = 3,
} bq25895_chg_status_t;

/* Fault register flags (REG_0F) */
#define BQ25895_FAULT_VBUS_OVP      (1 << 7)
#define BQ25895_FAULT_VBUS_OCP      (1 << 6)
#define BQ25895_FAULT_BAT_OVP       (1 << 5)
#define BQ25895_FAULT_CHG_THERM     (1 << 4)
#define BQ25895_FAULT_NTC_COLD       (1 << 3)
#define BQ25895_FAULT_NTC_HOT        (1 << 2)
#define BQ25895_FAULT_WATCHDOG       (1 << 1)
#define BQ25895_FAULT_CHG_FAULT      (1 << 0)

/* Public API */
void     bq25895_init(void);
uint8_t  bq25895_read_reg(uint8_t reg);
void     bq25895_write_reg(uint8_t reg, uint8_t val);
uint8_t  bq25895_get_charging_status(void);
uint8_t  bq25895_get_fault_status(void);
uint16_t bq25895_read_battery_voltage(void);
uint16_t bq25895_read_vbus_voltage(void);
uint16_t bq25895_read_system_voltage(void);
int16_t  bq25895_read_ibus_current(void);
int16_t  bq25895_read_die_temp(void);
void     bq25895_set_charge_current(uint16_t current_ma);
void     bq25895_set_charge_voltage(uint16_t voltage_mv);
void     bq25895_set_input_current_limit(uint16_t current_ma);
void     bq25895_enable_charging(uint8_t enable);
void     bq25895_enable_otg(uint8_t enable);
uint8_t  bq25895_is_vbus_present(void);
uint8_t  bq25895_is_ico_complete(void);

#endif /* BQ25895_H */