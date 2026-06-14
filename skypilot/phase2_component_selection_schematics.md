# SkyPilot — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| Ref | Part Number | Description | Qty | Package | Manufacturer | Unit Cost |
|---|---|---|---|---|---|---|
| U1 | STM32H743VIT6 | Flight processor, 480MHz Cortex-M7, 2MB Flash, 1MB RAM | 1 | LQFP-100 | STMicroelectronics | $12.80 |
| U2 | ICM-42688-P | 6-axis IMU (3-axis gyro + 3-axis accel), 32kHz ODR | 1 | LGA-14 (2.5×3mm) | TDK/InvenSense | $4.20 |
| U3 | BMI270 | 6-axis IMU (secondary/redundant), 6.4kHz ODR | 1 | LGA-14 (2.5×3mm) | Bosch | $3.10 |
| U4 | BMP390 | Barometric pressure sensor, ±0.5hPa | 1 | LGA-10 (2×2mm) | Bosch | $2.80 |
| U5 | SAM-M10Q | Multi-constellation GNSS receiver (GPS/Galileo/GLONASS/BeiDou) | 1 | LCC-24 (9.7×10.1mm) | u-blox | $14.50 |
| U6 | LARA-R6401 | 4G LTE Cat.4 modem (uplink 50Mbps, downlink 150Mbps) | 1 | LGA-171 (16×26mm) | u-blox | $22.00 |
| U7 | ESP32-H2 | Companion computer, 96MHz RISC-V, 802.15.4 | 1 | QFN-32 (5×5mm) | Espressif | $1.80 |
| U8 | TPS65294 | 5V/3A buck-boost BEC, 7-26V input | 1 | VQFN-24 (4×4mm) | Texas Instruments | $3.40 |
| U9 | TLV75533PDBV | 3.3V/1.5A LDO | 1 | SOT-23-5 | Texas Instruments | $0.30 |
| U10 | TLV75518PDBV | 1.8V/500mA LDO | 1 | SOT-23-5 | Texas Instruments | $0.30 |
| U11 | RV-3032-7 | Real-time clock with crystal, I2C | 1 | SOC-8 (3.7×2.5mm) | Micro Crystal | $1.90 |
| U12 | W25Q128JVS | 128Mbit SPI NOR flash (OSD/config) | 1 | SOIC-8 | Winbond | $0.80 |
| U13 | TPS22917 | Load switch, LTE modem power enable | 1 | SOT-23-6 | Texas Instruments | $0.45 |
| Y1 | ECS-2508CS-200 | 24MHz crystal, STM32H7 HSE | 1 | 3.2×2.5mm | ECS | $0.30 |
| Y2 | ABS07-32.768KHZ | 32.768kHz crystal, STM32H7 LSE | 1 | 3.2×1.5mm | Abracon | $0.20 |
| Y3 | ABS07-32.768KHZ-T | 32.768kHz crystal, RTC | 1 | 3.2×1.5mm | Abracon | $0.20 |
| Y4 | EFR-SMD-26MHZ | 26MHz TCXO, LARA-R6 | 1 | 3.2×2.5mm | u-blox | $0.50 |
| FB1-FB4 | BLM21PG221SN1 | Ferrite bead, 220Ω @100MHz, 2A | 4 | 0805 | Murata | $0.05 ea |
| C1-C40 | Various | Decoupling/bulk capacitors (see §4) | — | 0402/0603/0805 | Various | — |
| R1-R60 | Various | Resistors (see §5) | — | 0402 | Various | — |
| J1 | 10137065-002 | USB-C 2.0 receptacle, SMT | 1 | — | Amphenol | $0.80 |
| J2 | S4B-ZR-SM4A-TF | 40-pin JST-SH 0.8mm housing | 1 | — | JST | $1.20 |
| J3 | WM5559-ND | u.FL (IPEX) coaxial, LTE antenna | 1 | — | IPEX | $0.60 |
| J4 | WM5559-ND | u.FL (IPEX) coaxial, GNSS antenna | 1 | — | IPEX | $0.60 |
| SW1 | SKQGAFE010 | Tactile reset button | 1 | 3×4mm | ALPS | $0.10 |
| SW2 | SKQGAFE010 | Tactile boot button | 1 | 3×4mm | ALPS | $0.10 |
| LED1 | LTST-C191KRKT | Red LED (power) | 1 | 0603 | Lite-On | $0.03 |
| LED2 | LTST-C191KGKT | Green LED (status) | 1 | 0603 | Lite-On | $0.03 |
| LED3 | LTST-C191KBKT | Blue LED (LTE) | 1 | 0603 | Lite-On | $0.03 |

**Total BOM cost (estimated): ~$72.00**

## 2. Pinout Tables

### 2.1 STM32H743VIT6 — Pin Assignments (LQFP-100)

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | PE2 | TRACECLK / SPI4_SCK | U12 pin6 (W25Q128 SCK) |
| 2 | PE3 | TRACED0 / SPI4_MISO | U12 pin2 (W25Q128 DO) |
| 3 | PE4 | TRACED1 / SPI4_MOSI | U12 pin5 (W25Q128 DI) |
| 4 | PE5 | TRACED2 / SPI4_NSS | U12 pin1 (W25Q128 CS) |
| 6 | PE7 | TIM1_ETR | LED2 (Green status) |
| 7 | PE8 | TIM1_CH1 | DShot Motor 1 |
| 8 | PE9 | TIM1_CH2 | DShot Motor 2 |
| 9 | PE10 | TIM1_CH3 | DShot Motor 3 |
| 11 | PE11 | TIM1_CH4 | DShot Motor 4 |
| 12 | PE12 | TIM1_CH1N | DShot Motor 5 |
| 13 | PE13 | TIM1_CH2N | DShot Motor 6 |
| 14 | PE14 | TIM1_CH3N | DShot Motor 7 |
| 16 | PB0 | TIM3_CH3 | DShot Motor 8 |
| 17 | PB1 | TIM3_CH4 | DShot Motor 9 |
| 21 | PA0 | ADC1_INP16 (Battery) | R5/R6 voltage divider |
| 22 | PA1 | ADC1_INP17 (Current) | Current sense amp |
| 26 | PA4 | SPI1_NSS (ICM-42688) | U2 pin12 (CS) |
| 27 | PA5 | SPI1_SCK | U2 pin13 (SCK) |
| 28 | PA6 | SPI1_MISO | U2 pin11 (SDO) |
| 29 | PA7 | SPI1_MOSI | U2 pin14 (SDI) |
| 30 | PC4 | SPI2_NSS (BMI270) | U3 pin4 (CSB) |
| 31 | PC5 | SPI2_SCK | U3 pin5 (SCK) |
| 32 | PB13 | SPI2_MISO | U3 pin3 (SDO) |
| 33 | PB14 | SPI2_MOSI | U3 pin2 (SDI) |
| 35 | PA8 | I2C3_SCL | — (reserved) |
| 36 | PA9 | I2C3_SDA | — (reserved) |
| 37 | PA10 | TIM8_CH1 | DShot Motor 10 |
| 38 | PA11 | USB_DM | J1 (USB-C D-) |
| 39 | PA12 | USB_DP | J1 (USB-C D+) |
| 42 | PA13 | SWDIO | J2 pin1 (SWD) |
| 43 | PA14 | SWCLK | J2 pin2 (SWD) |
| 44 | PA15 | SPI1_NSS (alternate) | U2 pin12 (CS) alt |
| 46 | PB3 | SPI1_SCK (alternate) | — |
| 48 | PB5 | I2C1_SDA | U4 pin5 (BMP390 SDA), U11 pin3 (RTC SDA) |
| 49 | PB6 | I2C1_SCL | U4 pin6 (BMP390 SCL), U11 pin4 (RTC SCL) |
| 50 | PB7 | UART1_RX | U5 pin15 (GNSS TXD) |
| 51 | PB8 | UART1_TX | U5 pin14 (GNSS RXD) |
| 52 | PB9 | FDCAN1_TX | J2 pin31 (CAN H) |
| 53 | PB10 | TIM2_CH3 | DShot Motor 11 |
| 54 | PB11 | TIM2_CH4 | DShot Motor 12 |
| 55 | PB12 | SPI2_NSS (alt) | — |
| 56 | PB15 | UART4_TX | U7 pin21 (ESP32-H2 RX) |
| 58 | PC6 | TIM8_CH1 | DShot Motor 10 (alt) |
| 59 | PC7 | TIM8_CH2 | DShot Motor 11 (alt) |
| 60 | PC8 | TIM8_CH3 | DShot Motor 12 (alt) |
| 61 | PC9 | UART8_TX | U6 pin97 (LARA-R6 RXD) |
| 62 | PA0_ALT | GPIO_Output | LTE_PWR_ON (U13 EN) |
| 63 | PC10 | UART8_RX | U6 pin98 (LARA-R6 TXD) |
| 64 | PC11 | SPI3_MISO | — (reserved) |
| 65 | PC12 | SPI3_MOSI | — (reserved) |
| 66 | PD0 | FDCAN1_RX | J2 pin32 (CAN L) |
| 67 | PD1 | GPIO_Output | LED3 (LTE status) |
| 68 | PD2 | GPIO_Output | LED1 (Power) |
| 69 | PD3 | GPIO_Input | SW1 (Reset) |
| 70 | PD4 | GPIO_Input | SW2 (Boot) |
| 71 | PD5 | USART2_TX | Debug UART TX |
| 72 | PD6 | USART2_RX | Debug UART RX |
| 78 | PD7 | GPIO_Output | GNSS_RST (U5 RESET_N) |
| 79 | PB3_ALT | GPIO_Input | ICM-42688 INT1 | U2 pin9 (INT1) |
| 80 | PB4 | GPIO_Input | BMI270 INT1 | U3 pin7 (INT1) |
| 83 | PD8 | UART4_RX | U7 pin22 (ESP32-H2 TX) |
| 84 | PD9 | GPIO_Output | LTE_STATUS (U6 GPIO) |
| 86 | PD11 | GPIO_Input | ICM-42688 INT2 | U2 pin8 (INT2) |
| 87 | PD12 | GPIO_Input | BMI270 INT2 | U3 pin8 (INT2) |
| 90 | PD14 | GPIO_Output | LTE_RTS (U6 RTS) | U6 pin96 |
| 91 | PD15 | GPIO_Input | LTE_CTS (U6 CTS) | U6 pin95 |
| 93 | PE0 | GPIO_Input | BMP390 INT | U4 pin7 (INT) |
| 94 | PE1 | GPIO_Output | SPI4_NSS | U12 pin1 (W25Q128 CS) |
| 96 | VBAT | Battery backup | CR1225 coin cell |
| 97 | VSSA | Analog ground | GND |
| 98 | VREF+ | ADC reference | 3.3V (filtered) |
| 100 | VDDA | Analog supply | 3.3V (filtered) |

### 2.2 ICM-42688-P — Pin Assignments (LGA-14)

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | VDDIO | I/O supply (3.3V) | 3V3 net |
| 2 | SCL / SDO_M | SPI SCK (when AD0=0) | PA5 (SPI1_SCK) |
| 3 | SDA / SDI | SPI MOSI | PA7 (SPI1_MOSI) |
| 4 | AD0 / SDO | SPI MISO | PA6 (SPI1_MISO) |
| 5 | nCS | SPI chip select | PA4 (SPI1_NSS) |
| 6 | INT1 | Interrupt 1 | PB3 (GPIO) |
| 7 | INT2 | Interrupt 2 | PD11 (GPIO) |
| 8 | GND | Ground | GND |
| 9 | RESV | Reserved (tie to GND) | GND |
| 10 | GND | Ground | GND |
| 11 | VDD | Core supply (1.8V) | 1V8 net |
| 12 | RESV | Reserved (tie to GND) | GND |
| 13 | GND | Ground | GND |
| 14 | VDDIO | I/O supply (3.3V) | 3V3 net |

### 2.3 BMI270 — Pin Assignments (LGA-14)

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | VDDIO | I/O supply (3.3V) | 3V3 net |
| 2 | SDI | SPI MOSI | PB14 (SPI2_MOSI) |
| 3 | SDO | SPI MISO | PB13 (SPI2_MISO) |
| 4 | CSB | SPI chip select | PC4 (SPI2_NSS) |
| 5 | SCK | SPI clock | PC5 (SPI2_SCK) |
| 6 | INT2 | Interrupt 2 | PD12 (GPIO) |
| 7 | INT1 | Interrupt 1 | PB4 (GPIO) |
| 8 | GND | Ground | GND |
| 9 | GND | Ground | GND |
| 10 | VDD | Core supply (1.8V) | 1V8 net |
| 11 | GND | Ground | GND |
| 12 | RESV | Reserved | GND |
| 13 | GND | Ground | GND |
| 14 | VDDIO | I/O supply (3.3V) | 3V3 net |

### 2.4 LARA-R6401 — Key Pin Assignments (LGA-171)

| Pin | Name | Function | Connected To |
|---|---|---|---|
| 1 | GND | Ground | GND |
| 33 | VCC | Main supply (3.4-4.2V) | VBAT_DIRECT (via U13 load switch) |
| 36 | V_INT | Internal LDO output (1.8V) | — (floating) |
| 95 | CTS | UART CTS | PD15 (GPIO) |
| 96 | RTS | UART RTS | PD14 (GPIO) |
| 97 | TXD | UART transmit | PC9 (UART8_RX via cross) |
| 98 | RXD | UART receive | PC10 (UART8_TX via cross) |
| 100 | PWR_ON | Power-on control | PA0 (GPIO) |
| 103 | VCC | Main supply | VBAT_DIRECT |
| 110 | RESET_N | Reset | 3V3 via 10kΩ pull-up |
| 121 | GPIO1 | Status LED | PD9 (GPIO) |
| 140 | RF_ANT | Antenna | J3 (u.FL → LTE antenna) |
| 155 | USB_DP | USB D+ | — (not used, tied to GND via 1MΩ) |
| 156 | USB_DM | USB D- | — (not used, tied to GND via 1MΩ) |
| 167 | GND | Ground | GND |

## 3. Netlists (Source → Component → Destination)

### 3.1 SPI1 Bus (ICM-42688-P)

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| SPI1_SCK | STM32 PA5 | → U2 pin2 (SCL/SCK) | ICM-42688 SCK |
| SPI1_MISO | U2 pin4 (SDO) | → STM32 PA6 | ICM-42688 SDO |
| SPI1_MOSI | STM32 PA7 | → U2 pin3 (SDA/SDI) | ICM-42688 SDI |
| SPI1_NSS | STM32 PA4 | → U2 pin5 (nCS) | ICM-42688 CS |
| IMU1_INT1 | U2 pin6 (INT1) | → STM32 PB3 | ICM-42688 INT1 |
| IMU1_INT2 | U2 pin7 (INT2) | → STM32 PD11 | ICM-42688 INT2 |

### 3.2 SPI2 Bus (BMI270)

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| SPI2_SCK | STM32 PC5 | → U3 pin5 (SCK) | BMI270 SCK |
| SPI2_MISO | U3 pin3 (SDO) | → STM32 PB13 | BMI270 SDO |
| SPI2_MOSI | STM32 PB14 | → U3 pin2 (SDI) | BMI270 SDI |
| SPI2_NSS | STM32 PC4 | → U3 pin4 (CSB) | BMI270 CS |
| IMU2_INT1 | U3 pin7 (INT1) | → STM32 PB4 | BMI270 INT1 |
| IMU2_INT2 | U3 pin8 (INT2) | → STM32 PD12 | BMI270 INT2 |

### 3.3 I2C1 Bus (BMP390 + RV-3032-7)

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| I2C1_SCL | STM32 PB6 | → U4 pin6 (SCL) | BMP390 SCL |
| I2C1_SCL | STM32 PB6 | → U11 pin4 (SCL) | RV-3032 SCL |
| I2C1_SDA | STM32 PB5 | → U4 pin5 (SDA) | BMP390 SDA |
| I2C1_SDA | STM32 PB5 | → U11 pin3 (SDA) | RV-3032 SDA |
| BARO_INT | U4 pin7 (INT) | → STM32 PE0 | BMP390 INT |

### 3.4 UART8 Bus (LARA-R6)

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| UART8_TX | STM32 PC10 | → U6 pin98 (RXD) | LARA-R6 RX |
| UART8_RX | U6 pin97 (TXD) | → STM32 PC9 | LARA-R6 TX |
| UART8_RTS | STM32 PD14 | → U6 pin96 (RTS) | LARA-R6 RTS |
| UART8_CTS | U6 pin95 (CTS) | → STM32 PD15 | LARA-R6 CTS |
| LTE_PWR_ON | STM32 PA0 | → U6 pin100 (PWR_ON) | LARA-R6 PWR_ON |
| LTE_STATUS | U6 pin121 (GPIO1) | → STM32 PD9 | LARA-R6 STATUS |

### 3.5 UART1 Bus (SAM-M10Q)

| Net Name | Source Pin | Component | Dest Pin |
|---|---|---|---|
| GNSS_TX | STM32 PB8 | → U5 pin14 (RXD) | SAM-M10Q RX |
| GNSS_RX | U5 pin15 (TXD) | → STM32 PB7 | SAM-M10Q TX |
| GNSS_RST | STM32 PD7 | → U5 pin11 (RESET_N) | SAM-M10Q RESET |

### 3.6 Power Netlist

| Net Name | Source | Voltage | Current | Dest |
|---|---|---|---|---|
| VBAT_RAW | Battery connector | 7-26V | — | U8 pin1 (TPS65294 VIN) |
| 5V_BECP | U8 pin9 (VOUT) | 5.0V | 3A max | U9 pin1 (TLV75533 VIN), U13 pin1 (load switch), USB VBUS |
| 3V3 | U9 pin5 (VOUT) | 3.3V | 1.5A max | STM32 VDD, IMU VDDIO, BMP390, RV-3032, SPI flash, I2C pull-ups |
| 1V8 | U10 pin5 (VOUT) | 1.8V | 500mA | IMU VDD core, STM32 VDDA1V8 (SMPS), ESP32-H2 |
| VBAT_DIRECT | Battery connector | 7-26V | — | U6 pin33/103 (LARA-R6 VCC via U13) |
| VBAT_RTC | CR1225 coin cell | 3.0V | — | STM32 VBAT pin |
| VREF+ | 3V3 via ferrite | 3.3V | — | STM32 VREF+, ADC reference |

## 4. Decoupling Networks

### 4.1 STM32H743VIT6

| Pin Group | Capacitors | Notes |
|---|---|---|
| VDD (1.8V core) | 4× 100nF (0402, X5R) + 1× 4.7µF (0603, X5R) | Place near VDD pins |
| VDDIO (3.3V) | 6× 100nF (0402, X5R) + 2× 1µF (0402, X5R) | One per VDDIO pin pair |
| VDDA (analog) | 100nF (0402) + 1µF (0402) + 10µF (0603, X5R) | LC filter from 3V3 |
| VBAT (RTC) | 100nF (0402) | Near VBAT pin |

### 4.2 ICM-42688-P

| Pin | Capacitors | Notes |
|---|---|---|
| VDD (1.8V) | 100nF (0402) + 1µF (0402) | Place within 2mm |
| VDDIO (3.3V) | 100nF (0402) | Place within 2mm |

### 4.3 BMI270

| Pin | Capacitors | Notes |
|---|---|---|
| VDD (1.8V) | 100nF (0402) + 1µF (0402) | Place within 2mm |
| VDDIO (3.3V) | 100nF (0402) | Place within 2mm |

### 4.4 LARA-R6401

| Pin | Capacitors | Notes |
|---|---|---|
| VCC (3.4-4.2V) | 4× 100nF (0402) + 2× 10µF (0603) + 1× 100µF (1206, X5R) | Critical: burst current 2A |
| V_INT | 100nF (0402) | Internal LDO bypass |

### 4.5 BMP390

| Pin | Capacitors | Notes |
|---|---|---|
| VDDIO (3.3V) | 100nF (0402) | |
| VDD (3.3V) | 100nF (0402) + 1µF (0402) | |

### 4.6 SAM-M10Q

| Pin | Capacitors | Notes |
|---|---|---|
| VCC (3.3V) | 100nF (0402) + 10µF (0603) | |
| V_BCKP | 100nF (0402) | Backup supply bypass |
| VCC_RF | 10nF (0402) + 1µF (0402) | RF LDO output |

## 5. Impedance-Matched Pairs

| Net Pair | Impedance | Tolerance | Notes |
|---|---|---|---|
| USB_DP / USB_DM | 90Ω differential | ±10% | To USB-C connector |
| LARA-R6 RF trace | 50Ω single-ended | ±5Ω | To u.FL connector |
| SAM-M10Q RF trace | 50Ω single-ended | ±5Ω | To u.FL connector |
| SPI1 (SCK/MISO/MOSI) | 50Ω single-ended | ±15% | Length-matched within 5mm |
| SPI2 (SCK/MISO/MOSI) | 50Ω single-ended | ±15% | Length-matched within 5mm |

## 6. Pull-Up/Pull-Down Values

| Net | Component | Value | Notes |
|---|---|---|---|
| I2C1_SCL | R7 | 4.7kΩ to 3V3 | I2C pull-up |
| I2C1_SDA | R8 | 4.7kΩ to 3V3 | I2C pull-up |
| USB_DP | R9 | 1.5kΩ to 3V3 | USB device connect pull-up (via PA12) |
| SPI1_NSS | R10 | 10kΩ to 3V3 | CS pull-up (idle) |
| SPI2_NSS | R11 | 10kΩ to 3V3 | CS pull-up (idle) |
| SPI4_NSS | R12 | 10kΩ to 3V3 | CS pull-up (idle) |
| LARA-R6 RESET_N | R13 | 10kΩ to VBAT_DIRECT | Modem reset pull-up |
| SAM-M10Q RESET_N | R14 | 10kΩ to 3V3 | GNSS reset pull-up |
| BOOT0 | R15 | 10kΩ to GND | Boot from flash (default) |
| SW1 (Reset) | R16 | 100kΩ to 3V3 | Active-low reset pull-up |
| SW2 (Boot) | R17 | 100kΩ to GND | Active-high boot pull-down |
| LARA-R6 PWR_ON | R18 | 100kΩ to GND | Power-on control pull-down |
| USB_CC1/CC2 | R19/R20 | 5.1kΩ to GND | USB-C default source (5V1A) |
| Battery divider | R5/R6 | 10kΩ/1.5kΩ | 14V→3.28V for ADC (ratio 7.33:1) |