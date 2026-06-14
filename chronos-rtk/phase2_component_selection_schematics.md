# Chronos-RTK — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| Ref | Part Number | Manufacturer | Description | Qty | Unit Price (USD) |
|---|---|---|---|---|---|
| U1 | ZED-F9P | u-blox | Dual-frequency RTK GNSS receiver (LGA 24-pin) | 1 | 185.00 |
| U2 | STM32G474RET6 | STMicroelectronics | MCU, Cortex-M4F @ 170 MHz, LQFP-64 | 1 | 8.50 |
| U3 | SX1262IMLTRT | Semtech | Sub-GHz LoRa transceiver, QFN-24 | 1 | 6.20 |
| U4 | W25Q128JVSIM | Winbond | 128 Mbit SPI NOR flash, SOIC-8 | 1 | 1.40 |
| U5 | SSD1306Z | Solomon | OLED controller 128×64, I2C, die-bonded on flex | 1 | 3.80 |
| U6 | TPS62A0229RNM | Texas Instruments | Step-down converter 5→3.3 V, 2 A, VQFN-8 | 1 | 1.20 |
| U7 | TLV75518PDBV | Texas Instruments | LDO 3.3→1.8 V, 150 mA, SOT-23-5 | 1 | 0.35 |
| U8 | MCP73871T-2CCI | Microchip | LiPo charger, USB input, MSOP-10 | 1 | 1.80 |
| U9 | BME280 | Bosch | Humidity/pressure/temp sensor, LGA-8 (optional) | 1 | 2.50 |
| Y1 | TG5032CFN | Epson | 32.768 kHz TCXO ±20 ppm, SMD 3.2×2.5 | 1 | 0.90 |
| Y2 | ABM8-32.000MHZ | Abracon | 32 MHz crystal ±30 ppm, HC49/SMD | 1 | 0.40 |
| Y3 | TCX0130A | TXC | 32 MHz TCXO ±0.5 ppm (SX1262 ref), SMD | 1 | 2.10 |
| J1 | 1054500101 | Molex | USB-C receptacle, 24-pin SMD | 1 | 0.75 |
| J2 | CONSMA003.062 | Linx | SMA edge-mount, LoRa antenna | 1 | 1.50 |
| J3 | 734151154 | Würth | MCX edge-mount, GNSS antenna | 1 | 0.60 |
| J4 | S4B-PH-SM4 | JST | PH 4-pin, LiPo battery | 1 | 0.30 |
| SW1-3 | SKQGAFE010 | Alps | Tactile switch 6×6 mm, momentary | 3 | 0.15 |
| L1 | SRN4018-100M | Bourns | 10 µH 1.8 A shielded inductor, 4×4 mm | 1 | 0.40 |
| L2 | SRN3015-4N7M | Bourns | 4.7 µH 600 mA, SX1262 match | 1 | 0.25 |
| D1 | SMBJ5.0A | Littelfuse | TVS 5 V bidirectional, SMB | 1 | 0.20 |
| D2 | RB521S30T1G | ON Semi | Schottky 30 V 200 mA, SOD-523 (power path) | 1 | 0.08 |
| LED1-3 | 150060RS75000 | Würth | LED 0603 red/green/blue (PWR/RTK/Lora) | 3 | 0.06 |

**Passives**: ~45 × 0402 capacitors, ~20 × 0402 resistors — detailed in netlists below.

**Total BOM cost (1 qty)**: ≈ $220

## 2. Pinout Tables

### 2.1 STM32G474RET6 Pin Assignments (LQFP-64)

| Pin | Name | Function | Net | Notes |
|---|---|---|---|---|
| 1 | VBAT | Battery backup | VDD_3V3 | — |
| 2 | PC13 | GPIO | NET_RTC_INT | BME280 interrupt |
| 3 | PC14 | GPIO | NET_BTN_MODE | Mode button (active low, 10k pull-up) |
| 4 | PC15 | GPIO | NET_BTN_SEL | Select button |
| 5 | PF0/OSC_IN | Crystal | NET_XTAL32_IN | 32.768 kHz LSE |
| 6 | PF1/OSC_OUT | Crystal | NET_XTAL32_OUT | 32.768 kHz LSE |
| 7 | NRST | Reset | NET_RST | 10k pull-up + 100nF cap + reset button |
| 8 | PC0 | ADC | NET_VBAT_SENSE | Battery voltage divider (½ Vbat) |
| 9 | PC1 | ADC | NET_TEMP_SENSE | NTC thermistor (board temp) |
| 10 | PC2 | GPIO | NET_BTN_ACT | Action button |
| 11 | PC3 | SPI2_MOSI | NET_SPI2_MOSI | → SX1262 MOSI |
| 12 | PA0 | SPI2_SCK | NET_SPI2_SCK | → SX1262 SCK |
| 13 | PA1 | SPI2_NSS | NET_SPI2_NSS | → SX1262 NSS (active low) |
| 14 | PA2 | USART1_TX | NET_UART1_TX | → ZED-F9P RX |
| 15 | PA3 | USART1_RX | NET_UART1_RX | ← ZED-F9P TX |
| 16 | PA4 | SPI1_NSS | NET_SPI1_NSS | → W25Q128 CS# (active low) |
| 17 | PA5 | SPI1_SCK | NET_SPI1_SCK | → W25Q128 CLK |
| 18 | PA6 | SPI1_MISO | NET_SPI1_MISO | ← W25Q128 DO |
| 19 | PA7 | SPI1_MOSI | NET_SPI1_MOSI | → W25Q128 DI |
| 20 | PC4 | GPIO | NET_LORA_BUSY | ← SX1262 BUSY |
| 21 | PC5 | GPIO | NET_LORA_RST | → SX1262 NRST |
| 22 | PB0 | GPIO | NET_LORA_DIO1 | ← SX1262 DIO1 (IRQ) |
| 23 | PB1 | TIM3_CH4 | NET_LORA_TCXO_EN | SX1262 TCXO enable via MOSFET |
| 24 | PB2 | GPIO | NET_GNSS_RST | → ZED-F9P RESET_N |
| 25 | PB10 | I2C1_SCL | NET_I2C_SCL | → SSD1306, BME280 |
| 26 | PB11 | I2C1_SDA | NET_I2C_SDA | → SSD1306, BME280 |
| 27 | PB12 | GPIO | NET_LED_R | LED red (RTK fix status) |
| 28 | PB13 | GPIO | NET_LED_G | LED green (data activity) |
| 29 | PB14 | GPIO | NET_LED_B | LED blue (LoRa TX/RX) |
| 30 | PB15 | GPIO | NET_GNSS_PPS | ← ZED-F9P TIMEPULSE |
| 31 | PC6 | TIM8_CH1 | NET_BACKLIGHT_EN | OLED backlight PWM |
| 32 | PC7 | GPIO | NET_GNSS_SAFE | ← ZED-F9P SAFEBOOT_N |
| 33 | PC8 | GPIO | NET_GNSS_INT | ← ZED-F9P INT (exti) |
| 34 | PC9 | GPIO | NET_LORA_ANT_SW | TX/RX antenna switch control |
| 35 | PA8 | MCO | NET_MCO | 8 MHz clock output (debug) |
| 36 | PA9 | USB_OTG_FS_VBUS | NET_VBUS | USB VBUS detect |
| 37 | PA10 | USB_OTG_FS_ID | — | tied to GND |
| 38 | PA11 | USB_OTG_FS_DM | NET_USB_DM | USB data minus |
| 39 | PA12 | USB_OTG_FS_DP | NET_USB_DP | USB data plus |
| 40 | PA13 | SWDIO | NET_SWDIO | Debug port |
| 41 | PA14 | SWCLK | NET_SWCLK | Debug port |
| 42 | PA15 | GPIO | NET_FLASH_WP | W25Q128 WP# |
| 43 | PC10 | SPI3_SCK | — | (reserved for debug) |
| 44 | PC11 | SPI3_MISO | — | (reserved for debug) |
| 45 | PC12 | SPI3_MOSI | — | (reserved for debug) |
| 46 | PD2 | GPIO | NET_FLASH_HOLD | W25Q128 HOLD# |
| 47 | PB3 | GPIO | NET_POWER_EN | Main 3.3 V LDO enable (active high) |
| 48 | PB4 | GPIO | NET_CHARGE_EN | MCP73871 CE# (active low) |
| 49 | PB5 | GPIO | NET_CHARGE_STAT | MCP73871 STAT1 |
| 50 | PB6 | GPIO | NET_CHARGE_STAT2 | MCP73871 STAT2 |
| 51 | PB7 | GPIO | NET_OLED_RST | SSD1306 RES# |
| 52 | PB8 | TIM16_CH1 | NET_LORA_TCXO_CLK | Optional TCXO clock gating |
| 53 | PB9 | GPIO | NET_VBUS_VALID | USB 5 V present (comparator) |
| 54 | VDD | Power | VDD_3V3 | Decoupled per STM32 guidelines |
| 55 | VSS | Ground | GND | — |
| 56 | VDDA | Analog power | VDD_3V3A | Via ferrite bead from VDD_3V3 |
| 57 | VREF+ | Analog ref | VDD_3V3A | — |
| 58 | VDD | Power | VDD_3V3 | — |
| 59 | VSSA | Analog ground | GND | — |
| 60 | PB12–PB15 | (see above) | — | — |
| 61–64 | VDD/VSS | Power | VDD_3V3/GND | — |

### 2.2 ZED-F9P (LGA 24) Key Pins

| Pin | Name | Net | Notes |
|---|---|---|---|
| 1 | VCC | VDD_3V3 | 3.3 V supply |
| 2 | VCC_RF | VDD_1V8 | 1.8 V RF domain |
| 3 | GND | GND | — |
| 4 | TX1 | NET_UART1_RX | → STM32 PA3 |
| 5 | RX1 | NET_UART1_TX | ← STM32 PA2 |
| 6 | TIMEPULSE | NET_GNSS_PPS | → STM32 PB15 |
| 7 | EXTINT0 | — | NC |
| 8 | RESET_N | NET_GNSS_RST | ← STM32 PB2 (10k pull-up) |
| 9 | SAFEBOOT_N | NET_GNSS_SAFE | ← STM32 PC7 (10k pull-up) |
| 10 | LNA_EN | NET_LNA_EN | Active antenna LNA control |
| 11 | INT | NET_GNSS_INT | → STM32 PC9 |
| 12 | SDA | NET_I2C_SDA | Secondary I2C (host) |
| 13 | SCL | NET_I2C_SCL | — |
| 14 | VDD_USB | VDD_3V3 | USB power (not used) |
| 15 | RF_IN | NET_GNSS_RF_IN | → MCX connector via matching network |
| 16 | GND | GND | — |
| 17 | VDD_3V3 | VDD_3V3 | Decoupling |
| 18–24 | GND / NC | GND / — | Ground and no-connect |

### 2.3 SX1262 (QFN-24) Key Pins

| Pin | Name | Net | Notes |
|---|---|---|---|
| 1 | GND | GND | — |
| 2 | VDD_IN | VDD_3V3 | 3.3 V supply |
| 3 | VDD_IO | VDD_3V3 | IO supply |
| 4 | SCK | NET_SPI2_SCK | ← STM32 PA0 |
| 5 | MISO | NET_SPI2_MISO | → STM32 (SPI2) |
| 6 | MOSI | NET_SPI2_MOSI | ← STM32 PC3 |
| 7 | NSS | NET_SPI2_NSS | ← STM32 PA1 |
| 8 | BUSY | NET_LORA_BUSY | → STM32 PC4 |
| 9 | DIO1 | NET_LORA_DIO1 | → STM32 PB2 (EXTI) |
| 10 | NRST | NET_LORA_RST | ← STM32 PC5 |
| 11 | GND | GND | — |
| 12 | DCC_SW | NET_DCC_SW | Decoupling cap to GND |
| 13 | VDD_PA | VDD_3V3 | PA supply, decoupled |
| 14 | VR_PA | NET_VR_PA | Regulator output (cap) |
| 15 | RFI_LP | — | NC (low-power path not used) |
| 16 | RFI_HP | NET_LORA_RF_IN | → antenna matching → SMA |
| 17 | GND | GND | — |
| 18 | GND | GND | — |
| 19 | TCXO_IN | NET_TCXO_OUT | ← 32 MHz TCXO (Y3) |
| 20 | GND | GND | — |
| 21 | VDD_TCXO | VDD_3V3 | TCXO supply |
| 22 | DIO2 | NET_LORA_ANT_SW | Antenna switch control |
| 23 | DIO3 | NET_LORA_TCXO_EN | TCXO enable via MOSFET |
| 24 | GND | GND | — |

## 3. Netlists (Source → Component → Destination)

### 3.1 Power Nets

| Net | Source | Dest 1 | Dest 2 | Dest 3 | Dest 4 |
|---|---|---|---|---|---|
| VSRC | USB VBUS (J1) | MCP73871 (U8) V_IN | D2 cathode | TPS62A02 (U6) VIN | — |
| VDD_3V3 | TPS62A02 (U6) VOUT | STM32 VDD (×4) | ZED-F9P VCC | SX1262 VDD_IN/IO | W25Q128 VDD |
| VDD_1V8 | TLV75518 (U7) VOUT | ZED-F9P VCC_RF | — | — | — |
| VDD_3V3A | Ferrite bead FB1 | STM32 VDDA/VREF+ | — | — | — |
| VBAT_SENSE | R1/R2 divider | STM32 PC0 (ADC) | — | — | — |

### 3.2 UART1 (STM32 ↔ ZED-F9P)

| Net | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| NET_UART1_TX | STM32 PA2 (USART1_TX) | ZED-F9P RX1 | 460800 baud, 8N1 |
| NET_UART1_RX | ZED-F9P TX1 | STM32 PA3 (USART1_RX) | Level-matched (3.3 V) |

### 3.3 SPI1 (STM32 ↔ W25Q128)

| Net | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| NET_SPI1_SCK | STM32 PA5 | W25Q128 CLK | 50 MHz max |
| NET_SPI1_MOSI | STM32 PA7 | W25Q128 DI | — |
| NET_SPI1_MISO | W25Q128 DO | STM32 PA6 | — |
| NET_SPI1_NSS | STM32 PA4 | W25Q128 CS# | Active low |

### 3.4 SPI2 (STM32 ↔ SX1262)

| Net | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| NET_SPI2_SCK | STM32 PA0 | SX1262 SCK | 16 MHz max |
| NET_SPI2_MOSI | STM32 PC3 | SX1262 MOSI | — |
| NET_SPI2_MISO | SX1262 MISO | STM32 (SPI2_MISO) | — |
| NET_SPI2_NSS | STM32 PA1 | SX1262 NSS | Active low |

### 3.5 I2C1 (STM32 ↔ SSD1306 / BME280)

| Net | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| NET_I2C_SCL | STM32 PB10 | SSD1306 SCL, BME280 SCK | 400 kHz, 4.7k pull-ups |
| NET_I2C_SDA | STM32 PB11 | SSD1306 SDA, BME280 SDI | 400 kHz, 4.7k pull-ups |

### 3.6 USB (STM32 ↔ USB-C)

| Net | Source Pin | Dest Pin | Notes |
|---|---|---|---|
| NET_USB_DP | STM32 PA12 | J1 D+ | 90 Ω differential pair |
| NET_USB_DM | STM32 PA11 | J1 D− | 90 Ω differential pair |
| NET_VBUS | J1 VBUS | STM32 PA9 | 5 V detect via resistor divider |

## 4. Decoupling Networks

| IC | Pin | Cap | Value | Placement | Via |
|---|---|---|---|---|---|
| STM32G474 | VDD (×4) | C1–C4 | 100 nF 0402 X7R | Within 2 mm of pin | Direct to GND plane |
| STM32G474 | VDDA | C5 | 100 nF + C6 1 µF | Near VDDA pin | Ferrite bead isol. |
| ZED-F9P | VCC | C7 | 10 µF 0402 X5R | Adjacent to pin | Direct via |
| ZED-F9P | VCC_RF | C8 | 100 nF + C9 10 nF | Adjacent | Direct via |
| ZED-F9P | RF_IN | C10 | 56 pF C0G 0402 | DC block, series | Matched to 50 Ω |
| SX1262 | VDD_IN | C11 | 100 nF + C12 10 µF | Adjacent | Direct via |
| SX1262 | VDD_PA | C13 | 100 nF + C14 10 nF | Adjacent | Direct via |
| SX1262 | DCC_SW | C15 | 10 µF 0603 X5R | Pin to GND | Per datasheet |
| SX1262 | VR_PA | C16 | 4.7 µF 0402 X5R | Pin to GND | Per datasheet |
| W25Q128 | VDD | C17 | 100 nF | Adjacent | Direct via |
| TPS62A02 | VOUT | C18 | 22 µF 0603 X5R | Output bulk | — |
| TPS62A02 | VOUT | C19 | 100 nF | HF decoupling | — |
| TLV75518 | VOUT | C20 | 2.2 µF 0402 X5R | Stability | Per datasheet |

## 5. Impedance-Matched Pairs

| Pair | Impedance | Notes |
|---|---|---|
| USB DP/DM | 90 Ω differential (±10%) | Length-matched within 150 mils, no stubs |
| GNSS RF (MCX → F9P) | 50 Ω single-ended | Coplanar waveguide, 0.2 mm gap |
| LoRa RF (SX1262 → SMA) | 50 Ω single-ended | Coplanar waveguide, π-match network |

## 6. Pull-up / Pull-down Resistors

| Net | Resistor | Value | Purpose |
|---|---|---|---|
| I2C1 SCL | R10 | 4.7 kΩ pull-up to VDD_3V3 | I2C bus pull-up |
| I2C1 SDA | R11 | 4.7 kΩ pull-up to VDD_3V3 | I2C bus pull-up |
| NET_GNSS_RST | R12 | 10 kΩ pull-up to VDD_3V3 | ZED-F9P RESET_N |
| NET_GNSS_SAFE | R13 | 10 kΩ pull-up to VDD_3V3 | ZED-F9P SAFEBOOT_N |
| NET_SPI1_NSS | R14 | 10 kΩ pull-up to VDD_3V3 | W25Q128 CS# inactive high |
| NET_UART1_TX | R15 | 100 Ω series | Source termination |
| NET_UART1_RX | R16 | 100 Ω series | Source termination |
| NET_VBAT_SENSE | R17/R18 | 100 kΩ / 100 kΩ divider | ½ Vbat to ADC |
| NET_VBUS | R19/R20 | 47 kΩ / 47 kΩ divider | ½ VBUS to PA9 |
| NET_BTN_MODE | R21 | 10 kΩ pull-up to VDD_3V3 | Button pull-up |
| NET_BTN_SEL | R22 | 10 kΩ pull-up to VDD_3V3 | Button pull-up |
| NET_BTN_ACT | R23 | 10 kΩ pull-up to VDD_3V3 | Button pull-up |
| NET_LORA_DIO1 | R24 | 1 kΩ series | ESD current limit |

## 7. LoRa Antenna Matching Network

SX1262 RFI_HP → L_match (4.7 nH) → C_match (1.5 pF) → π-network (C1=2.2 pF, L1=3.3 nH, C2=2.7 pF) → SMA connector. Component values tuned for 868 MHz (EU) / 915 MHz (US) via 0402 parts.