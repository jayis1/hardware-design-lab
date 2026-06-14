/*
 * dac60508.h — DAC60508 8-Channel 12-bit DAC Driver Header
 */

#ifndef DAC60508_H
#define DAC60508_H

#include <stdint.h>

void dac60508_init(void);
void dac_set_channel(uint8_t ch, uint16_t value);
uint16_t dac_get_channel(uint8_t ch);
void dac_set_threshold(uint8_t channel, uint8_t polarity, uint16_t voltage_mv);
void dac_set_analog_offset(uint8_t channel, int16_t offset_mv);
void dac_set_gain_adjust(uint16_t gain_code);
uint16_t dac_get_device_id(void);
uint16_t dac_get_status(void);

/* DAC channel assignments */
#define DAC_CH_CMP1_THRESH_P    0
#define DAC_CH_CMP1_THRESH_N    1
#define DAC_CH_CMP2_THRESH_P    2
#define DAC_CH_CMP2_THRESH_N    3
#define DAC_CH_ANA_CAL_REF      4
#define DAC_CH_ANA_OFFSET_CH1   5
#define DAC_CH_ANA_OFFSET_CH2   6
#define DAC_CH_ANA_GAIN_ADJ     7

#endif /* DAC60508_H */