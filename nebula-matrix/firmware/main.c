/**
 * main.c — Nebula Matrix STM32H7 Firmware Entry Point
 *
 * Complete system initialization and main loop for the Nebula Matrix
 * LED display engine co-processor (STM32H743VIT6).
 *
 * Responsibilities:
 *   - System clock configuration (480 MHz)
 *   - FPGA interface initialization (SPI pixel stream + UART command)
 *   - Ethernet initialization (LWIP + Art-Net/sACN receiver)
 *   - USB RNDIS initialization (web configuration interface)
 *   - SD card initialization (FatFS for playback and config)
 *   - I2C sensor initialization (TMP117 ×4, INA219, Si5351A, PMIC)
 *   - Fan PWM control with PID thermal management
 *   - FreeRTOS task creation and scheduler start
 *   - System health monitoring and error handling
 *
 * Build: arm-none-eabi-gcc, STM32H7 HAL, FreeRTOS, LWIP, FatFS, TinyUSB
 */

#include "stm32h7xx_hal.h"
#include "board.h"
#include "registers.h"
#include "drivers/hub75e_spi_driver.h"
#include "drivers/fpga_uart_driver.h"
#include "drivers/tmp117_driver.h"
#include "drivers/si5351_driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/udp.h"
#include "lwip/tcpip.h"
#include "ff.h"
#include "tusb.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* =========================================================================
 * Global State
 * ========================================================================= */

typedef enum {
    SYSTEM_STATE_BOOT,
    SYSTEM_STATE_INIT_CLOCKS,
    SYSTEM_STATE_INIT_FPGA,
    SYSTEM_STATE_INIT_PERIPHERALS,
    SYSTEM_STATE_RUNNING,
    SYSTEM_STATE_ERROR,
    SYSTEM_STATE_THERMAL_SHUTDOWN,
} system_state_t;

static volatile system_state_t g_system_state = SYSTEM_STATE_BOOT;
static volatile uint32_t g_system_errors = 0;
static volatile uint32_t g_boot_timestamp = 0;

/* Error codes */
#define ERR_FPGA_NOT_READY      (1 << 0)
#define ERR_DDR_CAL_FAIL        (1 << 1)
#define ERR_ETH_INIT_FAIL       (1 << 2)
#define ERR_USB_INIT_FAIL       (1 << 3)
#define ERR_SD_INIT_FAIL        (1 << 4)
#define ERR_SI5351_FAIL         (1 << 5)
#define ERR_PMIC_FAULT          (1 << 6)
#define ERR_THERMAL_CRIT        (1 << 7)
#define ERR_SPI_OVERRUN         (1 << 8)
#define ERR_UART_TIMEOUT        (1 << 9)

/* =========================================================================
 * FreeRTOS Task Handles
 * ========================================================================= */

static TaskHandle_t task_eth_rx_handle = NULL;
static TaskHandle_t task_pixel_push_handle = NULL;
static TaskHandle_t task_artnet_handle = NULL;
static TaskHandle_t task_http_handle = NULL;
static TaskHandle_t task_monitor_handle = NULL;
static TaskHandle_t task_sd_playback_handle = NULL;
static TaskHandle_t task_fpga_cmd_handle = NULL;

/* =========================================================================
 * Queue Handles
 * ========================================================================= */

static QueueHandle_t pixel_queue = NULL;
static QueueHandle_t cmd_queue = NULL;
static QueueHandle_t event_queue = NULL;

/* =========================================================================
 * Peripheral Handles
 * ========================================================================= */

static SPI_HandleTypeDef hspi2;       /* FPGA pixel stream */
static UART_HandleTypeDef huart1;     /* FPGA command interface */
static I2C_HandleTypeDef hi2c2;       /* TMP117 sensors */
static I2C_HandleTypeDef hi2c3;       /* Si5351A clock gen */
static I2C_HandleTypeDef hi2c4;       /* TPS65218 PMIC */
static TIM_HandleTypeDef htim1;       /* Fan PWM */
static ADC_HandleTypeDef hadc1;       /* Voltage/current monitor */
static SD_HandleTypeDef hsd1;         /* SD card */

/* =========================================================================
 * LWIP Network Interface
 * ========================================================================= */

static struct netif g_netif;
static ip4_addr_t g_ipaddr, g_netmask, g_gateway;

/* =========================================================================
 * Forward Declarations
 * ========================================================================= */

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void FPGA_Init(void);
static void Ethernet_Init(void);
static void USB_Init(void);
static void SD_Card_Init(void);
static void Sensors_Init(void);
static void Fan_Init(void);
static void FreeRTOS_Init(void);
static void Error_Handler(void);
static void SystemMonitor_Task(void *pvParameters);
static void PixelPush_Task(void *pvParameters);
static void ArtNet_Task(void *pvParameters);
static void HTTP_Task(void *pvParameters);
static void SDPlayback_Task(void *pvParameters);
static void FpgaCmd_Task(void *pvParameters);

/* =========================================================================
 * Main Entry Point
 * ========================================================================= */

int main(void)
{
    /* =====================================================================
     * Stage 0: Basic HAL Initialization
     * ===================================================================== */
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    g_system_state = SYSTEM_STATE_INIT_CLOCKS;
    g_boot_timestamp = HAL_GetTick();

    /* =====================================================================
     * Stage 1: Configure Si5351A Clock Generator
     * ===================================================================== */
    if (si5351_init(&hi2c3) != 0) {
        g_system_errors |= ERR_SI5351_FAIL;
        Error_Handler();
    }

    /* Configure all 8 clock outputs per Nebula Matrix spec */
    si5351_configure_all();

    /* Verify PMIC status */
    if (pmic_check_status(&hi2c4) != 0) {
        g_system_errors |= ERR_PMIC_FAULT;
        Error_Handler();
    }

    g_system_state = SYSTEM_STATE_INIT_FPGA;

    /* =====================================================================
     * Stage 2: FPGA Initialization
     * ===================================================================== */

    /* Wait for FPGA DONE signal (with 500ms timeout) */
    uint32_t fpga_timeout = HAL_GetTick() + 500;
    while (GPIO_READ(FPGA_DONE_PORT, FPGA_DONE_PIN) == GPIO_PIN_RESET) {
        if (HAL_GetTick() > fpga_timeout) {
            /* FPGA didn't configure from flash — attempt reprogramming */
            fpga_reprogram_from_sd();
            fpga_timeout = HAL_GetTick() + 1000;  /* Extend timeout */
        }
    }

    /* Assert then de-assert FPGA reset to ensure clean state */
    RESET_ASSERT(FPGA_RESET_N_PORT, FPGA_RESET_N_PIN);
    HAL_Delay(1);
    RESET_DEASSERT(FPGA_RESET_N_PORT, FPGA_RESET_N_PIN);
    HAL_Delay(10);  /* Wait for FPGA PLL lock */

    /* Initialize FPGA communication channels */
    fpga_uart_init(&huart1);
    fpga_spi_init(&hspi2);

    /* Verify FPGA communication */
    uint32_t fpga_id;
    if (fpga_read_register(FPGA_REG_ID, &fpga_id) != 0) {
        g_system_errors |= ERR_FPGA_NOT_READY;
        Error_Handler();
    }

    if (fpga_id != 0x4E454255) {  /* "NEBU" */
        g_system_errors |= ERR_FPGA_NOT_READY;
        Error_Handler();
    }

    /* Wait for DDR3 calibration */
    uint32_t fpga_status;
    uint32_t ddr_timeout = HAL_GetTick() + 2000;
    do {
        fpga_read_register(FPGA_REG_STATUS, &fpga_status);
        if (HAL_GetTick() > ddr_timeout) {
            g_system_errors |= ERR_DDR_CAL_FAIL;
            Error_Handler();
        }
    } while (!(fpga_status & FPGA_STATUS_DDR_CAL_DONE));

    /* Configure FPGA with default matrix parameters */
    fpga_write_register(FPGA_REG_MATRIX_WIDTH, DEFAULT_MATRIX_WIDTH);
    fpga_write_register(FPGA_REG_MATRIX_HEIGHT, DEFAULT_MATRIX_HEIGHT);
    fpga_write_register(FPGA_REG_PANEL_SCAN, DEFAULT_PANEL_SCAN);
    fpga_write_register(FPGA_REG_BRIGHTNESS, DEFAULT_BRIGHTNESS);
    fpga_write_register(FPGA_REG_CONTRAST, DEFAULT_CONTRAST);
    fpga_write_register(FPGA_REG_INPUT_SOURCE, INPUT_SRC_NONE);

    /* Load default gamma LUTs (linear, 1:1 mapping) */
    fpga_load_default_gamma_luts();

    g_system_state = SYSTEM_STATE_INIT_PERIPHERALS;

    /* =====================================================================
     * Stage 3: Peripheral Initialization
     * ===================================================================== */

    /* Initialize temperature sensors */
    Sensors_Init();

    /* Initialize fan PWM control */
    Fan_Init();

    /* Initialize Ethernet */
    if (Ethernet_Init() != 0) {
        g_system_errors |= ERR_ETH_INIT_FAIL;
        /* Non-fatal: device can operate without Ethernet */
    }

    /* Initialize USB RNDIS */
    if (USB_Init() != 0) {
        g_system_errors |= ERR_USB_INIT_FAIL;
        /* Non-fatal: device can operate without USB */
    }

    /* Initialize SD card */
    if (SD_Card_Init() != 0) {
        g_system_errors |= ERR_SD_INIT_FAIL;
        /* Non-fatal: device can operate without SD card */
    }

    /* =====================================================================
     * Stage 4: Start FreeRTOS Scheduler
     * ===================================================================== */

    FreeRTOS_Init();

    g_system_state = SYSTEM_STATE_RUNNING;

    /* Start the scheduler — this function never returns */
    vTaskStartScheduler();

    /* Should never reach here */
    while (1) {
        Error_Handler();
    }
}

/* =========================================================================
 * System Clock Configuration
 * ========================================================================= */

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

    /* Configure voltage scaling for 480 MHz */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    /* HSE: 25 MHz from Si5351A CLK0 (bypass mode) */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192;
    RCC_OscInitStruct.PLL.PLLP = 2;
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    /* System clock: 480 MHz, AHB: 240 MHz, APB: 120 MHz */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2
                                | RCC_CLOCKTYPE_D1PCLK1 | RCC_CLOCKTYPE_D3PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4);

    /* Peripheral clocks */
    PeriphClkInitStruct.PeriphClockSelection =
        RCC_PERIPHCLK_SPI2 | RCC_PERIPHCLK_SPI1 |
        RCC_PERIPHCLK_USART1 | RCC_PERIPHCLK_SDMMC |
        RCC_PERIPHCLK_USB | RCC_PERIPHCLK_FDCAN;
    PeriphClkInitStruct.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
    PeriphClkInitStruct.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    PeriphClkInitStruct.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL;
    PeriphClkInitStruct.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
    PeriphClkInitStruct.PLL2.PLL2M = 5;
    PeriphClkInitStruct.PLL2.PLL2N = 100;
    PeriphClkInitStruct.PLL2.PLL2P = 5;
    PeriphClkInitStruct.PLL2.PLL2Q = 10;
    PeriphClkInitStruct.PLL2.PLL2R = 10;
    PeriphClkInitStruct.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_1;
    PeriphClkInitStruct.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE;
    PeriphClkInitStruct.PLL2.PLL2FRACN = 0;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);

    /* Enable caches */
    SCB_EnableICache();
    SCB_EnableDCache();
}

/* =========================================================================
 * GPIO Initialization
 * ========================================================================= */

static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable all GPIO clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* FPGA SPI2 pins (PB12-15, AF5, Very High Speed) */
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* FPGA UART1 pins (PA9, PA10, AF7) */
    GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* FPGA control signals (PE0-PE3) */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    GPIO_SET(FPGA_RESET_N_PORT, FPGA_RESET_N_PIN);

    /* Status LEDs (PB0-PB2) */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    LED_OFF(LED_STATUS_PORT, LED_STATUS_PIN);
    LED_OFF(LED_ERROR_PORT, LED_ERROR_PIN);
    LED_OFF(LED_ETH_ACT_PORT, LED_ETH_ACT_PIN);

    /* Fan PWM (PA0, PA1, AF1) */
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC inputs (PA2, PA3) */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* I2C2 (PB6, PB7, AF4) */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C3 (PB8, PB9, AF4) */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF4_I2C3;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* I2C4 (PB10, PB11, AF6) */
    GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF6_I2C4;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Ethernet RMII pins */
    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2
                        | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF11_ETH;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    /* ETH PHY control */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    RESET_ASSERT(ETH_RESET_N_PORT, ETH_RESET_N_PIN);

    /* USB OTG FS pins (PA11, PA12, AF10) */
    GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_OTG1_FS;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USB PHY control */
    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    RESET_ASSERT(USB_RESET_N_PORT, USB_RESET_N_PIN);

    /* SDMMC1 pins (PC6-PC12, PD2, AF12) */
    GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9
                        | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF12_SDMMC1;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* HDMI control (PB3, PB4) */
    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    RESET_ASSERT(HDMI_RESET_N_PORT, HDMI_RESET_N_PIN);

    /* PMIC control (PE14, PE15) */
    GPIO_InitStruct.Pin = GPIO_PIN_14;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
    GPIO_SET(PMIC_EN_PORT, PMIC_EN_PIN);

    /* Enable NVIC interrupts */
    HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
    HAL_NVIC_SetPriority(EXTI3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);
    HAL_NVIC_SetPriority(EXTI4_IRQn, 7, 0);
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 8, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* =========================================================================
 * FPGA Initialization
 * ========================================================================= */

static void FPGA_Init(void)
{
    /* FPGA initialization is handled inline in main() above.
     * This function is a placeholder for any additional FPGA setup. */
}

/* =========================================================================
 * Ethernet Initialization
 * ========================================================================= */

static void Ethernet_Init(void)
{
    /* Release KSZ9031RNX from reset */
    HAL_Delay(10);
    RESET_DEASSERT(ETH_RESET_N_PORT, ETH_RESET_N_PIN);
    HAL_Delay(50);  /* Wait for PHY to stabilize */

    /* Initialize LWIP */
    lwip_init();

    /* Configure network interface */
    IP4_ADDR(&g_ipaddr, 192, 168, 1, 100);
    IP4_ADDR(&g_netmask, 255, 255, 255, 0);
    IP4_ADDR(&g_gateway, 192, 168, 1, 1);

    netif_add(&g_netif, &g_ipaddr, &g_netmask, &g_gateway, NULL,
              ethernetif_init, tcpip_input);
    netif_set_default(&g_netif);
    netif_set_up(&g_netif);

    /* Start Art-Net/sACN receiver */
    artnet_receiver_init();
}

/* =========================================================================
 * USB Initialization
 * ========================================================================= */

static void USB_Init(void)
{
    /* Release USB3320C from reset */
    HAL_Delay(10);
    RESET_DEASSERT(USB_RESET_N_PORT, USB_RESET_N_PIN);
    HAL_Delay(20);

    /* Initialize TinyUSB */
    tusb_init();

    /* RNDIS will be configured when host enumerates */
}

/* =========================================================================
 * SD Card Initialization
 * ========================================================================= */

static void SD_Card_Init(void)
{
    FATFS fs;
    FRESULT res;

    /* Mount SD card */
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK) {
        /* SD card not present or failed — non-fatal */
        return;
    }

    /* Check for firmware update files */
    if (f_stat("0:/firmware/nebula_matrix.bit", NULL) == FR_OK) {
        /* New FPGA bitstream available */
    }

    if (f_stat("0:/firmware/nebula_matrix.bin", NULL) == FR_OK) {
        /* New MCU firmware available */
    }
}

/* =========================================================================
 * Sensor Initialization
 * ========================================================================= */

static void Sensors_Init(void)
{
    /* Initialize TMP117 temperature sensors on I2C2 */
    tmp117_init(&hi2c2, TMP117_I2C_ADDR_1);
    tmp117_init(&hi2c2, TMP117_I2C_ADDR_2);
    tmp117_init(&hi2c2, TMP117_I2C_ADDR_3);
    tmp117_init(&hi2c2, TMP117_I2C_ADDR_4);

    /* Initialize INA219 current/power monitor */
    ina219_init(&hi2c2, INA219_I2C_ADDR);
}

/* =========================================================================
 * Fan PWM Initialization
 * ========================================================================= */

static void Fan_Init(void)
{
    TIM_OC_InitTypeDef sConfigOC = {0};

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 0;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = FAN_PWM_PERIOD - 1;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_PWM_Init(&htim1);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;  /* Start with fans off */
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1);
    HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_2);
}

/* =========================================================================
 * FreeRTOS Initialization
 * ========================================================================= */

static void FreeRTOS_Init(void)
{
    /* Create queues */
    pixel_queue = xQueueCreate(512, sizeof(pixel_data_t));
    cmd_queue = xQueueCreate(32, sizeof(fpga_cmd_t));
    event_queue = xQueueCreate(16, sizeof(system_event_t));

    /* Create tasks */
    xTaskCreate(PixelPush_Task, "PIXEL_PUSH", 1024, NULL, 4, &task_pixel_push_handle);
    xTaskCreate(ArtNet_Task, "ARTNET", 4096, NULL, 3, &task_artnet_handle);
    xTaskCreate(HTTP_Task, "HTTP", 8192, NULL, 2, &task_http_handle);
    xTaskCreate(SystemMonitor_Task, "MONITOR", 1024, NULL, 2, &task_monitor_handle);
    xTaskCreate(SDPlayback_Task, "SD_PLAY", 4096, NULL, 3, &task_sd_playback_handle);
    xTaskCreate(FpgaCmd_Task, "FPGA_CMD", 2048, NULL, 3, &task_fpga_cmd_handle);
}

/* =========================================================================
 * System Monitor Task
 * ========================================================================= */

static void SystemMonitor_Task(void *pvParameters)
{
    float temps[4];
    float voltage_12v, current_12v;
    uint16_t fan_speed_pct = 0;
    TickType_t last_wake = xTaskGetTickCount();
    uint32_t fpga_status;

    for (;;) {
        /* Read all TMP117 sensors */
        for (int i = 0; i < 4; i++) {
            temps[i] = tmp117_read_temp_c(i);
        }

        /* Read INA219 */
        voltage_12v = ina219_read_voltage();
        current_12v = ina219_read_current();

        /* Read FPGA status */
        fpga_read_register(FPGA_REG_STATUS, &fpga_status);

        /* PID fan control based on FPGA temperature */
        float fpga_temp = temps[0];
        if (fpga_temp < FAN_OFF_TEMP_C) {
            fan_speed_pct = 0;
        } else if (fpga_temp < FAN_MIN_TEMP_C) {
            fan_speed_pct = (uint16_t)((fpga_temp - FAN_OFF_TEMP_C) * 3.0f);
        } else if (fpga_temp < 60.0f) {
            fan_speed_pct = 30 + (uint16_t)((fpga_temp - FAN_MIN_TEMP_C) * 3.0f);
        } else if (fpga_temp < FAN_MAX_TEMP_C) {
            fan_speed_pct = 60 + (uint16_t)((fpga_temp - 60.0f) * 4.0f);
        } else {
            fan_speed_pct = 100;
        }

        /* Set fan PWM */
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1,
            (uint32_t)(fan_speed_pct * FAN_PWM_PERIOD / 100));
        __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2,
            (uint32_t)(fan_speed_pct * FAN_PWM_PERIOD / 100));

        /* Thermal event handling */
        if (fpga_temp > THERMAL_SHUTDOWN_THRESHOLD_C) {
            g_system_state = SYSTEM_STATE_THERMAL_SHUTDOWN;
            g_system_errors |= ERR_THERMAL_CRIT;
            fpga_write_register(FPGA_REG_CONTROL, 0);  /* Disable output */
            LED_ON(LED_ERROR_PORT, LED_ERROR_PIN);
        } else if (fpga_temp > THERMAL_CRIT_THRESHOLD_C) {
            LED_ON(LED_ERROR_PORT, LED_ERROR_PIN);
            system_event_t evt = EVENT_THERMAL_CRIT;
            xQueueSend(event_queue, &evt, 0);
        } else if (fpga_temp > THERMAL_WARN_THRESHOLD_C) {
            system_event_t evt = EVENT_THERMAL_WARN;
            xQueueSend(event_queue, &evt, 0);
        }

        /* Update status LED */
        if (g_system_state == SYSTEM_STATE_RUNNING) {
            LED_TOGGLE(LED_STATUS_PORT, LED_STATUS_PIN);
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(1000));
    }
}

/* =========================================================================
 * Pixel Push Task
 * ========================================================================= */

static void PixelPush_Task(void *pvParameters)
{
    pixel_data_t pixel;

    for (;;) {
        if (xQueueReceive(pixel_queue, &pixel, portMAX_DELAY) == pdTRUE) {
            fpga_spi_send_pixel(pixel.x, pixel.y, pixel.r, pixel.g, pixel.b);
        }
    }
}

/* =========================================================================
 * Art-Net / sACN Task
 * ========================================================================= */

static void ArtNet_Task(void *pvParameters)
{
    /* This task is driven by LWIP callbacks.
     * It periodically processes received DMX data and pushes
     * complete frames to the pixel queue. */
    for (;;) {
        sys_check_timeouts();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/* =========================================================================
 * HTTP Configuration Task
 * ========================================================================= */

static void HTTP_Task(void *pvParameters)
{
    /* Mongoose HTTP server for web-based configuration.
     * Serves configuration page, REST API for settings,
     * and WebSocket for real-time monitoring. */
    for (;;) {
        mongoose_poll(100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* =========================================================================
 * SD Card Playback Task
 * ========================================================================= */

static void SDPlayback_Task(void *pvParameters)
{
    FIL file;
    FRESULT res;
    uint8_t buffer[4096];
    UINT bytes_read;

    for (;;) {
        /* Wait for playback command */
        uint32_t notification;
        xTaskNotifyWait(0, 0xFFFFFFFF, &notification, pdMS_TO_TICKS(1000));

        if (notification & 0x01) {
            /* Open raw pixel stream file */
            res = f_open(&file, "0:/playback/stream.raw", FA_READ);
            if (res != FR_OK) continue;

            /* Set FPGA input source to SD card */
            fpga_write_register(FPGA_REG_INPUT_SOURCE, INPUT_SRC_SD_CARD);

            /* Stream frames */
            while (f_read(&file, buffer, sizeof(buffer), &bytes_read) == FR_OK
                   && bytes_read > 0) {
                /* Push pixels to FPGA via SPI */
                for (UINT i = 0; i < bytes_read; i += 6) {
                    uint16_t x = (i / 6) % DEFAULT_MATRIX_WIDTH;
                    uint16_t y = (i / 6) / DEFAULT_MATRIX_WIDTH;
                    uint16_t r = (buffer[i+0] << 8) | buffer[i+1];
                    uint16_t g = (buffer[i+2] << 8) | buffer[i+3];
                    uint16_t b = (buffer[i+4] << 8) | buffer[i+5];

                    pixel_data_t pixel = {x, y, r, g, b};
                    xQueueSend(pixel_queue, &pixel, pdMS_TO_TICKS(10));
                }
            }

            f_close(&file);
        }
    }
}

/* =========================================================================
 * FPGA Command Task
 * ========================================================================= */

static void FpgaCmd_Task(void *pvParameters)
{
    fpga_cmd_t cmd;
    uint32_t fpga_status;

    for (;;) {
        if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (cmd.is_write) {
                fpga_write_register(cmd.addr, cmd.data);
            } else {
                fpga_read_register(cmd.addr, &cmd.data);
            }
        }

        /* Periodic FPGA status poll */
        static uint32_t last_poll = 0;
        uint32_t now = xTaskGetTickCount();
        if ((now - last_poll) > pdMS_TO_TICKS(500)) {
            fpga_read_register(FPGA_REG_STATUS, &fpga_status);

            if (fpga_status & (FPGA_STATUS_DDR_A_ERROR | FPGA_STATUS_DDR_B_ERROR)) {
                g_system_errors |= ERR_DDR_CAL_FAIL;
                LED_ON(LED_ERROR_PORT, LED_ERROR_PIN);
            }

            if (fpga_status & FPGA_STATUS_INPUT_OVERFLOW) {
                g_system_errors |= ERR_SPI_OVERRUN;
            }

            last_poll = now;
        }
    }
}

/* =========================================================================
 * Error Handler
 * ========================================================================= */

static void Error_Handler(void)
{
    g_system_state = SYSTEM_STATE_ERROR;
    LED_ON(LED_ERROR_PORT, LED_ERROR_PIN);
    LED_OFF(LED_STATUS_PORT, LED_STATUS_PIN);

    /* Disable HUB75E output */
    fpga_write_register(FPGA_REG_CONTROL, 0);

    /* Log error code for debugging */
    /* In production, this would be stored in backup registers or EEPROM */

    while (1) {
        /* Blink error LED with pattern indicating error code */
        for (int i = 0; i < 8; i++) {
            if (g_system_errors & (1 << i)) {
                LED_ON(LED_ERROR_PORT, LED_ERROR_PIN);
                HAL_Delay(200);
                LED_OFF(LED_ERROR_PORT, LED_ERROR_PIN);
                HAL_Delay(200);
            }
        }
        HAL_Delay(2000);  /* Pause between error code repeats */
    }
}

/* =========================================================================
 * Interrupt Handlers
 * ========================================================================= */

void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(FPGA_IRQ_PIN);
}

void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(FPGA_DONE_PIN);
}

void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(HDMI_INT1_PIN);
}

void EXTI15_10_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(ETH_INT_N_PIN);
    HAL_GPIO_EXTI_IRQHandler(USB_INT_N_PIN);
    HAL_GPIO_EXTI_IRQHandler(PMIC_INT_PIN);
}

void DMA1_Stream0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hspi2.hdmatx);
}

void USART1_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart1);
}

void ETH_IRQHandler(void)
{
    HAL_ETH_IRQHandler(&heth);
}

void OTG_FS_IRQHandler(void)
{
    tud_int_handler(0);
}

void HardFault_Handler(void)
{
    g_system_state = SYSTEM_STATE_ERROR;
    g_system_errors |= 0x80000000;  /* Hard fault indicator */
    while (1) {
        LED_ON(LED_ERROR_PORT, LED_ERROR_PIN);
    }
}

void MemManage_Handler(void)
{
    g_system_state = SYSTEM_STATE_ERROR;
    while (1) {}
}

void BusFault_Handler(void)
{
    g_system_state = SYSTEM_STATE_ERROR;
    while (1) {}
}

void UsageFault_Handler(void)
{
    g_system_state = SYSTEM_STATE_ERROR;
    while (1) {}
}

/* =========================================================================
 * HAL Callbacks
 * ========================================================================= */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == FPGA_DONE_PIN) {
        /* FPGA configuration complete */
        system_event_t evt = EVENT_FPGA_DONE;
        xQueueSendFromISR(event_queue, &evt, NULL);
    } else if (GPIO_Pin == FPGA_IRQ_PIN) {
        system_event_t evt = EVENT_FPGA_IRQ;
        xQueueSendFromISR(event_queue, &evt, NULL);
    } else if (GPIO_Pin == HDMI_INT1_PIN) {
        system_event_t evt = EVENT_HDMI_INT;
        xQueueSendFromISR(event_queue, &evt, NULL);
    }
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        /* SPI pixel transfer complete */
        GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI2) {
        GPIO_SET(FPGA_SPI2_NSS_PORT, FPGA_SPI2_NSS_PIN);
        g_system_errors |= ERR_SPI_OVERRUN;
    }
}

/* =========================================================================
 * Assert Handler (for debug builds)
 * ========================================================================= */

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* Log assertion failure */
    while (1) {}
}
#endif
