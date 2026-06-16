// clock_config.h — Clock Configuration Header
#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H
#include <stdint.h>

typedef enum {
    CLOCK_SYS = 0,
    CLOCK_ADC,
    CLOCK_TDC,
    CLOCK_USB,
    CLOCK_DDR3,
    CLOCK_DOMAIN_COUNT
} clock_domain_id_t;

int      clock_init(void);
uint32_t clock_get_frequency(clock_domain_id_t domain);
int      clock_domain_disable(clock_domain_id_t domain);
int      clock_domain_enable(clock_domain_id_t domain);
#endif
