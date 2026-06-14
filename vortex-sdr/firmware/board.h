/*
 * board.h — Vortex-SDR pin definitions, board constants, and configuration
 * STM32H743ZIT6 @ 480 MHz, Cortex-M7, 1MB Flash, 1MB RAM
 */

#ifndef VORTEX_SDR_BOARD_H
#define VORTEX_SDR_BOARD_H

#include <stdint.h>
#include <stddef.h>

/* ========================================================================== */
/* Clock configuration                                                        */
/* ========================================================================== */
#define HSE_FREQ_HZ        8000000UL    /* 8 MHz external crystal */
#define LSE_FREQ_HZ        32768UL      /* 32.768 kHz RTC crystal */
#define SYSCLK_FREQ_HZ     480000000UL  /* 480 MHz system clock */
#define AHB_FREQ_HZ        240000000UL  /* 240 MHz AHB (SYSCLK/2) */
#define APB1_FREQ_HZ       60000000UL   /* 60 MHz APB1 (AHB/4) */
#define APB2_FREQ_HZ       120000000UL  /* 120 MHz APB2 (AHB/2) */
#define PLL1_N              240UL        /* PLL1 multiplier */
#define PLL1_M              4UL          /* PLL1 divider */
#define PLL1_P              2UL          /* PLL1 P divider */
#define PLL1_Q              8UL          /* PLL1 Q divider */

/* ========================================================================== */
/* SPI clock speeds                                                           */
/* ========================================================================== */
#define SPI1_SPEED_HZ      50000000UL   /* FPGA SPI at 50 MHz */
#define SPI2_SPEED_HZ      30000000UL   /* Display SPI at 30 MHz */
#define SPI3_SPEED_HZ      50000000UL   /* Flash SPI at 50 MHz */

/* ========================================================================== */
/* UART baud rates                                                            */
/* ========================================================================== */
#define USART1_BAUD         115200UL     /* Debug console */
#define USART2_BAUD         921600UL     /* BLE module */

/* ========================================================================== */
/* GPIO pin definitions (STM32H743ZIT6 LQFP-144)                             */
/* ========================================================================== */

/* Port A */
#define PIN_ADC_IRQ         0   /* PA0  - FPGA data-ready interrupt (input) */
#define PIN_LNA_EN          1   /* PA1  - LNA enable (output) */
#define PIN_USART2_TX       2   /* PA2  - BLE UART TX (AF7) */
#define PIN_USART2_RX       3   /* PA3  - BLE UART RX (AF7) */
#define PIN_DAC1_OUT         4   /* PA4  - AGC control voltage (analog) */
#define PIN_SPI1_SCK         5   /* PA5  - FPGA SPI clock (AF5) */
#define PIN_SPI1_MISO        6   /* PA6  - FPGA SPI data out (AF5) */
#define PIN_SPI1_MOSI        7   /* PA7  - FPGA SPI data in (AF5) */
#define PIN_MCO1             8   /* PA8  - 8 MHz reference output (AF0) */
#define PIN_USB_VBUS         9   /* PA9  - USB VBUS detect (input) */
#define PIN_USB_ID           10  /* PA10 - USB OTG ID (input) */
#define PIN_USB_DM            11  /* PA11 - USB D- (AF10) */
#define PIN_USB_DP            12  /* PA12 - USB D+ (AF10) */
#define PIN_JTMS              13  /* PA13 - SWDIO (AF0) */
#define PIN_JTCK              14  /* PA14 - SWCLK (AF0) */
#define PIN_FPGA_CDONE        15  /* PA15 - FPGA config done (input) */

/* Port B */
#define PIN_MIXER_EN         0   /* PB0  - Mixer enable (output) */
#define PIN_ATTEN_SEL        1   /* PB1  - Attenuator select (output) */
#define PIN_SPI2_SCK         2   /* PB2  - Display SPI clock (AF5) */
#define PIN_SPI2_MOSI        3   /* PB3  - Display SPI data (AF5) */
#define PIN_LCD_DC            4   /* PB4  - Display D/C# (output) */
#define PIN_LCD_CS            5   /* PB5  - Display CS# (output) */
#define PIN_LCD_RST           6   /* PB6  - Display reset (output) */
#define PIN_TOUCH_IRQ         7   /* PB7  - Touch interrupt (input) */
#define PIN_I2C1_SCL          8   /* PB8  - Touch I2C clock (AF4) */
#define PIN_I2C1_SDA          9   /* PB9  - Touch I2C data (AF4) */
#define PIN_PLL_LE             10  /* PB10 - ADF4351 latch enable (output) */
#define PIN_PLL_CE             11  /* PB11 - ADF4351 chip enable (output) */
#define PIN_SPI3_SCK           12  /* PB12 - Flash SPI clock (AF6) */
#define PIN_SPI3_MISO          13  /* PB13 - Flash SPI data out (AF6) */
#define PIN_SPI3_MOSI          14  /* PB14 - Flash SPI data in (AF6) */
#define PIN_FLASH_CS            15  /* PB15 - Flash CS# (output) */

/* Port C */
#define PIN_FPGA_RST           6   /* PC6  - FPGA reset (output) */
#define PIN_FPGA_CS             7   /* PC7  - FPGA SPI CS# (output) */
#define PIN_TOUCH_CS            8   /* PC8  - Touch CS# (output) */
#define PIN_LED_STATUS          9   /* PC9  - Green status LED (output) */
#define PIN_LED_ERROR           10  /* PC10 - Red error LED (output) */
#define PIN_CHG_STAT1           11  /* PC11 - Charger status 1 (input) */
#define PIN_CHG_STAT2           12  /* PC12 - Charger status 2 (input) */

/* Port D */
#define PIN_FPGA_SCK            0   /* PD0  - FPGA config clock (output) */
#define PIN_FPGA_SI             1   /* PD1  - FPGA config data (output) */
#define PIN_FPGA_SS             2   /* PD2  - FPGA config select (output) */
#define PIN_BAT_SENSE           3   /* PD3  - Battery voltage ADC (analog) */
#define PIN_TEMP_SENSE          4   /* PD4  - Board temperature ADC (analog) */
#define PIN_MUX_SEL0            5   /* PD5  - RF path mux select 0 (output) */
#define PIN_MUX_SEL1            6   /* PD6  - RF path mux select 1 (output) */
#define PIN_MUX_EN              7   /* PD7  - RF path mux enable (output) */
#define PIN_USART1_TX           8   /* PD8  - Debug UART TX (AF7) */
#define PIN_USART1_RX           9   /* PD9  - Debug UART RX (AF7) */

/* Port E */
#define PIN_BTN_CENTER          0   /* PE0  - Center button (input, pull-up) */
#define PIN_BTN_UP              1   /* PE1  - Up button (input, pull-up) */
#define PIN_BTN_DOWN            2   /* PE2  - Down button (input, pull-up) */
#define PIN_BTN_LEFT            3   /* PE3  - Left button (input, pull-up) */
#define PIN_BTN_RIGHT           4   /* PE4  - Right button (input, pull-up) */
#define PIN_BTN_MENU            5   /* PE5  - Menu button (input, pull-up) */

/* ========================================================================== */
/* GPIO port base addresses                                                   */
/* ========================================================================== */
#define GPIOA_BASE         0x58020000UL
#define GPIOB_BASE         0x58020400UL
#define GPIOC_BASE         0x58020800UL
#define GPIOD_BASE         0x58020C00UL
#define GPIOE_BASE         0x58021000UL

#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *) GPIOE_BASE)

/* ========================================================================== */
/* Bit manipulation macros                                                     */
/* ========================================================================== */
#define BIT(n)              (1UL << (n))
#define SET_BIT(reg, bit)   ((reg) |= BIT(bit))
#define CLR_BIT(reg, bit)   ((reg) &= ~BIT(bit))
#define TOG_BIT(reg, bit)   ((reg) ^= BIT(bit))
#define GET_BIT(reg, bit)   (((reg) >> (bit)) & 1UL)
#define WRITE_REG(reg, val) ((reg) = (val))
#define READ_REG(reg)       ((reg))
#define MODIFY_REG(reg, clr, set) ((reg) = ((reg) & ~(clr)) | (set))

/* ========================================================================== */
/* FPGA configuration constants                                               */
/* ========================================================================== */
#define FPGA_CONFIG_SPI_SPEED   25000000UL   /* 25 MHz config SPI */
#define FPGA_CDONE_TIMEOUT_MS  100          /* 100ms CDONE timeout */
#define FPGA_RESET_DELAY_MS     1           /* 1ms reset pulse */

/* ========================================================================== */
/* Display constants                                                          */
/* ========================================================================== */
#define ILI9341_WIDTH          320
#define ILI9341_HEIGHT         240
#define ILI9341_SPI_SPEED      30000000UL   /* 30 MHz display SPI */

/* ========================================================================== */
/* FFT configuration                                                          */
/* ========================================================================== */
#define FFT_MAX_SIZE           8192          /* Maximum FFT size */
#define FFT_MIN_SIZE           256           /* Minimum FFT size */
#define FFT_DEFAULT_SIZE       1024          /* Default FFT size */
#define FFT_BIN_COUNT          FFT_DEFAULT_SIZE
#define FFT_DMA_BUF_SIZE       (FFT_BIN_COUNT * 2)  /* 16-bit bins */

/* ========================================================================== */
/* Sweep configuration                                                        */
/* ========================================================================== */
#define SWEEP_MIN_FREQ_HZ     100000UL       /* 100 kHz */
#define SWEEP_MAX_FREQ_HZ     6000000000UL   /* 6 GHz */
#define SWEEP_DEFAULT_START   88000000UL     /* 88 MHz */
#define SWEEP_DEFAULT_STOP    108000000UL    /* 108 MHz */
#define SWEEP_IF_FREQ_HZ      10700000UL     /* 10.7 MHz IF */
#define SWEEP_ADC_RATE_HZ     61440000UL     /* 61.44 MSPS */
#define SWEEP_MAX_BINS        8192

/* ========================================================================== */
/* Display colors (RGB565)                                                    */
/* ========================================================================== */
#define COLOR_BLACK            0x0000
#define COLOR_WHITE            0xFFFF
#define COLOR_RED              0xF800
#define COLOR_GREEN            0x07E0
#define COLOR_BLUE             0x001F
#define COLOR_CYAN             0x07FF
#define COLOR_MAGENTA          0xF81F
#define COLOR_YELLOW           0xFFE0
#define COLOR_ORANGE           0xFD20
#define COLOR_GRAY             0x8410
#define COLOR_DARK_GRAY        0x4208
#define COLOR_LIGHT_GRAY       0xC618

/* ========================================================================== */
/* USB configuration                                                          */
/* ========================================================================== */
#define VORTEX_VID             0x1209         /* pid.codes */
#define VORTEX_PID             0xVR01         /* Vortex-SDR Rev 1 */
#define USB_PACKET_SIZE        64             /* Full-speed packet size */
#define USB_BULK_SIZE          512            /* High-speed bulk size */

/* ========================================================================== */
/* BLE configuration                                                          */
/* ========================================================================== */
#define BLE_SERVICE_UUID       0xFEF1
#define BLE_CMD_CHAR_UUID      0xFEF2
#define BLE_DATA_CHAR_UUID     0xFEF3
#define BLE_STATUS_CHAR_UUID   0xFEF4
#define BLE_CONFIG_CHAR_UUID   0xFEF5

/* ========================================================================== */
/* Flash storage configuration                                                */
/* ========================================================================== */
#define FLASH_SECTOR_SIZE      4096          /* 4 KB sectors */
#define FLASH_PAGE_SIZE        256           /* 256 byte pages */
#define FLASH_TOTAL_SIZE       (32 * 1024 * 1024)  /* 32 MB */
#define FLASH_LOG_START        (FLASH_TOTAL_SIZE / 2)  /* Start at 16 MB */
#define FLASH_LOG_SECTORS     (FLASH_TOTAL_SIZE / 2 / FLASH_SECTOR_SIZE)

/* ========================================================================== */
/* Battery monitoring                                                          */
/* ========================================================================== */
#define BAT_SENSE_DIVIDER      2             /* Voltage divider ratio */
#define BAT_SENSE_MAX_MV       4200          /* Full charge voltage */
#define BAT_SENSE_MIN_MV       3000          /* Cutoff voltage */
#define BAT_SENSE_WARN_MV      3500          /* Warning voltage */

/* ========================================================================== */
/* System states                                                              */
/* ========================================================================== */
typedef enum {
    SYS_STATE_INIT = 0,
    SYS_STATE_IDLE,
    SYS_STATE_SWEEP_CONTINUOUS,
    SYS_STATE_SWEEP_SINGLE,
    SYS_STATE_ZERO_SPAN,
    SYS_STATE_DATA_LOG,
    SYS_STATE_USB_TRANSFER,
    SYS_STATE_ERROR,
    SYS_STATE_SLEEP
} sys_state_t;

/* ========================================================================== */
/* Sweep mode enumeration                                                     */
/* ========================================================================== */
typedef enum {
    SWEEP_MODE_CONTINUOUS = 0,
    SWEEP_MODE_SINGLE,
    SWEEP_MODE_ZERO_SPAN,
    SWEEP_MODE_MAX_HOLD,
    SWEEP_MODE_AVERAGE
} sweep_mode_t;

/* ========================================================================== */
/* FFT window enumeration                                                     */
/* ========================================================================== */
typedef enum {
    WINDOW_RECTANGULAR = 0,
    WINDOW_HANN,
    WINDOW_BLACKMAN_HARRIS,
    WINDOW_FLAT_TOP,
    WINDOW_KAISER
} window_t;

/* ========================================================================== */
/* Sweep state structure                                                      */
/* ========================================================================== */
typedef struct {
    uint64_t start_freq;         /* Start frequency in Hz */
    uint64_t stop_freq;          /* Stop frequency in Hz */
    uint32_t rbw;                /* Resolution bandwidth in Hz */
    uint32_t fft_size;           /* FFT size (256-8192) */
    window_t window;             /* Window function */
    uint8_t averaging;           /* Number of averages (1-32) */
    uint8_t attenuation;         /* Attenuation in dB (0, 10, 20) */
    int8_t agc_level;            /* AGC level in dB (-60 to 0) */
    sweep_mode_t mode;           /* Sweep mode */
    sys_state_t state;           /* System state */
    uint16_t ref_level;          /* Reference level in dBm * 10 */
    uint16_t scale;              /* Scale in dB/div * 10 */
    uint32_t sweep_count;        /* Number of completed sweeps */
    uint32_t sweep_time_ms;      /* Last sweep time in ms */
    uint8_t pll_locked;          /* PLL lock status */
    uint8_t adc_overrange;       /* ADC overrange flag */
    uint16_t battery_mv;         /* Battery voltage in mV */
    int8_t temperature;          /* Board temperature in °C */
} sweep_state_t;

/* ========================================================================== */
/* FFT bin data structure                                                     */
/* ========================================================================== */
typedef struct {
    int16_t bins[FFT_MAX_SIZE];  /* Power in dBm * 10 (e.g., -500 = -50.0 dBm) */
    uint16_t num_bins;           /* Number of valid bins */
    uint64_t center_freq;       /* Center frequency of first bin */
    uint32_t bin_bw;            /* Bandwidth per bin in Hz */
    uint32_t timestamp;         /* Timestamp in ms */
    uint8_t flags;              /* Status flags */
} fft_data_t;

/* ========================================================================== */
/* Peak detection structure                                                    */
/* ========================================================================== */
#define MAX_PEAKS 10

typedef struct {
    uint64_t freq;              /* Frequency in Hz */
    int16_t power;              /* Power in dBm * 10 */
    uint16_t bin_index;         /* Bin index in FFT data */
} peak_t;

typedef struct {
    peak_t peaks[MAX_PEAKS];    /* Up to 10 detected peaks */
    uint8_t num_peaks;          /* Number of peaks found */
} peak_data_t;

/* ========================================================================== */
/* Display buffer structure                                                   */
/* ========================================================================== */
typedef struct {
    uint16_t spectrum_buf[ILI9341_WIDTH];  /* Spectrum trace buffer (pixels) */
    uint16_t waterfall_buf[ILI9341_WIDTH];  /* Waterfall line buffer (colors) */
    uint8_t update_flags;                   /* Bit flags for display update */
    #define DISP_UPDATE_SPECTRUM   0x01
    #define DISP_UPDATE_WATERFALL  0x02
    #define DISP_UPDATE_STATUS     0x04
    #define DISP_UPDATE_MARKERS    0x08
    #define DISP_UPDATE_ALL        0x0F
} display_buf_t;

/* ========================================================================== */
/* Button definitions                                                         */
/* ========================================================================== */
#define BTN_CENTER_BIT    0
#define BTN_UP_BIT        1
#define BTN_DOWN_BIT      2
#define BTN_LEFT_BIT      3
#define BTN_RIGHT_BIT     4
#define BTN_MENU_BIT      5

#define BTN_CENTER        BIT(BTN_CENTER_BIT)
#define BTN_UP            BIT(BTN_UP_BIT)
#define BTN_DOWN          BIT(BTN_DOWN_BIT)
#define BTN_LEFT          BIT(BTN_LEFT_BIT)
#define BTN_RIGHT         BIT(BTN_RIGHT_BIT)
#define BTN_MENU          BIT(BTN_MENU_BIT)

/* ========================================================================== */
/* Command protocol definitions                                                */
/* ========================================================================== */
#define CMD_GET_STATUS     0x01
#define CMD_START_SWEEP    0x02
#define CMD_STOP_SWEEP     0x03
#define CMD_SET_FREQ       0x04
#define CMD_SET_FFT_SIZE   0x05
#define CMD_SET_WINDOW     0x06
#define CMD_SET_AVERAGING  0x07
#define CMD_SET_ATTEN      0x08
#define CMD_SET_AGC        0x09
#define CMD_GET_PEAKS      0x0A
#define CMD_SET_MODE       0x0B
#define CMD_LOG_START      0x0C
#define CMD_LOG_STOP       0x0D
#define CMD_LOG_READ       0x0E
#define CMD_RESET          0x0F
#define CMD_SET_REF_LEVEL  0x10
#define CMD_SET_SCALE      0x11

#define RSP_OK             0x00
#define RSP_ERROR           0x01
#define RSP_BUSY            0x02
#define RSP_INVALID_PARAM   0x03

/* ========================================================================== */
/* Version information                                                        */
/* ========================================================================== */
#define FIRMWARE_VERSION_MAJOR   1
#define FIRMWARE_VERSION_MINOR   0
#define FIRMWARE_VERSION_PATCH   0
#define HARDWARE_VERSION         1

#endif /* VORTEX_SDR_BOARD_H */