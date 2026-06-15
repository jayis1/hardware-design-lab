# PicoRadar — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| Ref | Part Number | Description | Package | Qty | Unit Cost | Notes |
|---|---|---|---|---|---|---|
| U1 | IWR6843AOP | 60 GHz FMCW radar SoC, 3TX/4RX, AiP | 0.65-mm BGA (88-pin) | 1 | $28.00 | TI mmWave |
| U2 | STM32H743VIT6 | Cortex-M7 @ 480 MHz, 1 MB SRAM | LQFP-100 | 1 | $12.50 | ST host MCU |
| U3 | ESP32-C3-MINI-1 | WiFi 4 + BLE 5 module, 4 MB flash | Module | 1 | $3.20 | Espressif AT firmware |
| U4 | LAN8720A | 10/100 Ethernet PHY | QFN-24 | 1 | $1.80 | Microchip |
| U5 | TPS65263 | 3-buck + 2-LDO PMIC | VQFN-32 | 1 | $2.90 | TI power |
| U6 | ICM-42688-P | 6-axis IMU (accel + gyro) | LGA-14 | 1 | $3.50 | TDK InvenSense |
| U7 | SH1106 | 1.3" 128×64 OLED (module) | Module (I2C) | 1 | $2.80 | Generic |
| U8 | MX25L25645G | 256 Mb QSPI flash | SOP-8 | 1 | $1.40 | Macronix |
| U9 | W25Q128JV | 128 Mb SPI flash (radar) | SOP-8 | 1 | $0.80 | Winbond |
| U10 | TPS2378 | PoE PD controller | VSSOP-10 | 1 | $1.50 | TI |
| Y1 | ABM8-40.000MHZ | 40 MHz TCXO, ±10 ppm | 3.2×2.5 mm | 1 | $1.20 | Abracon |
| Y2 | ECS-80-18-5 | 8 MHz crystal, ±30 ppm | HC49/S | 1 | $0.30 | ECS |
| Y3 | ECS-25-20-5 | 25 MHz crystal, ±50 ppm | 3.2×2.5 mm | 1 | $0.40 | ECS |
| Y4 | ABM8-40.000MHZ-B4 | 40 MHz crystal (ESP32) | 3.2×2.5 mm | 1 | $0.30 | Abracon |
| J1 | USB-C (16-pin) | USB 2.0 Type-C receptacle | SMD | 1 | $0.80 | GCT USB4105 |
| J2 | RJ45 + magnetics | 10/100 with integrated magnetics | SMD | 1 | $2.10 | Pulse J0014D21 |
| J3 | 10-pin ARM SWD | 1.27 mm pitch debug header | Through-hole | 1 | $0.40 | Samtec |
| J4 | 4-pin JST-PH | I2C expansion header | SMD | 1 | $0.20 | JST |
| D1 | USBLC6-2SC6 | USB ESD protection | SOT-23-6 | 1 | $0.40 | ST |
| D2 | PRTR5V0U2X | 5 V ESD clamp | SOT-143 | 1 | $0.20 | NXP |
| L1 | 74437368010 | 10 µH, 3.5 A shielded inductor | 7.3×7.3 mm | 1 | $0.80 | Würth |
| L2 | 74437368010 | 10 µH, 3.5 A shielded inductor | 7.3×7.3 mm | 1 | $0.80 | Würth |
| L3 | 74437368010 | 10 µH, 3.5 A shielded inductor | 7.3×7.3 mm | 1 | $0.80 | Würth |
| C1–C8 | CL05B104KO5NNNC | 100 nF, 25 V, X7R, 0402 | 0402 | 8 | $0.005 | Decoupling |
| C9–C16 | GRM155R60J475ME01D | 4.7 µF, 6.3 V, X5R, 0402 | 0402 | 8 | $0.02 | Bulk decoupling |
| C17 | GRM21BR61C226ME44L | 22 µF, 16 V, X5R, 0805 | 0805 | 1 | $0.10 | Input cap |
| R1 | RC0402FR-071KL | 1 kΩ, 1%, 0402 | 0402 | 1 | $0.005 | Pull-up |
| R2–R5 | RC0402FR-0710KL | 10 kΩ, 1%, 0402 | 0402 | 4 | $0.005 | Pull-ups/downs |
| R6–R9 | RC0402FR-0749R9L | 49.9 Ω, 1%, 0402 | 0402 | 4 | $0.005 | USB D+/D– term |
| R10 | RC0402FR-0749R9L | 49.9 Ω, 1%, 0402 | 0402 | 1 | $0.005 | RMII refclk term |
| FB1–FB3 | BLM15PG121SN1D | 120 Ω @ 100 MHz, 0402 | 0402 | 3 | $0.01 | Ferrite beads |

**Total BOM (1K units): ~$82.60**

## 2. IWR6843AOP Pinout & Connections

### 2.1 Power Pins

| Pin | Name | Connection | Notes |
|---|---|---|---|
| A1 | VDDIN | TPS65263 BUCK2 (1.8 V) | Digital I/O supply |
| A2 | VDD | TPS65263 BUCK3 (1.2 V) | Core supply |
| A3 | VDDA_RF1 | TPS65263 LDO1 (1.0 V) | RF LDO output |
| A4 | VDDA_RF2 | TPS65263 LDO2 (3.0 V) | Analog supply |
| B1 | VSS | GND | Ground |
| B2 | VSS | GND | Ground |
| C1 | VDDIO | TPS65263 BUCK1 (3.3 V) | I/O supply |

### 2.2 Host Interface Pins

| Pin | Name | Direction | STM32H7 Pin | Function |
|---|---|---|---|---|
| D1 | UART_TX | O | PA10 (UART1_RX) | Radar TX → Host RX |
| D2 | UART_RX | I | PA9 (UART1_TX) | Host TX → Radar RX |
| E1 | UART_CTS | I | PA11 | Host CTS (flow control) |
| E2 | UART_RTS | O | PA12 | Radar RTS (flow control) |
| F1 | HOST_INTR | O | PC6 | Radar→Host interrupt |
| F2 | NRST | I | PD0 | Host-controlled reset |
| G1 | SYNC_IN | I | PD1 | Frame sync input |
| G2 | SYNC_OUT | O | PD2 | Frame sync output |

### 2.3 SPI Flash Interface (IWR6843's dedicated bus)

| Pin | Name | Direction | W25Q128 Pin | Function |
|---|---|---|---|---|
| H1 | SPI0_CLK | O | Pin 6 (CLK) | SPI clock |
| H2 | SPI0_CSz | O | Pin 1 (CSz) | Chip select |
| J1 | SPI0_MOSI | O | Pin 5 (DI) | Data to flash |
| J2 | SPI0_MISO | I | Pin 2 (DO) | Data from flash |

### 2.4 Boot Configuration Pins

| Pin | Name | Pull | Value | Meaning |
|---|---|---|---|---|
| K1 | BOOT0 | 10 kΩ to GND | 0 | SPI flash boot |
| K2 | BOOT1 | 10 kΩ to GND | 0 | Normal mode |
| L1 | SOP0 | 10 kΩ to 3.3 V | 1 | Functional mode |
| L2 | SOP1 | 10 kΩ to GND | 0 | No debug SOP |
| M1 | SOP2 | 10 kΩ to GND | 0 | — |

## 3. STM32H743VIT6 Pinout & Connections

### 3.1 Power Pins

| Pin Group | Supply | Source | Decoupling |
|---|---|---|---|
| VDD (1.2 V core) | 1.2 V | TPS65263 BUCK3 | 100 nF + 4.7 µF per pin |
| VDD3V3 (I/O) | 3.3 V | TPS65263 BUCK1 | 100 nF + 4.7 µF per pin |
| VDDA (analog) | 3.3 V | BUCK1 via ferrite | 100 nF + 1 µF |
| VBAT | 3.3 V | BUCK1 | 100 nF |
| VSS | GND | Ground plane | — |

### 3.2 Peripheral Pin Assignments

| STM32 Pin | Peripheral | Direction | Connected To | Alt Func |
|---|---|---|---|---|
| PA9 | USART1_TX | O | IWR6843 UART_RX (D2) | AF7 |
| PA10 | USART1_RX | I | IWR6843 UART_TX (D1) | AF7 |
| PA11 | USART1_CTS | O | IWR6843 UART_CTS (E1) | AF7 |
| PA12 | USART1_RTS | I | IWR6843 UART_RTS (E2) | AF7 |
| PB10 | USART2_TX | O | ESP32-C3 RXD0 | AF4 |
| PB11 | USART2_RX | I | ESP32-C3 TXD0 | AF4 |
| PA2 | ETH_RMII_REFCLK | I | LAN8720A CLKIN/REFCLK0 | AF11 |
| PA1 | ETH_RMII_MDC | O | LAN8720A MDC | AF11 |
| PA7 | ETH_RMII_MDIO | B | LAN8720A MDIO | AF11 |
| PC1 | ETH_RMII_MDC | O | LAN8720A MDC | AF11 |
| PC4 | ETH_RMII_RXD0 | I | LAN8720A RXD0 | AF11 |
| PC5 | ETH_RMII_RXD1 | I | LAN8720A RXD1 | AF11 |
| PB0 | ETH_RMII_RXDV | I | LAN8720A CRS/DV | AF11 |
| PB13 | ETH_RMII_TXD0 | O | LAN8720A TXD0 | AF11 |
| PB14 | ETH_RMII_TXD1 | O | LAN8720A TXD1 | AF11 |
| PB11 | ETH_RMII_TXEN | O | LAN8720A TXEN | AF11 |
| PA5 | SPI1_SCK | O | ICM-42688 SCK | AF5 |
| PA6 | SPI1_MISO | I | ICM-42688 SDO | AF5 |
| PA7 | SPI1_MOSI | O | ICM-42688 SDI | AF5 |
| PA4 | SPI1_NSS | O | ICM-42688 CSz | AF5 |
| PB6 | I2C1_SCL | OD | SH1106 SCL, TPS65263 SCL | AF4 |
| PB7 | I2C1_SDA | OD | SH1106 SDA, TPS65263 SDA | AF4 |
| PA0 | GPIO_OUT | O | IWR6843 NRST (F2) | — |
| PC6 | GPIO_IN | I | IWR6843 HOST_INTR (F1) | — |
| PD0 | GPIO_OUT | O | IWR6843 SYNC_IN (G1) | — |
| PD1 | GPIO_IN | I | IWR6843 SYNC_OUT (G2) | — |
| PD8 | USB_FS_DM | B | USB-C D– via USBLC6 | AF10 |
| PD9 | USB_FS_DP | B | USB-C D+ via USBLC6 | AF10 |
| PE2 | QSPI_CLK | O | MX25L25645G CLK | AF9 |
| PE3 | QSPI_NCS | O | MX25L25645G CSz | AF9 |
| PE4 | QSPI_IO0 | B | MX25L25645G DQ0 | AF9 |
| PE5 | QSPI_IO1 | B | MX25L25645G DQ1 | AF9 |
| PE6 | QSPI_IO2 | B | MX25L25645G DQ2 | AF9 |
| PE7 | QSPI_IO3 | B | MX25L25645G DQ3 | AF9 |
| PC7 | GPIO_OUT | O | ESP32-C3 EN (reset) | — |
| PC8 | GPIO_OUT | O | ESP32-C3 BOOT (GPIO8) | — |
| PD2 | GPIO_OUT | O | LAN8720A nRST | — |
| PD3 | GPIO_IN | I | LAN8720A nINT | — |
| PA13 | SWDIO | B | J3 pin 2 | AF0 |
| PA14 | SWCLK | I | J3 pin 4 | AF0 |

## 4. LAN8720A Connections (Ethernet PHY)

| LAN8720A Pin | Name | Direction | Connected To | Notes |
|---|---|---|---|---|
| 1 | VDDCR | PWR | 3.3 V (BUCK1) | 100 nF + 4.7 µF decoupling |
| 2 | VDDIO | PWR | 3.3 V (BUCK1) | 100 nF decoupling |
| 3 | TXD0 | I | STM32 PB13 | RMII TXD0 |
| 4 | TXD1 | I | STM32 PB14 | RMII TXD1 |
| 5 | TXEN | I | STM32 PB11 | RMII TXEN |
| 6 | RXD0 | O | STM32 PC4 | RMII RXD0 |
| 7 | RXD1 | O | STM32 PC5 | RMII RXD1 |
| 8 | CRS_DV | O | STM32 PB0 | RMII CRS/DV |
| 9 | MDIO | B | STM32 PA7 | SMI data |
| 10 | MDC | I | STM32 PA1 | SMI clock |
| 11 | REFCLK0 | O | STM32 PA2 (RMII_REFCLK) | 50 MHz from 25 MHz XTAL (PLL) |
| 12 | nINT | OD | STM32 PD3 | Interrupt, active low |
| 13 | nRST | I | STM32 PD2 | Reset, active low |
| 14 | XTAL1/CLKIN | I | 25 MHz crystal Y3 | 25 MHz reference |
| 15 | RBIAS | — | 10 kΩ to GND | Internal bias |
| 16 | TXP | O | RJ45 TX+ (via magnetics) | Differential pair |
| 17 | TXN | O | RJ45 TX– (via magnetics) | Differential pair |
| 18 | RXP | I | RJ45 RX+ (via magnetics) | Differential pair |
| 19 | RXN | I | RJ45 RX– (via magnetics) | Differential pair |
| 20–24 | GND | PWR | Ground plane | — |

## 5. TPS65263 PMIC Connections

### 5.1 Input & Control

| Pin | Name | Connection | Notes |
|---|---|---|---|
| 1 | VIN | 5 V (USB VBUS or PoE output) | 22 µF input cap C17 |
| 2 | EN | 10 kΩ to VIN (always on) | Auto-enable |
| 3 | PGOOD | STM32 PB5 (GPIO) | Power-good status |
| 4 | SDA | I2C1 bus (shared) | PMBus interface |
| 5 | SCL | I2C1 bus (shared) | PMBus interface |
| 6 | nRST | 10 kΩ to 3.3 V | Active-low reset |

### 5.2 Outputs

| Output | Voltage | Current | Inductor | Load | Sequence |
|---|---|---|---|---|---|
| BUCK1 | 3.3 V | 2 A | L1 (10 µH) | STM32 I/O, PHY, ESP32, OLED, IMU | 1st (0 ms) |
| BUCK2 | 1.8 V | 1 A | L2 (10 µH) | IWR6843 VDDIN, STM32 VDD | 2nd (+2 ms) |
| BUCK3 | 1.2 V | 1 A | L3 (10 µH) | IWR6843 VDD, STM32 core | 3rd (+4 ms) |
| LDO1 | 1.0 V | 0.5 A | — | IWR6843 RF | 4th (+6 ms) |
| LDO2 | 3.0 V | 0.1 A | — | IWR6843 analog | 5th (+8 ms) |

### 5.3 Output Capacitors

| Output | Cap | Value | Package | ESR target |
|---|---|---|---|---|
| BUCK1 | 2× 22 µF | 22 µF, 6.3 V, X5R | 0805 | < 10 mΩ |
| BUCK2 | 1× 22 µF | 22 µF, 6.3 V, X5R | 0805 | < 10 mΩ |
| BUCK3 | 1× 22 µF | 22 µF, 6.3 V, X5R | 0805 | < 10 mΩ |
| LDO1 | 1× 4.7 µF | 4.7 µF, 6.3 V, X5R | 0402 | < 50 mΩ |
| LDO2 | 1× 4.7 µF | 4.7 µF, 6.3 V, X5R | 0402 | < 50 mΩ |

## 6. ICM-42688-P IMU Connections

| Pin | Name | Direction | STM32 Pin | Notes |
|---|---|---|---|---|
| 1 | SDO | O | PA6 (SPI1_MISO) | Data out |
| 2 | SDI | I | PA7 (SPI1_MOSI) | Data in |
| 3 | SCK | I | PA5 (SPI1_SCK) | SPI clock, up to 24 MHz |
| 4 | CSz | I | PA4 (SPI1_NSS) | Chip select, active low |
| 5 | INT1 | O | PC9 (GPIO/EXTI) | Data-ready interrupt |
| 6 | INT2 | — | NC | Not used |
| 7 | VDD | PWR | 3.3 V (BUCK1) | 100 nF decoupling |
| 8 | VDDIO | PWR | 3.3 V (BUCK1) | 100 nF decoupling |
| 9–14 | GND | PWR | GND | — |

## 7. ESP32-C3-MINI-1 Module Connections

| Pin | Name | Direction | STM32 Pin | Notes |
|---|---|---|---|---|
| TXD0 | TX | O | PB11 (USART2_RX) | ESP32 TX → STM32 RX |
| RXD0 | RX | I | PB10 (USART2_TX) | STM32 TX → ESP32 RX |
| GPIO8 | BOOT | I | PC8 | Boot mode select |
| EN | Reset | I | PC7 | Active-low reset |
| GPIO2 | RTS | I | NC | Not used (AT fw auto-flow) |
| GPIO3 | CTS | O | NC | Not used |
| 3V3 | PWR | PWR | 3.3 V (BUCK1) | 10 µF + 100 nF decoupling |
| GND | PWR | GND | Ground | — |
| Antenna | — | — | Chip antenna (on-module) | 2.4 GHz PCB trace |

## 8. USB-C Connector (J1) Connections

| J1 Pin | Name | Connected To | Notes |
|---|---|---|---|
| A1, B12 | GND | Ground plane | — |
| A4, B9 | VBUS | 5 V input → TPS65263 VIN | 10 µF bulk |
| A5 | CC1 | 5.1 kΩ to GND | 5 V / 1.5 A advertisement |
| B5 | CC2 | 5.1 kΩ to GND | 5 V / 1.5 A advertisement |
| A6 | D+ | USBLC6 D1 (→ STM32 DP) | USB 2.0 FS |
| A7 | D– | USBLC6 D2 (→ STM32 DM) | USB 2.0 FS |
| B6 | D+ | A6 (shorted) | Redundant |
| B7 | D– | A7 (shorted) | Redundant |
| A2, A3, B2, B3 | SBU | NC | No alternate mode |

## 9. Netlist (Key Signals)

### 9.1 Radar Host Interface

| Net Name | Source Pin | Via | Dest Pin | Notes |
|---|---|---|---|---|
| RADAR_TX | IWR6843 D1 (UART_TX) | Direct | STM32H7 PA10 (USART1_RX) | 3.3 V, series 33 Ω |
| RADAR_RX | STM32H7 PA9 (USART1_TX) | Direct | IWR6843 D2 (UART_RX) | 3.3 V, series 33 Ω |
| RADAR_CTS | IWR6843 E2 (UART_RTS) | Direct | STM32H7 PA12 (USART1_RTS) | 3.3 V |
| RADAR_RTS | STM32H7 PA11 (USART1_CTS) | Direct | IWR6843 E1 (UART_CTS) | 3.3 V |
| RADAR_INTR | IWR6843 F1 (HOST_INTR) | Direct | STM32H7 PC6 (GPIO) | 3.3 V, 10 kΩ pull-up |
| RADAR_NRST | STM32H7 PD0 (GPIO) | Direct | IWR6843 F2 (NRST) | Active low, 10 kΩ pull-up |
| RADAR_SYNC_IN | STM32H7 PD1 (GPIO) | Direct | IWR6843 G1 (SYNC_IN) | — |
| RADAR_SYNC_OUT | IWR6843 G2 (SYNC_OUT) | Direct | STM32H7 PD2 (GPIO) | — |

### 9.2 Ethernet RMII

| Net Name | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| ETH_REFCLK | LAN8720A REFCLK0 (pin 11) | STM32H7 PA2 (RMII_REFCLK) | 50 MHz, 49.9 Ω series term |
| ETH_MDC | STM32H7 PA1 | LAN8720A MDC (pin 10) | 2.5 MHz max |
| ETH_MDIO | STM32H7 PA7 | LAN8720A MDIO (pin 9) | 1.5 kΩ pull-up to 3.3 V |
| ETH_TXD0 | STM32H7 PB13 | LAN8720A TXD0 (pin 3) | — |
| ETH_TXD1 | STM32H7 PB14 | LAN8720A TXD1 (pin 4) | — |
| ETH_TXEN | STM32H7 PB11 | LAN8720A TXEN (pin 5) | — |
| ETH_RXD0 | LAN8720A RXD0 (pin 6) | STM32H7 PC4 | — |
| ETH_RXD1 | LAN8720A RXD1 (pin 7) | STM32H7 PC5 | — |
| ETH_RXDV | LAN8720A CRS_DV (pin 8) | STM32H7 PB0 | — |
| ETH_nRST | STM32H7 PD2 | LAN8720A nRST (pin 13) | 10 kΩ pull-up |
| ETH_nINT | LAN8720A nINT (pin 12) | STM32H7 PD3 | 10 kΩ pull-up |

### 9.3 SPI (IMU)

| Net Name | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| IMU_SCK | STM32H7 PA5 (SPI1_SCK) | ICM-42688 SCK (pin 3) | 10 MHz, 3.3 V |
| IMU_MOSI | STM32H7 PA7 (SPI1_MOSI) | ICM-42688 SDI (pin 2) | — |
| IMU_MISO | ICM-42688 SDO (pin 1) | STM32H7 PA6 (SPI1_MISO) | — |
| IMU_CSz | STM32H7 PA4 (SPI1_NSS) | ICM-42688 CSz (pin 4) | Active low |
| IMU_INT1 | ICM-42688 INT1 (pin 5) | STM32H7 PC9 | Rising edge EXTI |

### 9.4 I2C (Shared Bus: OLED + PMIC)

| Net Name | STM32 Pin | Devices | Notes |
|---|---|---|---|
| I2C1_SCL | PB6 (I2C1_SCL) | SH1106 SCL, TPS65263 SCL | 400 kHz, 4.7 kΩ pull-up to 3.3 V |
| I2C1_SDA | PB7 (I2C1_SDA) | SH1106 SDA, TPS65263 SDA | 400 kHz, 4.7 kΩ pull-up to 3.3 V |

### 9.5 QSPI Flash

| Net Name | STM32 Pin | MX25L25645G Pin | Notes |
|---|---|---|---|
| QSPI_CLK | PE2 | Pin 6 (CLK) | 80 MHz max |
| QSPI_NCS | PE3 | Pin 1 (CSz) | Active low |
| QSPI_DQ0 | PE4 | Pin 5 (DQ0/SI) | — |
| QSPI_DQ1 | PE5 | Pin 2 (DQ1/SO) | — |
| QSPI_DQ2 | PE6 | Pin 3 (DQ2/WPz) | Used in quad mode |
| QSPI_DQ3 | PE7 | Pin 7 (DQ3/HOLDz) | Used in quad mode |

## 10. Decoupling Networks

### 10.1 IWR6843AOP Decoupling

| Rail | Caps | Placement | Notes |
|---|---|---|---|
| 1.2 V (VDD) | 4× 100 nF + 1× 4.7 µF | Within 2 mm of each VDD pin | Place on same side |
| 1.8 V (VDDIN) | 2× 100 nF + 1× 4.7 µF | Adjacent to VDDIN pins | — |
| 1.0 V (VDDA_RF1) | 2× 100 nF + 1× 1 µF | Near RF LDO output | Low-ESL critical |
| 3.0 V (VDDA_RF2) | 1× 100 nF + 1× 1 µF | Near analog supply pin | — |

### 10.2 STM32H743 Decoupling

| Rail | Caps | Placement | Notes |
|---|---|---|---|
| 1.2 V (VDD) | 8× 100 nF + 2× 4.7 µF | One 100 nF per VDD pin | — |
| 3.3 V (VDD3V3) | 6× 100 nF + 2× 4.7 µF | Distributed | — |
| VDDA (analog) | 1× 100 nF + 1× 1 µF | Via ferrite bead from 3.3 V | — |

### 10.3 LAN8720A Decoupling

| Rail | Caps | Placement | Notes |
|---|---|---|---|
| VDDCR (1.2 V internal) | 1× 100 nF + 1× 4.7 µF | Adjacent to pin | — |
| VDDIO (3.3 V) | 1× 100 nF | Adjacent to pin | — |

### 10.4 ESP32-C3 Decoupling

| Rail | Caps | Placement | Notes |
|---|---|---|---|
| 3.3 V | 1× 10 µF + 1× 100 nF | Near module pins | Bulk + HF |

## 11. Impedance-Matched Pairs

| Pair | Impedance | Spacing | Width | Layer | Notes |
|---|---|---|---|---|---|
| USB D+/D– | 90 Ω diff | 0.15 mm | 0.20 mm | L3 (inner) | USB 2.0 FS |
| ETH TXP/TXN | 100 Ω diff | 0.15 mm | 0.18 mm | L3 (inner) | From PHY to RJ45 magnetics |
| ETH RXP/RXN | 100 Ω diff | 0.15 mm | 0.18 mm | L3 (inner) | From RJ45 to PHY |
| ETH_RMII_REFCLK | 50 Ω single | — | 0.20 mm | L1 (top) | 25/50 MHz ref clock |
| QSPI (CLK, DQ0–3) | 50 Ω single | 0.30 mm gap | 0.20 mm | L1 (top) | Length-matched within 5 mm |
| SPI1 (SCK, MOSI, MISO) | 50 Ω single | 0.30 mm gap | 0.20 mm | L1 (top) | Length-matched within 10 mm |

## 12. Pull-Up / Pull-Down Summary

| Signal | Resistor | To | Value | Purpose |
|---|---|---|---|---|
| I2C1_SCL | 4.7 kΩ | 3.3 V | 1 per bus | I2C pull-up |
| I2C1_SDA | 4.7 kΩ | 3.3 V | 1 per bus | I2C pull-up |
| ETH_MDIO | 1.5 kΩ | 3.3 V | — | SMI pull-up (spec) |
| ETH_nRST | 10 kΩ | 3.3 V | — | Keep PHY out of reset |
| ETH_nINT | 10 kΩ | 3.3 V | — | Interrupt inactive high |
| RADAR_INTR | 10 kΩ | 3.3 V | — | Default high (no intr) |
| RADAR_NRST | 10 kΩ | 3.3 V | — | Keep radar out of reset |
| IWR6843 SOP0 | 10 kΩ | 3.3 V | — | Functional boot mode |
| IWR6843 SOP1 | 10 kΩ | GND | — | — |
| IWR6843 SOP2 | 10 kΩ | GND | — | — |
| USB_CC1 | 5.1 kΩ | GND | — | USB-C 1.5 A advertise |
| USB_CC2 | 5.1 kΩ | GND | — | USB-C 1.5 A advertise |
| ESP32_EN | 10 kΩ | 3.3 V | — | Keep module enabled |
| IMU_INT1 | 10 kΩ | GND | — | Default low (no intr) |
| TPS65263 nRST | 10 kΩ | 3.3 V | — | PMIC active |