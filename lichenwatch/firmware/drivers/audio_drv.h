/*
 * audio_drv.h — Contact acoustic desiccation event detector (I²S MEMS mic).
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef AUDIO_DRV_H
#define AUDIO_DRV_H

#include <stdint.h>

typedef struct {
    uint16_t click_count;   /* number of desiccation click events detected */
    uint16_t peak_energy;   /* peak signal energy during capture */
} audio_result_t;

int audio_init(void);
int audio_capture(audio_result_t *res);

#endif /* AUDIO_DRV_H */