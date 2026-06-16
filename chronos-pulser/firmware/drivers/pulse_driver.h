// pulse_driver.h — Pulse Generator Driver Header
#ifndef PULSE_DRIVER_H
#define PULSE_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t frequency_hz;
    uint8_t  amplitude_dac;
    bool     continuous;
    uint8_t  burst_count;
    bool     external_trigger;
} pulse_config_t;

int  pulse_driver_init(void);
int  pulse_configure(const pulse_config_t *config);
int  pulse_start(void);
int  pulse_stop(void);
int  pulse_single_shot(void);
int  pulse_get_status(uint32_t *pulses_emitted, bool *busy);
void pulse_set_amplitude(uint8_t dac_value);
void pulse_set_frequency(uint32_t frequency_hz);
#endif
