# Terramesh — Phase 2: Component Selection & Schematics

**Author: jayis1**  
**Copyright © 2026 jayis1**

---

## 1. Bill of Materials

| Ref | Part Number | Description | Package | Qty |
|-----|-------------|-------------|---------|-----|
| U1 | STM32U5A5ZJT6Q | Cortex-M33 MCU, 4 MB Flash, 2.5 MB SRAM | LQFP144 | 1 |
| U2 | SX1262IMLTRT | LoRa transceiver, 868/915 MHz | QFN24 | 1 |
| U3 | BG95-M3 | LTE Cat M1 / NB-IoT module | LGA96 | 1 |
| U4 | ADXL372BCCZ | High-g accelerometer, 200 g | LGA16 | 1 |
| U5 | SCL3300-D01 | 3-axis inclinometer, ±90° | SOIC-16 | 1 |
| U6 | BME688 | Environmental sensor (T/H/P/Gas) | LGA8 | 1 |
| U7, U8 | ADS1120IRVA | 16-bit ADC, 2-ch, PGA | VQFN16 | 2 |
| U9 | MX25R6435FM1IL0 | 64 Mb serial NOR flash | SOP-8 | 1 |
| U10 | RV-3028-C7 | RTC, 45 nA, ±3 ppm | QFN10 | 1 |
| U11 | MAX20361EWA+ | PMIC, boost + battery monitor | WLP25 | 1 |
| U12 | ST25DV04K | Dynamic NFC tag, 4 Kb | SO-8 | 1 |
| U13 | TPS7A4700 | 3.3 V LDO, 1 A, ultra-low noise | HTSSOP20 | 1 |
| U14 | TPS7A2025 | 2.5 V reference LDO | SOT-23-5 | 1 |
| D1 | PESD5V0S1UB | ESD protection, USB-C | SOD-323 | 1 |
| D2–D3 | PESD3V3S1UB | ESD protection, antenna | SOD-323 | 2 |
| L1 | 74404064100 | 10 µH shielded inductor, LoRa PA | 6.4×6.4 mm | 1 |
| L2 | 74404064047 | 4.7 µH inductor, boost converter | 6.4×6.4 mm | 1 |
| C1–C10 | GRM188R60J106K | 10 µF, 6.3 V, X5R, decoupling | 0603 | 10 |
| C11–C20 | GRM188R71H104K | 0.1 µF, 50 V, X7R, bypass | 0603 | 10 |
| C21–C22 | GRM1885C1H220J | 22 pF, 50 V, C0G, crystal load | 0603 | 2 |
| C23 | GRM188R61A106K | 10 µF, 10 V, X5R, USB | 0603 | 1 |
| R1–R10 | CRCW060310K0F | 10 kΩ, 1% | 0603 | 10 |
| R11–R12 | CRCW06031K00F | 1 kΩ, 1% | 0603 | 2 |
| R13 | CRCW06034K70F | 4.7 kΩ, 1% | 0603 | 1 |
| R14 | CRCW0603470RF | 470 Ω, 1% | 0603 | 1 |
| Y1 | FC-135 32.768 kHz | 32.768 kHz crystal, ±20 ppm | 3.2×1.5 mm | 1 |
| Y2 | 7M-26.000MAAJ-T | 26 MHz crystal, ±10 ppm | 3.2×2.5 mm | 1 |
| J1 | USB-C-31-M-12 | USB-C connector, 16-pin | SMD | 1 |
| J2 | TC2050-IDC | Tag-Connect SWD debug | 5-pin | 1 |
| J3 | SMA-142-0701 | SMA antenna connector | Edge-mount | 1 |
| J4 | Molex 53261-0471 | Battery connector, 4-pin | SMD | 1 |
| BT1 | LiSOCl₂ D-cell ×3 | 3.6 V, 19 Ah each | D-cell | 3 |
| SC1 | SCMR18C105PRBB0 | 1 F supercapacitor, 2.7 V | Radial | 1 |
| ANT1 | ANT-868-CW-HW | 868 MHz ¼-wave whip | SMA | 1 |

## 2. Pinout / MUX Table

### STM32U5A5ZJT6Q Pin Assignments

| Pin | Function | Connected To | Notes |
|-----|----------|-------------|-------|
| PA0 | SPI1_SCK | SX1262 SCK, MX25R64 SCK | 10 MHz |
| PA1 | SPI1_MISO | SX1262 MISO, MX25R64 SO | |
| PA2 | SPI1_MOSI | SX1262 MOSI, MX25R64 SI | |
| PA3 | GPIO_OUT | SX1262 NSS | Chip select |
| PA4 | GPIO_OUT | MX25R64 CS | Chip select |
| PA5 | GPIO_OUT | SX1262 DIO1 | IRQ |
| PA6 | GPIO_OUT | SX1262 BUSY | Busy indicator |
| PA7 | GPIO_OUT | SX1262 RESET | Reset |
| PB0 | SPI2_SCK | SCL3300 SCK, ADXL372 SCK | 8 MHz |
| PB1 | SPI2_MISO | SCL3300 MISO, ADXL372 MISO | |
| PB2 | SPI2_MOSI | SCL3300 MOSI, ADXL372 MOSI | |
| PB3 | GPIO_OUT | SCL3300 CS | Chip select |
| PB4 | GPIO_OUT | ADXL372 CS | Chip select |
| PB5 | GPIO_IN | ADXL372 INT1 | Motion interrupt |
| PB6 | I2C1_SCL | BME688, RV-3028, ST25DV | 400 kHz |
| PB7 | I2C1_SDA | BME688, RV-3028, ST25DV | |
| PB8 | I2C2_SCL | ADS1120 #1, #2 | 100 kHz |
| PB9 | I2C2_SDA | ADS1120 #1, #2 | |
| PC0 | LPUART1_TX | BG95-M3 UART_RX | 9600 baud |
| PC1 | LPUART1_RX | BG95-M3 UART_TX | |
| PC2 | GPIO_OUT | BG95-M3 PWRKEY | Power on |
| PC3 | GPIO_IN | BG95-M3 STATUS | Module status |
| PC4 | USART2_TX | USB CDC-ACM | 115200 baud |
| PC5 | USART2_RX | USB CDC-ACM | |
| PC6 | GPIO_IN | RV-3028 INT | RTC alarm |
| PC7 | GPIO_IN | SCL3300 DRDY | Data ready |
| PC8 | GPIO_IN | ADS1120 #1 DRDY | ADC data ready |
| PC9 | GPIO_IN | ADS1120 #2 DRDY | ADC data ready |
| PD0 | GPIO_OUT | Moisture probe enable | FET gate |
| PD1 | ADC_IN1 | Battery voltage divider | 1/3 ratio |
| PD2 | ADC_IN2 | Moisture probe analog | Capacitance |
| PD3 | GPIO_OUT | LED_RED | Status indicator |
| PD4 | GPIO_OUT | LED_GREEN | Status indicator |
| PD5 | GPIO_OUT | PMIC_EN | Enable 3.3 V rail |
| PD6 | GPIO_OUT | LTE_EN | Enable LTE rail |

## 3. Decoupling Network

| Rail | Cap Values | Quantity | Placement |
|------|------------|----------|-----------|
| VDD_MCU (1.8 V) | 10 µF + 0.1 µF | 1+1 per pin pair | < 2 mm from each VDD/VSS pair |
| VDD_3V3 (3.3 V) | 10 µF + 0.1 µF | 2+2 | Near LDO output and each IC |
| VDD_LTE (3.8 V) | 100 µF + 10 µF + 0.1 µF | 1+1+1 | Near BG95-M3 VBAT pins |
| VREF (2.5 V) | 10 µF + 0.1 µF | 1+1 | Near ADS1120 REF pin |
| VDD_IO (3.3 V) | 10 µF + 0.1 µF | 1+1 per bank | Near each IO bank |

## 4. Netlist (Key Connections)

```
# Power
VBAT+   → MAX20361 VBAT, TPS7A4700 IN
VBAT-   → GND
VDD_3V3 → TPS7A4700 OUT → all 3.3 V devices
VDD_1V8 → STM32U5 VDD, VDD_USB
VREF_2V5 → TPS7A2025 OUT → ADS1120 REFP

# LoRa (SPI1)
SPI1_SCK  → U2(SCK), U9(SCK)
SPI1_MISO → U2(MISO), U9(SO)
SPI1_MOSI → U2(MOSI), U9(SI)
U2_NSS    → PA3
U9_CS     → PA4
U2_DIO1   → PA5 (IRQ)
U2_BUSY   → PA6
U2_RESET  → PA7
U2_RF_HF  → L1 → ANT1

# Tilt + Accel (SPI2)
SPI2_SCK  → U5(SCK), U4(SCK)
SPI2_MISO → U5(MISO), U4(MISO)
SPI2_MOSI → U5(MOSI), U4(MOSI)
U5_CS     → PB3
U4_CS     → PB4
U4_INT1   → PB5

# I2C1 (Environmental + RTC + NFC)
I2C1_SCL  → U6(SCL), U10(SCL), U12(SCL)
I2C1_SDA  → U6(SDA), U10(SDA), U12(SDA)
U10_INT   → PC6

# I2C2 (Pore pressure ADCs)
I2C2_SCL  → U7(SCL), U8(SCL)
I2C2_SDA  → U7(SDA), U8(SDA)
U7_DRDY   → PC8
U8_DRDY   → PC9

# LTE (LPUART1)
LPUART1_TX → U3(RX)
LPUART1_RX → U3(TX)
U3_PWRKEY  → PC2
U3_STATUS  → PC3

# USB
USB_DP     → J1(DP)
USB_DN     → J1(DN)
USB_VBUS   → J1(VBUS) → TPS7A4700 EN
USB_ID     → GND (OTG host mode)

# Debug SWD
SWDIO      → J2(3)
SWCLK      → J2(2)
SWO        → J2(4)
RESET      → J2(5)
```

## 5. Impedance Matching

### LoRa RF Path (868 MHz)

- Characteristic impedance: 50 Ω
- Microstrip width: 0.45 mm (4-layer, 0.2 mm prepreg, Er=4.2)
- Matching network: π-network (C1=1.5 pF, L1=10 nH, C2=1.5 pF) between SX1262 RF_HF and SMA
- Harmonic filter: 3rd-order low-pass, fc=1.0 GHz

### USB Differential Pair

- Differential impedance: 90 Ω
- Trace width: 0.35 mm, gap: 0.15 mm
- Length matching: < 1 mm skew
- Total length: < 50 mm
