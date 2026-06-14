// ============================================================================
// main.c — RideCore SPL/Board Init Entry Point
// ============================================================================

#include "registers.h"
#include "board.h"
#include "drivers/pmic.h"
#include "drivers/canfd.h"
#include "drivers/spi_flash.h"
#include "drivers/ble_uart.h"

// ---- Global State ----
volatile uint32_t g_tick_ms = 0;
volatile uint8_t  g_fault_flags = FAULT_NONE;
volatile uint8_t  g_foc_running = 0;

// FOC state
typedef struct {
    float ia, ib, ic;       // Phase currents (A)
    float id, iq;            // d/q axis currents (A)
    float vd, vq;            // d/q axis voltages
    float theta;             // Electrical angle (rad)
    float omega;             // Angular velocity (rad/s)
    float id_ref, iq_ref;    // Current references
    float kp_d, ki_d;        // PI gains d-axis
    float kp_q, ki_q;        // PI gains q-axis
    float id_int, iq_int;    // PI integrators
    float vbus;              // DC bus voltage (V)
    float throttle;          // 0.0 .. 1.0
    float brake;             // 0.0 .. 1.0
} foc_state_t;

static foc_state_t foc;

// ---- SysTick Handler (1 ms tick) ----
void SysTick_Handler(void) {
    g_tick_ms++;
}

// ---- Delay (ms) ----
static void delay_ms(uint32_t ms) {
    uint32_t start = g_tick_ms;
    while ((g_tick_ms - start) < ms);
}

// ---- Clock Configuration (forward declaration, defined in clock_config.c) ----
extern void SystemClock_Config(void);
extern void Peripheral_Clock_Enable(void);

// ---- GPIO Initialization (forward declaration, defined in gpio_init.c) ----
extern void GPIO_Init(void);
extern void PWM_Output_Enable(void);

// ---- FOC Forward Declarations (defined in foc.c) ----
extern void FOC_Init(void);
extern void FOC_Run(void);
extern void TIM1_UP_IRQHandler(void);

// ---- ADC Read (simplified) ----
static uint16_t adc_read(uint8_t channel) {
    (void)channel;
    // In production: trigger ADC, wait for EOC, return DR
    // Stub: return mid-scale
    return 2048;
}

// ---- VBAT Read ----
static float read_vbat(void) {
    uint16_t raw = adc_read(ADC_CH_VBAT);
    float adc_v = (raw / ADC_RESOLUTION) * ADC_VREF_MV;
    return adc_v * VBAT_DIVIDER_RATIO / 1000.0f;  // V
}

// ---- Current Read ----
static void read_phase_currents(void) {
    uint16_t raw_a = adc_read(ADC_CH_CUR_A);
    uint16_t raw_b = adc_read(ADC_CH_CUR_B);
    uint16_t raw_c = adc_read(ADC_CH_CUR_C);
    
    foc.ia = ((float)raw_a - 2048.0f) * CURRENT_SCALE_MA_PER_LSB / 1000.0f;
    foc.ib = ((float)raw_b - 2048.0f) * CURRENT_SCALE_MA_PER_LSB / 1000.0f;
    foc.ic = ((float)raw_c - 2048.0f) * CURRENT_SCALE_MA_PER_LSB / 1000.0f;
}

// ---- Fault Check ----
static void check_faults(void) {
    float vbat = read_vbat();
    
    if (vbat > 60.0f) g_fault_flags |= FAULT_OVERVOLTAGE;
    if (vbat < 22.0f) g_fault_flags |= FAULT_UNDERVOLTAGE;
    if (DESAT_FAULT_ACTIVE()) g_fault_flags |= FAULT_DESAT;
    
    // Overcurrent check
    float max_i = foc.ia;
    if (foc.ib > max_i) max_i = foc.ib;
    if (foc.ic > max_i) max_i = foc.ic;
    if (max_i > (MOTOR_MAX_CURRENT_MA / 1000.0f)) g_fault_flags |= FAULT_OVERCURRENT;
    
    if (g_fault_flags) {
        GATE_DISABLE();
        FAULT_LED_ON();
        g_foc_running = 0;
    }
}

// ---- Heartbeat ----
static void heartbeat(void) {
    static uint32_t last_toggle = 0;
    if ((g_tick_ms - last_toggle) >= 500) {
        STATUS_LED_TOGGLE();
        last_toggle = g_tick_ms;
    }
}

// ---- Send Status over CAN ----
static void send_status_can(void) {
    static uint32_t last_tx = 0;
    if ((g_tick_ms - last_tx) >= 100) {  // 10 Hz
        canfd_frame_t frame;
        frame.id = CAN_MSG_STATUS;
        frame.ide = 0;
        frame.fdf = 0;
        frame.brs = 0;
        frame.esi = 0;
        frame.dlc = 8;
        
        // Pack status
        int16_t vbat_10mv = (int16_t)(foc.vbus * 100.0f);
        int16_t current_10ma = (int16_t)(foc.iq * 100.0f);
        frame.data[0] = vbat_10mv & 0xFF;
        frame.data[1] = (vbat_10mv >> 8) & 0xFF;
        frame.data[2] = current_10ma & 0xFF;
        frame.data[3] = (current_10ma >> 8) & 0xFF;
        frame.data[4] = g_fault_flags;
        frame.data[5] = g_foc_running;
        frame.data[6] = (uint8_t)(foc.omega * 10.0f);  // rpm approx
        frame.data[7] = (uint8_t)(foc.throttle * 255.0f);
        
        canfd_tx(&frame);
        last_tx = g_tick_ms;
    }
}

// ---- Process CAN RX ----
static void process_can_rx(void) {
    canfd_frame_t rx_frame;
    if (canfd_rx(&rx_frame) == 0) {
        switch (rx_frame.id) {
        case CAN_MSG_CMD_THROTTLE:
            if (rx_frame.dlc >= 2) {
                foc.throttle = (float)((rx_frame.data[0] | (rx_frame.data[1] << 8))) / 65535.0f;
                if (foc.throttle > 1.0f) foc.throttle = 1.0f;
            }
            break;
        case CAN_MSG_CMD_BRAKE:
            if (rx_frame.dlc >= 2) {
                foc.brake = (float)((rx_frame.data[0] | (rx_frame.data[1] << 8))) / 65535.0f;
            }
            break;
        default:
            break;
        }
    }
}

// ====================================================================
// MAIN
// ====================================================================
int main(void) {
    // 1. System clock: HSE 8 MHz → PLL → 170 MHz SYSCLK
    SystemClock_Config();
    Peripheral_Clock_Enable();

    // 2. SysTick: 1 ms @ 170 MHz
    SysTick->LOAD = (SYSCLK_HZ / 1000) - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = 0x07;  // CLKSOURCE=CPU, TICKINT=1, ENABLE=1

    // 3. GPIO init
    GPIO_Init();

    // 4. PMIC init (power rails)
    pmic_init();
    pmic_enable_rail(PMIC_RAIL_DCDC1);   // 5V
    delay_ms(10);
    pmic_enable_rail(PMIC_RAIL_LDO1);    // 3.3V
    delay_ms(10);
    pmic_enable_rail(PMIC_RAIL_LDO2);    // 1.8V
    delay_ms(10);

    // 5. SPI flash init
    spi_flash_init();
    
    // 6. Load config from SPI flash
    foc.kp_d = 0.5f; foc.ki_d = 0.01f;
    foc.kp_q = 0.5f; foc.ki_q = 0.01f;
    foc.id_ref = 0.0f;
    foc.iq_ref = 0.0f;
    // In production: spi_flash_read_config(&foc);

    // 7. CAN FD init
    canfd_init();

    // 8. BLE init
    ble_uart_init();

    // 9. FOC init
    FOC_Init();

    // 10. Enable gate drivers (with safety delay)
    delay_ms(50);
    GATE_ENABLE();
    delay_ms(10);

    // 11. Enable PWM outputs
    PWM_Output_Enable();

    // 12. Enable TIM1 (starts FOC ISR)
    TIM1->CR1 |= TIM_CR1_CEN;
    TIM1->BDTR |= TIM_BDTR_MOE;
    g_foc_running = 1;

    // 13. Send CAN "READY" message
    {
        canfd_frame_t ready_frame = {0};
        ready_frame.id = CAN_MSG_READY;
        ready_frame.dlc = 1;
        ready_frame.data[0] = 0x01;
        canfd_tx(&ready_frame);
    }

    // 14. Main loop
    while (1) {
        read_phase_currents();
        foc.vbus = read_vbat();
        check_faults();
        process_can_rx();
        send_status_can();
        ble_uart_process();
        heartbeat();
        
        // WFI to save power when idle
        __asm volatile("wfi");
    }
}

// ---- FOC ISR (TIM1 Update) ----
void TIM1_UP_IRQHandler(void) {
    if (TIM1->SR & TIM_SR_UIF) {
        TIM1->SR &= ~TIM_SR_UIF;
        
        if (g_foc_running && !g_fault_flags) {
            FOC_Run();
        }
    }
}