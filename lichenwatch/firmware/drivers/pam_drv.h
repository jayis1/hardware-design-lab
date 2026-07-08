/*
 * pam_drv.h — Pulse-Amplitude Modulation fluorometer driver interface.
 *
 * Author: jayis1
 * License: MIT
 */

#ifndef PAM_DRV_H
#define PAM_DRV_H

#include <stdint.h>

typedef struct {
    float f0;        /* minimal fluorescence (dark-adapted) */
    float fm;        /* maximal fluorescence (saturated)   */
    float fv_fm;     /* (fm - f0) / fm  — quantum yield     */
    uint16_t raw_f0; /* raw ADC counts */
    uint16_t raw_fm;
} pam_result_t;

int  pam_init(void);
void pam_shroud_close(void);
void pam_shroud_open(void);
int  pam_measure(pam_result_t *res);

#endif /* PAM_DRV_H */