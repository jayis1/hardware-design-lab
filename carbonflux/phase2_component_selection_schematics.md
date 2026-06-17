# Phase 2: Component Selection & Schematics — CarbonFlux

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**

---

## 1. Bill of Materials

### 1.1 Main Board (Console)

| Ref | Qty | Part | Description | Package | Supplier | Cost (1K) |
|-----|-----|------|-------------|---------|----------|-----------|
| U1 | 1 | STM32U5A9ZGJ6 | Cortex-M33 MCU, 2 MB Flash, 1.5 MB SRAM | UFBGA176+25 | Mouser | $12.50 |
| U2 | 1 | SX1262IMLTRT | LoRa transceiver, 868/915 MHz, +22 dBm | QFN-24 | DigiKey | $4.80 |
| U3 | 1 | W25Q128JVSIQ | 128 Mb (16 MB) SPI/QSPI flash | SOIC-8 | Mouser | $1.20 |
| U4 | 1 | SCD41 | CO₂ sensor, NDIR, 0–40,000 ppm, I²C | 12-pin LGA | Mouser | $28.50 |
| U5 | 1 | DPS310XTSA1 | Barometric pressure sensor, ±0.002 hPa | LGA-8 | Mouser | $2.80 |
| U6 | 1 | TMP117MAIDRVT | Temperature sensor, ±0.1°C, I²C | WSON-6 | DigiKey | $1.50 |
| U7 | 1 | RV-8803-C7 | RTC, ±3 ppm, I²C, battery backup | SON-12 | Mouser | $3.20 |
| U8 | 1 | CN3791 | MPPT solar charger IC | SOP-16 | LCSC | $1.10 |
| U9 | 1 | TPS63070RNMR | Buck-boost converter, 3.3V/2A | QFN-15 | DigiKey | $2.90 |
| U10 | 1 | MCP73871-2CCI/ML | LiFePO₄ charger (backup) | QFN-20 | Mouser | $1.80 |
| Q1 | 1 | IRF9540N | P-channel MOSFET, power switch | TO-220 | LCSC | $0.60 |
| L1 | 1 | NR3015T3R3M | 3.3 µH inductor, TPS63070 | SMD 3×3 mm | Mouser | $0.35 |
| L2 | 1 | NR4018T2R2M | 2.2 µH inductor, CN3791 | SMD 4×4 mm | Mouser | $0.30 |
| X1 | 1 | FC-135 32.768 kHz | RTC crystal, ±20 ppm | SMD 3.2×1.5mm | DigiKey | $0.50 |
| X2 | 1 | TCXO 32 MHz | SX1262 reference, ±0.5 ppm | SMD 2.5×2.0mm | Mouser | $1.50 |
| X3 | 1 | 16 MHz XTAL | MCU HSE crystal | SMD 3.2×2.5mm | Mouser | $0.30 |
| U11 | 1 | DS18B20+ | 1-Wire temperature sensor × 3 (on stake) | TO-92 | Mouser | $3.00 (3pcs) |
| J1 | 1 | USB4115-GF-A | USB-C connector, 16-pin, with CC | SMD | DigiKey | $0.50 |
| J2 | 1 | SMA_EDGE | SMA edge connector, LoRa antenna | SMD | Mouser | $0.40 |
| J3 | 1 | MC1.5/2-G-3.81 | 2-pin screw terminal, battery | Through-hole | Mouser | $0.50 |
| J4 | 1 | MC1.5/3-G-3.81 | 3-pin screw terminal, solar panel | Through-hole | Mouser | $0.60 |
| J5 | 1 | 8-PIN_WP | 8-pin waterproof connector, sensor stake | Panel mount | LCSC | $1.80 |
| J6 | 1 | 3-PIN_PWM | 3-pin header, servo | 2.54mm | LCSC | $0.10 |
| D1 | 1 | 1N5819 | Schottky diode, reverse polarity protection | DO-41 | LCSC | $0.10 |
| D2 | 1 | LED_GREEN | Power/status indicator LED | 0805 | LCSC | $0.05 |
| D3 | 1 | LED_RED | Error indicator LED | 0805 | LCSC | $0.05 |
| D4 | 1 | LED_BLUE | LoRa TX indicator | 0805 | LCSC | $0.05 |
| C1–C10 | 10 | 100nF | Decoupling capacitors | 0603 | LCSC | $0.10 |
| C11–C15 | 5 | 10µF | Bulk capacitors | 0805 | LCSC | $0.15 |
| C16–C18 | 3 | 22pF | Crystal load capacitors | 0603 | LCSC | $0.05 |
| R1–R10 | 10 | 10kΩ | Pull-up/pull-down resistors | 0603 | LCSC | $0.10 |
| R11 | 1 | 150kΩ | SCD41 I²C pull-up | 0603 | LCSC | $0.01 |
| R12 | 1 | 100kΩ | ADC voltage divider (Vbat) | 0603 | LCSC | $0.01 |
| R13 | 1 | 10kΩ | ADC voltage divider (Vbat) | 0603 | LCSC | $0.01 |
| U12 | 1 | SP3242E | RS-232 transceiver (spare UART) | SSOP-16 | Mouser | $1.20 |
| **Total Main Board** | | | | | | **~$72.00** |

### 1.2 Sensor Stake

| Ref | Qty | Part | Description | Cost (1K) |
|-----|-----|------|-------------|-----------|
| – | 1 | TEROS-11 | Soil moisture/temperature/EC | $65.00 |
| – | 3 | DS18B20+ | Soil temperature sensors | $3.00 |
| – | 1 | SQ-420-SS | PAR sensor, analog | $120.00 |
| – | 1 | 6-conductor shielded cable | 5m, waterproof connector | $8.00 |
| – | 1 | PVC pipe 20 cm dia. × 30 cm | Soil collar | $5.00 |
| **Total Sensor Stake** | | | | **~$201.00** |

### 1.3 Power + Enclosure

| Ref | Qty | Part | Cost |
|-----|-----|------|------|
| – | 1 | LiFePO₄ 12.8V 12Ah | $45.00 |
| – | 1 | Solar panel 6W 12V | $18.00 |
| – | 1 | ABS IP65 enclosure 200×150×100mm | $12.00 |
| **Total Power + Enclosure** | | | **~$75.00** |

### 1.4 Grand Total BOM

| Category | Cost |
|----------|------|
| Main Board | $72.00 |
| Sensor Stake | $201.00 |
| Power + Enclosure | $75.00 |
| **Total** | **~$348.00** |

## 2. Pinout Table (STM32U5A9ZG — UFBGA176+25)

| Pin | Pin Name | Function | Connected To | Notes |
|-----|----------|----------|-------------|-------|
| A1 | PB3 | SPI1_SCK | SX1262 SCK | 8 MHz |
| A2 | PB4 | SPI1_MISO | SX1262 MISO | |
| A3 | PB5 | SPI1_MOSI | SX1262 MOSI | |
| A4 | PB0 | GPIO_OUT | SX1262 NSS | CS, active low |
| A5 | PB1 | GPIO_OUT | SX1262 DIO1 | IRQ line |
| A6 | PB2 | GPIO_OUT | SX1262 RESET | Active low reset |
| B1 | PB6 | I2C1_SCL | SCD41, DPS310, TMP117, RV-8803 SCL | 400 kHz, 10k pull-up |
| B2 | PB7 | I2C1_SDA | SCD41, DPS310, TMP117, RV-8803 SDA | 400 kHz, 10k pull-up |
| B3 | PB8 | GPIO_OUT | EN_SCD41 (sensor power) | HIGH = enabled |
| B4 | PB9 | GPIO_OUT | EN_DPS310 | HIGH = enabled |
| B5 | PA0 | GPIO_OUT | EN_TMP117 | HIGH = enabled |
| B6 | PA1 | GPIO_IN | SX1262 BUSY | Busy indicator |
| C1 | PA2 | USART2_TX | USB CDC TX | Virtual COM port |
| C2 | PA3 | USART2_RX | USB CDC RX | Virtual COM port |
| C3 | PA4 | GPIO_OUT | RV-8803 NINT | RTC interrupt, active low |
| C4 | PA5 | SPI2_SCK | W25Q128 SCK | 40 MHz QSPI |
| C5 | PA6 | SPI2_MISO | W25Q128 SO | |
| C6 | PA7 | SPI2_MOSI | W25Q128 SI | |
| D1 | PA8 | GPIO_OUT | W25Q128 CS | Chip select |
| D2 | PA9 | GPIO_OUT | LED_RED | Error indicator |
| D3 | PA10 | GPIO_OUT | LED_GREEN | Power indicator |
| D4 | PA11 | USB_DP | USB-C D+ | Full-speed USB |
| D5 | PA12 | USB_DM | USB-C D- | Full-speed USB |
| D6 | PA13 | SWD_SWDIO | SWD programming | 10k pull-up |
| E1 | PA14 | SWD_SWCLK | SWD programming | 10k pull-down |
| E2 | PA15 | GPIO_IN | DIP switch bit 0 | Configuration |
| E3 | PC0 | ADC1_IN0 | PAR sensor analog | 0–2.5V, 12-bit |
| E4 | PC1 | ADC1_IN1 | Vbat monitor | Divider: 100k/10k |
| E5 | PC2 | ADC1_IN2 | Spare analog | Expansion |
| E6 | PC3 | GPIO_OUT | EN_5V | Servo power enable |
| F1 | PC4 | GPIO_OUT | SERVO_PWM | TIM2_CH1, 50 Hz |
| F2 | PC5 | GPIO_IN | SERVO_STALL | Stall detect feedback |
| F3 | PC6 | DS18B20_DQ | 1-Wire data | External pull-up 4.7k |
| F4 | PC7 | GPIO_IN | RAIN_SENSE | Rain capacitance sensor |
| F5 | PC8 | GPIO_IN | COLLAR_SWITCH | Collar presence detect |
| F6 | PC9 | GPIO_OUT | BUZZER | Buzzer/alarm |

## 3. Netlist — Critical Connections

### 3.1 I²C Bus (I2C1)

```
SCL ──┬── SCD41(12) ──┬── 10kΩ ── VDD_3V3
      ├── DPS310(4)   │
      ├── TMP117(5)   │
      └── RV-8803(9)  │
SDA ──┬── SCD41(13) ──┘
      ├── DPS310(3)
      ├── TMP117(4)
      └── RV-8803(10)
```

Each sensor has an independent enable (EN_SCD41, EN_DPS310, EN_TMP117) driven by GPIO. Only the active sensor's enable is HIGH to reduce bus capacitance and allow individual power cycling.

### 3.2 LoRa Front End (SX1262)

```
ANT ── SMA connector
        │
      ┌──┴──┐
      │ Balun │ (RF matching network, 50Ω)
      └──┬──┘
         │
      SX1262(RFP/RFN)
```

The SX1262 output is matched to 50Ω via a discrete LC balun and harmonic filter (recommended by SX1262 datasheet). The TCXO provides ±0.5 ppm frequency stability over -20°C to +60°C.

### 3.3 Power Tree

```
Solar Panel (18V Voc, 6W)
    │
    ├── D1 (1N5819 reverse protection)
    │
    ├── CN3791 MPPT Charger
    │    │
    │    ├── LiFePO₄ 12.8V 12Ah
    │    │    │
    │    │    ├── TPS63070 Buck-Boost (VIN: 2.5–14V → 3.3V)
    │    │    │    │
    │    │    │    ├── VDD_3V3 (MCU, sensors, LoRa, flash)
    │    │    │    └── EN_5V (GPIO-controlled → servo)
    │    │    │
    │    │    └── Vbat monitor (100k + 10k divider → ADC1_IN1)
    │    │
    │    └── MCP73871 Backup Charger (USB-C 5V input)
    │
    └── USB-C (5V @ 1A)
```

## 4. Decoupling Network

Every power pin on the STM32U5A9ZG gets:
- 100 nF (0603) per supply pair, placed <3 mm from pin
- One 10 µF (0805) per supply domain (VDD, VDDA, VDD_USB)

SX1262 gets:
- 100 nF on VDD_RF
- 10 µF on VBAT_IN
- 1 nF + 10 pF on VDDPA (RF PLL supply)

SCD41 gets:
- 100 nF on VDD
- 10 µF on VDDIO (separate analog supply decoupling)

## 5. Signal Integrity Notes

- I²C bus runs at 400 kHz with 10kΩ pull-ups to 3.3V. Total bus capacitance < 50 pF.
- SPI1 (SX1262) runs at 8 MHz — keep traces <3 cm, impedance ~50Ω.
- SPI2/QSPI (W25Q128) runs at 40 MHz — match trace lengths, no stubs.
- USB D+/D− matched to within 2 mm differential pair, 90Ω differential impedance.
- ADC analog input (PAR) has a 100 nF capacitor right at MCU pin, RC filter R=1kΩ C=10nF.
- DS18B20 1-Wire bus has 4.7kΩ pull-up. Trace <30 cm to terminal block for stake.

## 6. MUX & Configuration

DIP switch (2-position) on PA15 + PB15:
- 00 = Normal operation (continuous cycling)
- 01 = Calibration mode (manual lid control)
- 10 = Factory test mode
- 11 = Bootloader (DFU via USB)

## 7. Thermal Management

Maximum power dissipation on the board:
- TPS63070: ~400 mW at 500 mA load (85% efficiency at 3.3V from 12V)
- STM32U5: ~60 mW active, 5 µW sleep
- SX1262: ~500 mW during TX (+22 dBm, 30 mA at 3.3V)
- **Total peak: ~1.1 W**

Exposed pad on TPS63070 (PGND) requires 1 cm² copper pour on bottom layer. The ABS IP65 enclosure has natural convection adequate for 1.1 W dissipation with <15°C internal temperature rise.