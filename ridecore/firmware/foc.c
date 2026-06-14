// ============================================================================
// foc.c — Field-Oriented Control for PMSM (STM32G474)
// Sensorless FOC with SVPWM, 20 kHz control rate
// ============================================================================

#include "foc.h"
#include "registers.h"
#include "board.h"
#include <math.h>

// ---- Math constants ----
#define SQRT3      1.7320508f
#define TWO_PI     6.2831853f
#define PI_2       1.5707963f
#define ONE_BY_SQRT3  0.5773503f

// ---- SVPWM sector determination ----
static uint8_t svpwm_sector(float alpha, float beta) {
    // Determine sector from alpha-beta angle
    float angle = atan2f(beta, alpha);
    if (angle < 0) angle += TWO_PI;
    return (uint8_t)(angle / (PI_2 / 3.0f)) + 1;  // Sector 1-6
}

// ---- Clarke Transform (3-phase → 2-phase) ----
// ia, ib, ic → alpha, beta
static void clarke(float ia, float ib, float ic, float *alpha, float *beta) {
    *alpha = ia;
    *beta = (ia + 2.0f * ib) / SQRT3;
    (void)ic;  // ia + ib + ic = 0 for balanced
}

// ---- Park Transform (alpha-beta → d-q) ----
static void park(float alpha, float beta, float theta, float *d, float *q) {
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    *d = alpha * cos_t + beta * sin_t;
    *q = -alpha * sin_t + beta * cos_t;
}

// ---- Inverse Park Transform (d-q → alpha-beta) ----
static void inv_park(float d, float q, float theta, float *alpha, float *beta) {
    float cos_t = cosf(theta);
    float sin_t = sinf(theta);
    *alpha = d * cos_t - q * sin_t;
    *beta = d * sin_t + q * cos_t;
}

// ---- SVPWM (Space Vector PWM) ----
// Computes TIM1 CCR1/2/3 duty cycles from alpha-beta voltages
static void svpwm(float valpha, float vbeta, float vbus, uint16_t *ccr1, uint16_t *ccr2, uint16_t *ccr3) {
    // Normalize to modulation index
    float inv_vbus = 1.0f / vbus;
    float va = valpha * inv_vbus;
    float vb = vbeta * inv_vbus;
    
    // Sector determination
    uint8_t sector = svpwm_sector(va, vb);
    
    // Calculate X, Y, Z
    float x = vb * SQRT3;
    float y = va * 0.5f + vb * ONE_BY_SQRT3;
    float z = -va * 0.5f + vb * ONE_BY_SQRT3;
    
    float t1, t2;
    switch (sector) {
    case 1: t1 = z; t2 = y;  break;
    case 2: t1 = y;  t2 = -x; break;
    case 3: t1 = -z; t2 = x; break;
    case 4: t1 = -y; t2 = -z; break;
    case 5: t1 = x;  t2 = z;  break;
    case 6: t1 = -x; t2 = y; break;
    default: t1 = 0; t2 = 0; break;
    }
    
    // Clamp to avoid overmodulation
    float tsum = t1 + t2;
    if (tsum > 1.0f) {
        t1 /= tsum;
        t2 /= tsum;
    }
    
    float t0 = 1.0f - t1 - t2;  // Zero vector time
    
    // Compute duty cycles (center-aligned: split t0/2 at each end)
    float ta, tb, tc;
    switch (sector) {
    case 1: ta = t1 + t2 + t0/2; tb = t2 + t0/2;      tc = t0/2; break;
    case 2: ta = t1 + t0/2;      tb = t1 + t2 + t0/2;  tc = t0/2; break;
    case 3: ta = t0/2;           tb = t1 + t2 + t0/2;  tc = t2 + t0/2; break;
    case 4: ta = t0/2;           tb = t1 + t0/2;      tc = t1 + t2 + t0/2; break;
    case 5: ta = t2 + t0/2;      tb = t0/2;            tc = t1 + t2 + t0/2; break;
    case 6: ta = t1 + t2 + t0/2; tb = t0/2;            tc = t1 + t0/2; break;
    default: ta = tb = tc = 0.5f; break;
    }
    
    // Convert to TIM1 CCR values
    *ccr1 = (uint16_t)(ta * TIM1_ARR_VALUE);
    *ccr2 = (uint16_t)(tb * TIM1_ARR_VALUE);
    *ccr3 = (uint16_t)(tc * TIM1_ARR_VALUE);
}

// ---- PI Controller ----
static float pi_control(float error, float kp, float ki, float *integrator, float limit) {
    float p_term = kp * error;
    *integrator += ki * error;
    
    // Anti-windup: clamp integrator
    if (*integrator > limit) *integrator = limit;
    if (*integrator < -limit) *integrator = -limit;
    
    return p_term + *integrator;
}

// ---- Sensorless Observer (PLL-based) ----
// Simplified: in production, use full sliding-mode or extended EMF observer
static float sensorless_observer(float ia, float ib, float va, float vb, float omega_est) {
    (void)ia; (void)ib; (void)va; (void)vb; (void)omega_est;
    // Stub: return open-loop ramp angle
    // Production: implement EEMF or SMO observer
    return 0.0f;
}

// ---- FOC Init ----
void FOC_Init(void) {
    // Configure TIM1 for center-aligned PWM mode 1
    TIM1->PSC = 0;                         // No prescaler
    TIM1->ARR = TIM1_ARR_VALUE;            // 4250 for 20 kHz @ 170 MHz
    TIM1->CR1 = TIM_CR1_CMS_CENTER1 |      // Center-aligned mode 1
                TIM_CR1_ARPE;               // Auto-reload preload
    TIM1->CR2 = (1 << 3);                  // CCPC: COM event preload
    
    // Dead time: DTG = 85 → 500 ns @ 170 MHz
    TIM1->BDTR = (TIM1_DTG_VALUE & 0xFF);  // Dead time, MOE set later
    
    // PWM mode 1 on CH1, CH2, CH3
    TIM1->CCMR1 = TIM_CCMR1_OC1M_PWM1 | TIM_CCMR1_OC1PE |
                  TIM_CCMR1_OC2M_PWM1 | TIM_CCMR1_OC2PE;
    TIM1->CCMR2 = TIM_CCMR2_OC3M_PWM1 | TIM_CCMR2_OC3PE;
    
    // Initial duty: 50% (all phases at half)
    TIM1->CCR1 = TIM1_ARR_VALUE / 2;
    TIM1->CCR2 = TIM1_ARR_VALUE / 2;
    TIM1->CCR3 = TIM1_ARR_VALUE / 2;
    
    // Enable update interrupt for FOC ISR
    TIM1->DIER = TIM_DIER_UIE;
    
    // Enable TIM1 UP interrupt in NVIC (highest priority)
    NVIC_EnableIRQ(TIM1_UP_IRQn);
    NVIC_SetPriority(TIM1_UP_IRQn, 0);  // Highest priority
}

// ---- FOC Run (called from TIM1 UP ISR at 20 kHz) ----
// Uses external foc state from main.c
extern float g_ia, g_ib, g_ic;  // Set by ADC read in main loop
extern float g_vbus;

static float g_theta = 0.0f;
static float g_omega_est = 0.0f;
static float g_id_int = 0.0f, g_iq_int = 0.0f;

void FOC_Run(void) {
    // 1. Read phase currents (already captured by ADC, triggered by TIM1)
    float ia = g_ia;
    float ib = g_ib;
    float ic = g_ic;
    float vbus = g_vbus;
    
    if (vbus < 1.0f) vbus = 1.0f;  // Prevent div-by-zero
    
    // 2. Clarke transform
    float alpha, beta;
    clarke(ia, ib, ic, &alpha, &beta);
    
    // 3. Park transform (using estimated angle)
    float id, iq;
    park(alpha, beta, g_theta, &id, &iq);
    
    // 4. Current references
    // id_ref = 0 (maximum torque per ampere for surface PM)
    // iq_ref = throttle * max_current
    float id_ref = 0.0f;
    float iq_ref = 0.0f;  // Set by throttle command
    
    // 5. PI controllers
    float vd = pi_control(id_ref - id, 0.5f, 0.01f, &g_id_int, vbus * 0.5f);
    float vq = pi_control(iq_ref - iq, 0.5f, 0.01f, &g_iq_int, vbus * 0.5f);
    
    // 6. Inverse Park transform
    float valpha, vbeta;
    inv_park(vd, vq, g_theta, &valpha, &vbeta);
    
    // 7. SVPWM
    uint16_t ccr1, ccr2, ccr3;
    svpwm(valpha, vbeta, vbus, &ccr1, &ccr2, &ccr3);
    
    // 8. Update PWM duty cycles
    TIM1->CCR1 = ccr1;
    TIM1->CCR2 = ccr2;
    TIM1->CCR3 = ccr3;
    
    // 9. Update angle (open-loop for now, sensorless observer in production)
    g_omega_est = 0.0f;  // From sensorless observer
    g_theta += g_omega_est * (1.0f / MOTOR_PWM_FREQ_HZ);
    if (g_theta > TWO_PI) g_theta -= TWO_PI;
    if (g_theta < 0) g_theta += TWO_PI;
}