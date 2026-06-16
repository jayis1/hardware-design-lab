/**
 * main.c — PhotonWeave STM32H743 System Controller Firmware
 *
 * Main firmware for the STM32H743VIT6 system controller on the
 * PhotonWeave CGH Engine PCIe card.
 *
 * Responsibilities:
 *   - Power sequencing verification
 *   - Si5345 clock generator configuration
 *   - FPGA bitstream programming via QSPI
 *   - System health monitoring (temperature, voltage, clocks)
 *   - I2C bus master for PMIC, sensors, EEPROM
 *   - SPI communication with FPGA
 *   - USB command processing via FT601
 *   - LED status indication
 *   - Watchdog management
 *   - Thermal throttling coordination
 */

#include "stm32h743xx.h"
#include "board.h"
#include "registers.h"
#include <string.h>
#include <stdio.h>

//=============================================================================
// Global State
//=============================================================================
static system_state_t g_system_state = SYS_STATE_BOOT;
static uint32_t g_uptime_ms = 0;
static volatile uint32_t g_systick_counter = 0;

// Peripheral handles
static I2C_HandleTypeDef   hi2c1;
static SPI_HandleTypeDef   hspi1;
static UART_HandleTypeDef  huart4;
static TIM_HandleTypeDef   htim2;  // 1 kHz system tick

// Forward declarations
static void SystemClock_Config(void);
static void SystemTick_Config(void);
static void I2C1_Init(void);
static void SPI1_Init(void);
static void UART4_Init(void);
static void TIM2_Init(void);
static void NVIC_Config(void);
static void Error_Handler(void);

// Driver forward declarations (implemented in separate files)
extern int  si5345_configure(I2C_HandleTypeDef *hi2c);
extern int  fpga_program(void);
extern int  board_eeprom_read(uint8_t *serial, uint8_t *mac, uint16_t *revision);
extern int  tmp117_init_all(I2C_HandleTypeDef *hi2c);
extern int  tmp117_read_all(I2C_HandleTypeDef *hi2c, int32_t temps[4]);
extern void thermal_manager_update(int32_t temps[4]);
extern int  usb_command_process(void);
extern void debug_uart_printf(const char *fmt, ...);

//=============================================================================
// Main Entry Point
//=============================================================================
int main(void)
{
    // 1. System initialization
    SystemClock_Config();
    SystemTick_Config();
    NVIC_Config();

    // Initialize peripherals
    I2C1_Init();
    SPI1_Init();
    UART4_Init();
    TIM2_Init();

    debug_uart_printf("\r\n\r\n=== PhotonWeave STM32H743 System Controller ===\r\n");
    debug_uart_printf("Firmware Version: 1.0.0\r\n");
    debug_uart_printf("SYSCLK: %lu MHz\r\n", SystemCoreClock / 1000000);

    // 2. Read board identity from EEPROM
    uint8_t serial_bytes[4];
    uint8_t mac_bytes[6];
    uint16_t board_revision;

    if (board_eeprom_read(serial_bytes, mac_bytes, &board_revision) == 0) {
        uint32_t serial = (serial_bytes[0] << 24) | (serial_bytes[1] << 16) |
                          (serial_bytes[2] << 8)  | serial_bytes[3];
        debug_uart_printf("Board Rev: %d.%d, Serial: %08lX\r\n",
                          (board_revision >> 8) & 0xFF, board_revision & 0xFF, serial);
        debug_uart_printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                          mac_bytes[0], mac_bytes[1], mac_bytes[2],
                          mac_bytes[3], mac_bytes[4], mac_bytes[5]);
    } else {
        debug_uart_printf("WARNING: EEPROM read failed — using defaults\r\n");
    }

    // 3. Configure Si5345 clock generator
    g_system_state = SYS_STATE_CLOCK_INIT;
    debug_uart_printf("Configuring Si5345 clock generator...\r\n");

    int clk_result = si5345_configure(&hi2c1);
    if (clk_result != 0) {
        debug_uart_printf("ERROR: Si5345 configuration failed (code %d)\r\n", clk_result);
        g_system_state = SYS_STATE_ERROR;
        board_error_indicate();
        while (1) {
            board_heartbeat_update(); // Still blink heartbeat
        }
    }
    debug_uart_printf("Si5345 configured successfully, all outputs locked\r\n");

    // 4. Initialize temperature sensors
    if (tmp117_init_all(&hi2c1) != 0) {
        debug_uart_printf("WARNING: TMP117 initialization partial failure\r\n");
    }

    // 5. Program FPGA
    g_system_state = SYS_STATE_FPGA_PROG;
    debug_uart_printf("Programming FPGA from QSPI flash...\r\n");

    int fpga_result = fpga_program();
    if (fpga_result != 0) {
        debug_uart_printf("ERROR: FPGA programming failed (code %d)\r\n", fpga_result);
        g_system_state = SYS_STATE_ERROR;
        board_error_indicate();
        while (1) {
            board_heartbeat_update();
        }
    }
    debug_uart_printf("FPGA configuration complete, DONE asserted\r\n");

    // 6. Wait for FPGA internal initialization
    g_system_state = SYS_STATE_FPGA_CAL;
    debug_uart_printf("Waiting for FPGA internal calibration...\r\n");

    // Poll FPGA status over SPI
    uint32_t fpga_status = 0;
    uint32_t cal_timeout = 0;
    const uint32_t CAL_TIMEOUT_MS = 10000; // 10 seconds max

    while (cal_timeout < CAL_TIMEOUT_MS) {
        // Read FPGA status via SPI
        uint8_t tx_data[4] = {SPI_CMD_STATUS_REQ, 0, 0, 0};
        uint8_t rx_data[4] = {0};

        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
        HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 4, 100);
        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

        fpga_status = (rx_data[1] << 16) | (rx_data[2] << 8) | rx_data[3];

        if (fpga_status & PW_STATUS_FPGA_READY) {
            debug_uart_printf("FPGA ready: PLLs locked, DDR4 calibrated, PCIe link up\r\n");
            break;
        }

        board_delay_ms(100);
        cal_timeout += 100;
    }

    if (!(fpga_status & PW_STATUS_FPGA_READY)) {
        debug_uart_printf("ERROR: FPGA initialization timeout\r\n");
        debug_uart_printf("FPGA status: 0x%08lX\r\n", fpga_status);
        g_system_state = SYS_STATE_ERROR;
        board_error_indicate();
        while (1) {
            board_heartbeat_update();
        }
    }

    // 7. System ready
    g_system_state = SYS_STATE_READY;
    debug_uart_printf("=== PhotonWeave System READY ===\r\n");
    debug_uart_printf("DDR4: Calibrated, PCIe: L0, CGH: Idle\r\n");

    // Turn on USB ready LED
    HAL_GPIO_WritePin(LED_USB_READY_PORT, LED_USB_READY_PIN, GPIO_PIN_SET);

    //=========================================================================
    // Main Loop — 1 kHz system tick
    //=========================================================================
    uint32_t last_watchdog_kick = 0;
    uint32_t last_temp_read = 0;
    uint32_t last_status_log = 0;
    int32_t temperatures[4] = {0};

    debug_uart_printf("Entering main loop...\r\n");

    while (1) {
        uint32_t now = g_uptime_ms;

        // ── Watchdog kick (every 500ms) ──
        if ((now - last_watchdog_kick) >= WATCHDOG_KICK_INTERVAL_MS) {
            last_watchdog_kick = now;
            // Kick FPGA watchdog via SPI
            uint8_t kick_cmd[4] = {SPI_CMD_REG_WRITE, 0x01, 0x2C, 0x00};
            uint8_t kick_resp[4];
            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, kick_cmd, kick_resp, 4, 10);
            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
        }

        // ── Temperature monitoring (every 1000ms) ──
        if ((now - last_temp_read) >= 1000) {
            last_temp_read = now;
            if (tmp117_read_all(&hi2c1, temperatures) == 0) {
                thermal_manager_update(temperatures);

                // Check for thermal critical
                for (int i = 0; i < 4; i++) {
                    if (temperatures[i] > TEMP_CRITICAL_MILLIC) {
                        debug_uart_printf("THERMAL CRITICAL: Sensor %d = %ld.%03ld C\r\n",
                                          i, temperatures[i] / 1000,
                                          temperatures[i] % 1000);
                        g_system_state = SYS_STATE_SHUTDOWN;
                        // Send throttle command to FPGA
                        uint8_t throttle_cmd[4] = {SPI_CMD_CLOCK_THROTTLE, 0xFF, 0, 0};
                        uint8_t throttle_resp[4];
                        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
                        HAL_SPI_TransmitReceive(&hspi1, throttle_cmd, throttle_resp, 4, 10);
                        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
                    } else if (temperatures[i] > TEMP_WARNING_MILLIC) {
                        if (g_system_state == SYS_STATE_READY) {
                            g_system_state = SYS_STATE_THERMAL_THROT;
                            debug_uart_printf("THERMAL WARNING: Sensor %d = %ld.%03ld C\r\n",
                                              i, temperatures[i] / 1000,
                                              temperatures[i] % 1000);
                            // Mild throttle
                            uint8_t throttle_cmd[4] = {SPI_CMD_CLOCK_THROTTLE, 0x01, 0, 0};
                            uint8_t throttle_resp[4];
                            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
                            HAL_SPI_TransmitReceive(&hspi1, throttle_cmd, throttle_resp, 4, 10);
                            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
                        }
                    }
                }

                // If all temps back below warning, return to ready
                if (g_system_state == SYS_STATE_THERMAL_THROT) {
                    bool all_ok = true;
                    for (int i = 0; i < 4; i++) {
                        if (temperatures[i] > (TEMP_WARNING_MILLIC - TEMP_HYSTERESIS_MILLIC)) {
                            all_ok = false;
                            break;
                        }
                    }
                    if (all_ok) {
                        g_system_state = SYS_STATE_READY;
                        debug_uart_printf("Thermal condition cleared, returning to READY\r\n");
                        uint8_t unthrottle_cmd[4] = {SPI_CMD_CLOCK_THROTTLE, 0x00, 0, 0};
                        uint8_t unthrottle_resp[4];
                        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
                        HAL_SPI_TransmitReceive(&hspi1, unthrottle_cmd, unthrottle_resp, 4, 10);
                        HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);
                    }
                }
            }
        }

        // ── Status logging (every 5000ms) ──
        if ((now - last_status_log) >= 5000) {
            last_status_log = now;
            debug_uart_printf("[%lu] State: %s, Temps: %ld.%ld %ld.%ld %ld.%ld %ld.%ld C\r\n",
                              now / 1000,
                              board_state_string(g_system_state),
                              temperatures[0] / 1000, (temperatures[0] % 1000) / 100,
                              temperatures[1] / 1000, (temperatures[1] % 1000) / 100,
                              temperatures[2] / 1000, (temperatures[2] % 1000) / 100,
                              temperatures[3] / 1000, (temperatures[3] % 1000) / 100);
        }

        // ── USB command processing ──
        usb_command_process();

        // ── Heartbeat LED ──
        board_heartbeat_update();

        // ── Check FPGA interrupt line ──
        if (HAL_GPIO_ReadPin(FPGA_INT_PORT, FPGA_INT_PIN) == GPIO_PIN_RESET) {
            // FPGA attention — read status over SPI
            uint8_t int_cmd[4] = {SPI_CMD_STATUS_REQ, 0, 0, 0};
            uint8_t int_resp[4];
            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_RESET);
            HAL_SPI_TransmitReceive(&hspi1, int_cmd, int_resp, 4, 10);
            HAL_GPIO_WritePin(SPI1_NSS_PORT, SPI1_NSS_PIN, GPIO_PIN_SET);

            uint32_t event_status = (int_resp[1] << 16) | (int_resp[2] << 8) | int_resp[3];

            if (event_status & PW_STATUS_THERMAL_CRITICAL) {
                debug_uart_printf("FPGA thermal critical interrupt!\r\n");
                g_system_state = SYS_STATE_SHUTDOWN;
            }
            if (event_status & PW_STATUS_CLOCK_LOL) {
                debug_uart_printf("Si5345 loss of lock detected!\r\n");
            }
            if (event_status & PW_STATUS_DDR4_ECC_ERROR) {
                debug_uart_printf("DDR4 ECC error detected!\r\n");
            }
            if (event_status & PW_STATUS_WATCHDOG_EXPIRED) {
                debug_uart_printf("FPGA watchdog expired — system may be unstable!\r\n");
            }
        }
    }
}

//=============================================================================
// System Clock Configuration
// 25 MHz HSE bypass (from Si5345 OUT5) → PLL → 480 MHz SYSCLK
//=============================================================================
static void SystemClock_Config(void)
{
    // Enable HSE bypass (external clock from Si5345)
    RCC->CR |= RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY));

    // Voltage Scale 1 for 480 MHz operation
    PWR->CR3 |= PWR_CR3_SCUEN;
    PWR->D3CR |= (PWR_D3CR_VOS_0 | PWR_D3CR_VOS_1);
    while (!(PWR->D3CR & PWR_D3CR_VOSRDY));

    // Flash latency: 4 wait states for 480 MHz @ VOS1
    FLASH->ACR = FLASH_ACR_LATENCY_4WS;

    // PLL1: HSE (25 MHz) / DIVM1(5) × DIVN1(96) = 480 MHz VCO
    // DIVP1 = 2 → PLL1P = 240 MHz (used as SYSCLK via D1CPRE=1)
    RCC->PLLCKSELR = (5 << RCC_PLLCKSELR_DIVM1_Pos) | RCC_PLLCKSELR_PLLSRC_HSE;
    RCC->PLL1DIVR = ((96-1) << RCC_PLL1DIVR_DIVN1_Pos) |
                    ((2-1)  << RCC_PLL1DIVR_DIVP1_Pos) |
                    ((4-1)  << RCC_PLL1DIVR_DIVQ1_Pos) |
                    ((2-1)  << RCC_PLL1DIVR_DIVR1_Pos);

    RCC->CR |= RCC_CR_PLL1ON;
    while (!(RCC->CR & RCC_CR_PLL1RDY));

    // Bus dividers
    // D1CPRE = 1 → SYSCLK = PLL1P / 1 = 240 MHz
    // HPRE = 0 → HCLK = SYSCLK / 1 = 240 MHz
    // D1PPRE = 2 → APB3 = HCLK / 4 = 60 MHz
    // D2PPRE1 = 2 → APB1 = HCLK / 4 = 60 MHz
    // D2PPRE2 = 2 → APB2 = HCLK / 4 = 60 MHz
    // D3PPRE = 2 → APB4 = HCLK / 4 = 60 MHz
    RCC->D1CFGR = (0 << RCC_D1CFGR_HPRE_Pos) |
                  (4 << RCC_D1CFGR_D1PPRE_Pos) |
                  (0 << RCC_D1CFGR_D1CPRE_Pos);
    RCC->D2CFGR = (4 << RCC_D2CFGR_D2PPRE1_Pos) |
                  (4 << RCC_D2CFGR_D2PPRE2_Pos);
    RCC->D3CFGR = (4 << RCC_D3CFGR_D3PPRE_Pos);

    // Switch to PLL1
    RCC->CFGR |= RCC_CFGR_SW_PLL1;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1);

    SystemCoreClock = 240000000; // SYSCLK = 240 MHz
}

//=============================================================================
// SysTick Configuration — 1 ms tick
//=============================================================================
static void SystemTick_Config(void)
{
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 0);
}

void SysTick_Handler(void)
{
    g_systick_counter++;
    g_uptime_ms++;
    HAL_IncTick();
}

//=============================================================================
// I2C1 Initialization — 100 kHz Standard-mode
//=============================================================================
static void I2C1_Init(void)
{
    __HAL_RCC_I2C1_CLK_ENABLE();

    hi2c1.Instance = I2C1;
    hi2c1.Init.Timing = I2C1_TIMING;
    hi2c1.Init.OwnAddress1 = 0;
    hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c1.Init.OwnAddress2 = 0;
    hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;

    if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
        Error_Handler();
    }

    // Configure analog filter
    HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
    // Configure digital filter
    HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
}

//=============================================================================
// SPI1 Initialization — 50 MHz, Master, Motorola format
//=============================================================================
static void SPI1_Init(void)
{
    __HAL_RCC_SPI1_CLK_ENABLE();

    hspi1.Instance = SPI1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4; // 60 MHz / 4 = 15 MHz
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi1.Init.CRCPolynomial = 7;
    hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    hspi1.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    hspi1.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    hspi1.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi1.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    hspi1.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    hspi1.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    hspi1.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    hspi1.Init.AutoSuspendDelay = SPI_AUTOSUSPEND_DELAY_00CYCLE;
    hspi1.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    hspi1.Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(&hspi1) != HAL_OK) {
        Error_Handler();
    }
}

//=============================================================================
// UART4 Initialization — 115200 baud, 8N1, debug console
//=============================================================================
static void UART4_Init(void)
{
    __HAL_RCC_UART4_CLK_ENABLE();

    huart4.Instance = UART4;
    huart4.Init.BaudRate = UART4_BAUDRATE;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart4.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart4) != HAL_OK) {
        Error_Handler();
    }
}

//=============================================================================
// TIM2 — 1 kHz system timer for periodic tasks
//=============================================================================
static void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = (APB1_FREQUENCY_HZ / 100000) - 1; // 100 kHz timer clock
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 100 - 1; // 100 kHz / 100 = 1 kHz
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

    if (HAL_TIM_Base_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }

    HAL_TIM_Base_Start_IT(&htim2);
    NVIC_SetPriority(TIM2_IRQn, 1);
    NVIC_EnableIRQ(TIM2_IRQn);
}

//=============================================================================
// NVIC Configuration
//=============================================================================
static void NVIC_Config(void)
{
    // Set priority grouping: 4 bits preempt, 0 bits subpriority
    NVIC_SetPriorityGrouping(0x03);

    // FPGA interrupt (PE0 EXTI0) — priority 2
    NVIC_SetPriority(EXTI0_IRQn, 2);
    NVIC_EnableIRQ(EXTI0_IRQn);

    // TMP117 alert (PE4 EXTI4) — priority 3
    NVIC_SetPriority(EXTI4_IRQn, 3);
    NVIC_EnableIRQ(EXTI4_IRQn);

    // Si5345 interrupt (PE5 EXTI5) — priority 3
    NVIC_SetPriority(EXTI9_5_IRQn, 3);
    NVIC_EnableIRQ(EXTI9_5_IRQn);

    // SPI1 interrupt — priority 4
    NVIC_SetPriority(SPI1_IRQn, 4);
    NVIC_EnableIRQ(SPI1_IRQn);

    // I2C1 interrupt — priority 5
    NVIC_SetPriority(I2C1_EV_IRQn, 5);
    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_SetPriority(I2C1_ER_IRQn, 5);
    NVIC_EnableIRQ(I2C1_ER_IRQn);

    // UART4 interrupt — priority 6
    NVIC_SetPriority(UART4_IRQn, 6);
    NVIC_EnableIRQ(UART4_IRQn);
}

//=============================================================================
// EXTI Interrupt Handlers
//=============================================================================
void EXTI0_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(FPGA_INT_PIN) != 0x00u) {
        __HAL_GPIO_EXTI_CLEAR_IT(FPGA_INT_PIN);
        // FPGA attention — handled in main loop
    }
}

void EXTI4_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(TMP117_ALERT_PIN) != 0x00u) {
        __HAL_GPIO_EXTI_CLEAR_IT(TMP117_ALERT_PIN);
        // Temperature alert — handled in main loop
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (__HAL_GPIO_EXTI_GET_IT(SI5345_INTR_PIN) != 0x00u) {
        __HAL_GPIO_EXTI_CLEAR_IT(SI5345_INTR_PIN);
        // Si5345 interrupt — loss of lock or other fault
    }
}

//=============================================================================
// TIM2 Interrupt Handler — 1 kHz system tick
//=============================================================================
void TIM2_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim2);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        // 1 kHz tasks can be added here
    }
}

//=============================================================================
// Board Support Functions
//=============================================================================
void board_init(void)
{
    // Already done in main() — this is for external callers
}

void board_get_state(system_state_t *state)
{
    if (state) *state = g_system_state;
}

void board_set_state(system_state_t state)
{
    g_system_state = state;
}

const char *board_state_string(system_state_t state)
{
    switch (state) {
        case SYS_STATE_BOOT:           return "BOOT";
        case SYS_STATE_CLOCK_INIT:     return "CLOCK_INIT";
        case SYS_STATE_FPGA_PROG:      return "FPGA_PROG";
        case SYS_STATE_FPGA_CAL:       return "FPGA_CAL";
        case SYS_STATE_READY:          return "READY";
        case SYS_STATE_ERROR:          return "ERROR";
        case SYS_STATE_THERMAL_THROT:  return "THERMAL_THROT";
        case SYS_STATE_SHUTDOWN:       return "SHUTDOWN";
        default:                       return "UNKNOWN";
    }
}

void board_delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

void board_delay_us(uint32_t us)
{
    // Simple busy-wait delay loop
    // At 240 MHz, ~4 cycles per iteration → ~60M iterations per second
    uint32_t count = us * 60;
    for (volatile uint32_t i = 0; i < count; i++) {
        __asm__("nop");
    }
}

uint32_t board_get_uptime_ms(void)
{
    return g_uptime_ms;
}

void board_heartbeat_update(void)
{
    // Toggle heartbeat LED at 1 Hz
    uint32_t phase = g_uptime_ms % LED_HEARTBEAT_PERIOD_MS;
    if (phase < (LED_HEARTBEAT_PERIOD_MS / 2)) {
        HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_RESET);
    }
}

void board_error_indicate(void)
{
    // Fast blink heartbeat LED to indicate error
    uint32_t phase = g_uptime_ms % LED_ERROR_BLINK_MS;
    if (phase < (LED_ERROR_BLINK_MS / 2)) {
        HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_HEARTBEAT_PORT, LED_HEARTBEAT_PIN, GPIO_PIN_RESET);
    }
    HAL_GPIO_WritePin(LED_USB_READY_PORT, LED_USB_READY_PIN, GPIO_PIN_RESET);
}

//=============================================================================
// Debug UART Printf (minimal implementation)
//=============================================================================
void debug_uart_printf(const char *fmt, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0) {
        HAL_UART_Transmit(&huart4, (uint8_t *)buffer, len, 100);
    }
}

//=============================================================================
// Error Handler — Fatal error trap
//=============================================================================
static void Error_Handler(void)
{
    __disable_irq();
    board_error_indicate();
    while (1) {
        // Trap — system halted
    }
}

//=============================================================================
// HAL Assert Failed Callback
//=============================================================================
void assert_failed(uint8_t *file, uint32_t line)
{
    debug_uart_printf("ASSERT FAILED: %s:%lu\r\n", (const char *)file, line);
    Error_Handler();
}
