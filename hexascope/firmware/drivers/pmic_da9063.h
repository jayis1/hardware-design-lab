/*
 * pmic_da9063.h — DA9063 PMIC Driver Header
 */

#ifndef PMIC_DA9063_H
#define PMIC_DA9063_H

#include <stdint.h>

void pmic_init(void);
void pmic_enable_rails(void);
uint8_t pmic_read_reg(uint8_t reg);
void pmic_write_reg(uint8_t reg, uint8_t val);
uint8_t pmic_get_fault_status(void);
void pmic_disable_rail(uint8_t rail_reg);
void pmic_enable_rail(uint8_t rail_reg);

/* Rail register addresses */
#define PMIC_RAIL_VDD_CORE     0x20U  /* BUCK1 */
#define PMIC_RAIL_VDD_DDR      0x21U  /* BUCK2 */
#define PMIC_RAIL_VDD_IO18     0x22U  /* BUCK3 */
#define PMIC_RAIL_VDD_IO33     0x30U  /* LDO1 */
#define PMIC_RAIL_VDD_ANA      0x31U  /* LDO2 */
#define PMIC_RAIL_VDD_RTC      0x32U  /* LDO3 */

#endif /* PMIC_DA9063_H */