/*
 * main.c — PicoRadar SPL/board init entry point
 *
 * STM32H743VIT6 FreeRTOS-based radar host application.
 * Initializes all peripherals, starts radar, enters task scheduler.
 */

#include "registers.h"
#include "board.h"
#include "drivers/imu_icm42688.h"
#include "drivers/oled_sh1106.h"
#include <string.h>

/* ---- FreeRTOS headers (stub for standalone build) ---- */
#ifdef USE_FREERTOS
#include "FreeRTOS.h"
#include "task.h"
#else
/* Bare-metal stubs */
typedef void *TaskHandle_t;
typedef uint32_t TickType_t;
#define configMINIMAL_STACK_SIZE 128
typedef void (*TaskFunction_t)(void *);
static inline int xTaskCreate(void (*f)(void *), const char *n, uint16_t s,
                               void *p, int pri, TaskHandle_t *h) { (void)f; (void)n; (void)s; (void)p; (void)pri; (void)h; return 0; }
static inline void vTaskStartScheduler(void) { while(1); }
static inline void vTaskDelay(TickType_t t) { (void)t; }
#define pdMS_TO_TICKS(ms) (ms)
#endif

/* ---- Global task handles ---- */
static TaskHandle_t radar_task_handle;
static TaskHandle_t imu_task_handle;
static TaskHandle_t display_task_handle;
static TaskHandle_t wifi_task_handle;
static TaskHandle_t eth_task_handle;
static TaskHandle_t usb_task_handle;

/* ---- Simple delay (busy-wait, used before scheduler) ---- */
static void delay_ms(uint32_t ms)
{
    /* Approximate: at 480 MHz, each loop iteration ~4 cycles → ~6 cycles with branch */
    volatile uint32_t count = ms * (480000000 / 6000);
    while (count--) __asm__("nop");
}

/* ---- External functions defined in other modules ---- */
extern void SystemClock_Config(void);
extern void GPIO_Init(void);

/* ---- UART1 Init (Radar) ---- */
static void UART1_Init(void)
{
    /* Enable USART1 clock */
    RCC_APB2ENR |= (1 << 4);  // USART1EN

    /* Disable USART */
    USART_CR1(USART1_BASE) &= ~USART_CR1_UE;

    /* Configure: 8N1, oversampling by 16, no hardware flow control */
    USART_CR1(USART1_BASE) = USART_CR1_TE | USART_CR1_RE;
    USART_CR2(USART1_BASE) = 0;  // 1 stop bit
    USART_CR3(USART1_BASE) = 0;  // No CTS/RTS hardware flow (managed by software)

    /* Baud rate = 921600
     * BRR = APB2_FREQ / baud = 120000000 / 921600 = ~130.2 → 130
     * (Oversampling 16, so BRR = USARTDIV, mantissa + fraction)
     * USARTDIV = 120000000 / 921600 = 130.208 → BRR = 130
     */
    USART_BRR(USART1_BASE) = 130;

    /* Enable USART */
    USART_CR1(USART1_BASE) |= USART_CR1_UE;
}

/* ---- UART2 Init (ESP32-C3) ---- */
static void UART2_Init(void)
{
    /* Enable USART2 clock */
    RCC_APB1LENR |= (1 << 17);  // USART2EN

    USART_CR1(USART2_BASE) &= ~USART_CR1_UE;
    USART_CR1(USART2_BASE) = USART_CR1_TE | USART_CR1_RE;
    USART_CR2(USART2_BASE) = 0;

    /* Baud rate = 115200
     * USARTDIV = 120000000 / 115200 = 1041.67 → BRR = 1042
     */
    USART_BRR(USART2_BASE) = 1042;

    USART_CR1(USART2_BASE) |= USART_CR1_UE;
}

/* ---- UART helper: send string ---- */
static void uart1_send_str(const char *str)
{
    while (*str) {
        while (!(USART_ISR(USART1_BASE) & USART_ISR_TXE));
        USART_TDR(USART1_BASE) = *str++;
    }
    while (!(USART_ISR(USART1_BASE) & USART_ISR_TC));
}

static void uart2_send_str(const char *str)
{
    while (*str) {
        while (!(USART_ISR(USART2_BASE) & USART_ISR_TXE));
        USART_TDR(USART2_BASE) = *str++;
    }
    while (!(USART_ISR(USART2_BASE) & USART_ISR_TC));
}

/* ---- Radar initialization ---- */
static int radar_init(void)
{
    /* Hold radar in reset for 100 ms after power stable */
    GPIO_ODR(GPIOD_BASE) &= ~(1 << RADAR_NRST_PIN);  // Assert NRST low
    delay_ms(100);
    GPIO_ODR(GPIOD_BASE) |= (1 << RADAR_NRST_PIN);    // Release NRST

    /* Wait for radar to boot (IWR6843 takes ~1.5s from reset) */
    delay_ms(1500);

    /* Send sensorStart command */
    uart1_send_str("sensorStart\n");

    /* Wait for HOST_INTR or response */
    delay_ms(500);

    return 0;
}

/* ---- Ethernet PHY init ---- */
static void eth_phy_init(void)
{
    /* Reset PHY */
    GPIO_ODR(GPIOD_BASE) &= ~(1 << ETH_nRST_PIN);
    delay_ms(10);
    GPIO_ODR(GPIOD_BASE) |= (1 << ETH_nRST_PIN);
    delay_ms(50);

    /* Read PHY ID to verify */
    uint32_t phy_id1 = 0, phy_id2 = 0;
    /* SMI read: write MACMIIAR, read MACMIIDR */
    ETH_MACMIIAR = (0x00 << 6) | (0 << 1) | (1 << 0);  // PHY addr 0, reg 2, read
    while (ETH_MACMIIAR & 1);  // Wait for busy clear
    phy_id1 = ETH_MACMIIDR;

    ETH_MACMIIAR = (0x00 << 6) | (3 << 1) | (1 << 0);  // PHY addr 0, reg 3, read
    while (ETH_MACMIIAR & 1);
    phy_id2 = ETH_MACMIIDR;

    (void)phy_id1;
    (void)phy_id2;
}

/* ---- ESP32-C3 init ---- */
static void esp32_init(void)
{
    /* Hold ESP32 in reset */
    GPIO_ODR(GPIOC_BASE) &= ~(1 << ESP_EN_PIN);
    delay_ms(10);

    /* Set BOOT pin high (normal boot) */
    GPIO_ODR(GPIOC_BASE) |= (1 << ESP_BOOT_PIN);

    /* Release reset */
    GPIO_ODR(GPIOC_BASE) |= (1 << ESP_EN_PIN);
    delay_ms(500);

    /* Send AT command to verify */
    uart2_send_str("AT\r\n");
    delay_ms(100);
}

/* ===================================================================
 * FreeRTOS Tasks
 * =================================================================== */

static void RadarTask(void *pvParams)
{
    (void)pvParams;
    uint8_t rx_buf[256];
    int rx_idx = 0;

    while (1) {
        /* Read radar data from UART1 */
        if (USART_ISR(USART1_BASE) & USART_ISR_RXNE) {
            uint8_t byte = (uint8_t)USART_RDR(USART1_BASE);
            rx_buf[rx_idx++] = byte;
            if (rx_idx >= sizeof(rx_buf)) rx_idx = 0;

            /* Parse TLV frames from mmWave SDK output */
            /* Header: magic (0x70 0x70 0x70 0x70), version, totalLen, platform, frameNum, ...
             * This is a simplified parser; real implementation handles full protocol */
            if (rx_idx >= 8 && rx_buf[0] == 0x70 && rx_buf[1] == 0x70) {
                /* Valid frame header detected — process point cloud TLVs */
                /* TODO: Full TLV parsing, publish to WiFi/Eth/Display queues */
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static void ImuTask(void *pvParams)
{
    (void)pvParams;
    imu_sample_t sample;
    imu_scaled_t scaled;

    while (1) {
        imu_read_raw(&sample);
        imu_scale(&sample, &scaled);
        /* TODO: Feed scaled IMU data to sensor fusion algorithm */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void DisplayTask(void *pvParams)
{
    (void)pvParams;
    uint32_t frame_count = 0;

    while (1) {
        oled_clear();
        oled_draw_string(0, 0, "PicoRadar", OLED_COLOR_WHITE);
        oled_draw_string(0, 8, "60GHz FMCW", OLED_COLOR_WHITE);

        /* Display point count (placeholder) */
        char buf[16];
        int pts = 0;  /* TODO: Read from radar task queue */
        buf[0] = 'P'; buf[1] = 't'; buf[2] = 's'; buf[3] = ':';
        buf[4] = '0' + (pts % 10);
        buf[5] = '\0';
        oled_draw_string(0, 24, buf, OLED_COLOR_WHITE);

        /* Display range bars (simplified visualization) */
        for (int i = 0; i < 16; i++) {
            uint8_t h = (i * 3 + (frame_count & 0x1F)) % 40;
            oled_fill_rect(i * 8, 63 - h, 7, h, OLED_COLOR_WHITE);
        }

        oled_update();
        frame_count++;
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void WifiTask(void *pvParams)
{
    (void)pvParams;

    /* Configure ESP32-C3 for WiFi station mode */
    uart2_send_str("AT+CWMODE=1\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));

    /* TODO: Join AP, open TCP socket, stream radar data */

    while (1) {
        /* Periodic WiFi status check + data streaming */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void EthTask(void *pvParams)
{
    (void)pvParams;

    /* TODO: LwIP init, DHCP, MQTT connect, publish radar point cloud */

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void UsbTask(void *pvParams)
{
    (void)pvParams;

    /* TODO: USB CDC init, serial console for debug/config */

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/* ===================================================================
 * Main Entry Point
 * =================================================================== */

int main(void)
{
    /* Step 1: Configure system clocks (HSE → PLL → 480 MHz) */
    SystemClock_Config();

    /* Step 2: Initialize all GPIO pins */
    GPIO_Init();

    /* Step 3: Initialize UART1 (radar) and UART2 (ESP32) */
    UART1_Init();
    UART2_Init();

    /* Step 4: Initialize OLED display */
    oled_init();
    oled_draw_string(0, 0, "PicoRadar", OLED_COLOR_WHITE);
    oled_draw_string(0, 16, "Booting...", OLED_COLOR_WHITE);
    oled_update();

    /* Step 5: Initialize IMU */
    int imu_status = imu_init();
    if (imu_status == 0) {
        oled_draw_string(0, 24, "IMU: OK", OLED_COLOR_WHITE);
    } else {
        oled_draw_string(0, 24, "IMU: FAIL", OLED_COLOR_WHITE);
    }
    oled_update();

    /* Step 6: Initialize Ethernet PHY */
    eth_phy_init();

    /* Step 7: Initialize ESP32-C3 WiFi/BLE module */
    esp32_init();

    /* Step 8: Initialize radar SoC */
    radar_init();
    oled_draw_string(0, 32, "Radar: ON", OLED_COLOR_WHITE);
    oled_update();

    /* Step 9: Create FreeRTOS tasks */
    xTaskCreate(RadarTask,   "Radar",   1024, NULL, 5, &radar_task_handle);
    xTaskCreate(ImuTask,     "IMU",     512,  NULL, 4, &imu_task_handle);
    xTaskCreate(DisplayTask, "Display", 1024, NULL, 3, &display_task_handle);
    xTaskCreate(WifiTask,    "WiFi",    2048, NULL, 2, &wifi_task_handle);
    xTaskCreate(EthTask,     "Eth",     2048, NULL, 2, &eth_task_handle);
    xTaskCreate(UsbTask,     "USB",     512,  NULL, 1, &usb_task_handle);

    /* Step 10: Start FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never reach here */
    while (1) {
        /* Fallback: blink LED or watchdog feed */
    }

    return 0;
}

/* ---- FreeRTOS hooks (required stubs) ---- */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    while (1);
}

void vApplicationMallocFailedHook(void)
{
    while (1);
}

/* ---- Cortex-M7 fault handlers ---- */
void HardFault_Handler(void)
{
    while (1);
}

void MemManage_Handler(void)
{
    while (1);
}

void BusFault_Handler(void)
{
    while (1);
}

void UsageFault_Handler(void)
{
    while (1);
}

/* ---- Default interrupt handler ---- */
void Default_Handler(void)
{
    while (1);
}