# CyberGuard — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| # | Reference | Part Number | Description | Package | Qty | Unit Cost | Manufacturer |
|---|-----------|-------------|-------------|---------|-----|-----------|-------------|
| 1 | U1 | STM32L562QEI6 | Application MCU, Cortex-M33, 512KB Flash, 256KB RAM | UFQFPN48 | 1 | $3.80 | STMicroelectronics |
| 2 | U2 | A71CH-2HN/D8A | Secure Element, ECC/Ed25519, 100KB, I2C | HVQFN32 | 1 | $1.95 | NXP |
| 3 | U3 | PN7150B0HN/C11002 | NFC Controller, ISO 14443 A/B, ISO 18092 | HVQFN40 | 1 | $1.40 | NXP |
| 4 | U4 | FPC1025A1 | Fingerprint sensor, capacitive 160×160 | LGA57 | 1 | $2.50 | FPC/Goodix |
| 5 | U5 | nRF52833-QIAA | BLE 5.3 coprocessor, Cortex-M4, 256KB/64KB | QFN48 | 1 | $2.20 | Nordic |
| 6 | U6 | MX25R1635FZUIL0 | QSPI Flash 16MB, 1.65-3.6V | USON8 4×4 | 1 | $0.35 | Macronix |
| 7 | U7 | STPMIC1APQR | Power management IC, 3× LDO + 1× buck | QFN24 4×4 | 1 | $0.85 | STMicroelectronics |
| 8 | Y1 | ECS-240-10-36-CKM-TR | 32.768 kHz crystal, 12.5 pF | ECX-32 | 1 | $0.12 | ECS |
| 9 | Y2 | ABS07-32.768KHZ-T | 32.768 kHz for nRF52833 | 3.2×1.5mm | 1 | $0.10 | Abracon |
| 10 | Y3 | ABS25-32.000MHZ-6-T | 32 MHz crystal for nRF52833 | HC49/SMD | 1 | $0.15 | Abracon |
| 11 | FB1 | BLM18PG121SN1D | Ferrite bead, 120Ω @ 100MHz | 0603 | 1 | $0.01 | Murata |
| 12 | L1 | NRP4018T4R7MJRF | Inductor 4.7µH, 1.2A, buck converter | 4×4mm | 1 | $0.12 | Taiyo Yuden |
| 13 | C1-C8 | Various | Decoupling capacitors (see Section 4) | 0402/0603 | 25 | ~$0.003 ea | MLCC |
| 14 | R1-R14 | Various | Resistors (pull-ups, dividers) | 0402 | 14 | ~$0.001 ea | Std thick-film |
| 15 | D1 | PRTR5V0U2X | TVS/ESD protection, USB-C | SOT143B | 1 | $0.12 | Nexperia |
| 16 | D2-D4 | 150040VS73220 | Green LED 565nm | 0402 | 3 | $0.02 ea | Würth |
| 17 | SW1 | SKQGADE010 | Tactile switch, 6×3.5mm | SMD | 1 | $0.05 | Alps |
| 18 | J1 | 124018324606A | USB-C receptacle, 16-pin, SMD | SMD | 1 | $0.25 | Amphenol |
| 19 | J2 | Antenna NFC | 13.56 MHz NFC antenna, 30mm dia | Custom flex | 1 | $0.30 | Custom |
| 20 | BT1 | BAT-HLD-001-THM | CR2032 coin cell holder | SMD | 1 | $0.10 | Linx |

**Total BOM: ~$14.83 @ 1K, ~$11.20 @ 10K**

## 2. Pinout Tables

### 2.1 STM32L562QE (U1) — UFQFPN48

| Pin | Name | Function | Net | Notes |
|-----|------|----------|-----|-------|
| 1 | VBAT | Power | VBAT_CR2032 | Backup domain, coin cell |
| 2 | PC13 | GPIO | nFP_RST | Fingerprint sensor reset |
| 3 | PC14 | GPIO | nSE_RST | A71CH reset |
| 4 | PC15 | GPIO | LED_GREEN | Status LED green |
| 5 | PH0 | OSC_IN | OSC32_IN | 32.768 kHz crystal |
| 6 | PH1 | OSC_OUT | OSC32_OUT | 32.768 kHz crystal |
| 7 | NRST | Reset | nRST (pull-up) | System reset, active low |
| 8 | PC0 | GPIO | nBLE_RST | nRF52833 reset |
| 9 | PC1 | GPIO | BLE_MODE | BLE mode select |
| 10 | PC2 | GPIO | NFC_IRQ | PN7150 interrupt |
| 11 | PC3 | GPIO | nNFC_RST | PN7150 reset |
| 12 | VSS | Ground | GND | — |
| 13 | VDD | Power | VDD_3V3 | Main 3.3V rail |
| 14 | PA0 | TIM2_CH1 | LED_RED | Status LED red |
| 15 | PA1 | TIM2_CH2 | LED_BLUE | Status LED blue |
| 16 | PA2 | USART2_TX | BLE_RX | UART to nRF52833 |
| 17 | PA3 | USART2_RX | BLE_TX | UART from nRF52833 |
| 18 | PA4 | SPI1_NSS | NFC_NSS | SPI chip select for PN7150 |
| 19 | PA5 | SPI1_SCK | NFC_SCK | SPI clock for PN7150 |
| 20 | PA6 | SPI1_MISO | NFC_MISO | SPI MISO from PN7150 |
| 21 | PA7 | SPI1_MOSI | NFC_MOSI | SPI MOSI to PN7150 |
| 22 | PC4 | GPIO | SE_IRQ | A71CH interrupt out |
| 23 | PC5 | I2C1_SDA | SE_SDA | I2C1 data to A71CH |
| 24 | PB0 | I2C1_SCL | SE_SCL | I2C1 clock to A71CH |
| 25 | PB1 | USART1_TX | FP_TX | UART to FPC1025 |
| 26 | PB2 | USART1_RX | FP_RX | UART from FPC1025 |
| 27 | PB10 | I2C2_SCL | PMIC_SCL | I2C2 clock to STPMIC1 |
| 28 | PB11 | I2C2_SDA | PMIC_SDA | I2C2 data to STPMIC1 |
| 29 | VSS | Ground | GND | — |
| 30 | VDD | Power | VDD_3V3 | Main 3.3V rail |
| 31 | PB12 | SPI2_NSS | FLASH_NSS | QSPI Flash chip select |
| 32 | PB13 | SPI2_SCK | FLASH_SCK | QSPI clock |
| 33 | PB14 | SPI2_MISO | FLASH_MISO | QSPI data 0 |
| 34 | PB15 | SPI2_MOSI | FLASH_MOSI | QSPI data 1 |
| 35 | PC6 | GPIO | FLASH_D2 | QSPI data 2 (quad mode) |
| 36 | PC7 | GPIO | FLASH_D3 | QSPI data 3 (quad mode) |
| 37 | PC8 | GPIO | FLASH_SIO2 | Alt QSPI data 2 |
| 38 | PC9 | GPIO | FLASH_SIO3 | Alt QSPI data 3 |
| 39 | PA8 | USB_OTG_FS_SOF | — | Unused (tie low) |
| 40 | PA9 | USB_OTG_FS_VBUS | USB_VBUS | VBUS detect |
| 41 | PA10 | USB_OTG_FS_ID | — | Unused (tie low) |
| 42 | PA11 | USB_OTG_FS_DM | USB_DM | USB data minus |
| 43 | PA12 | USB_OTG_FS_DP | USB_DP | USB data plus |
| 44 | PA13 | SWDIO | SWDIO | Debug (disabled in prod) |
| 45 | PA14 | SWCLK | SWCLK | Debug (disabled in prod) |
| 46 | PA15 | GPIO | nFLASH_WP | QSPI Flash write protect |
| 47 | BOOT0 | Boot config | BOOT0 (pull-down) | Boot from flash |
| 48 | VDDA | Analog power | VDDA_3V3 | ADC / analog reference |

### 2.2 A71CH Secure Element (U2) — HVQFN32

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 1 | VDD | Power | VDD_3V3 |
| 2 | GND | Ground | GND |
| 3 | SCL | I2C Clock | SE_SCL |
| 4 | SDA | I2C Data | SE_SDA |
| 5 | RST_N | Reset | nSE_RST |
| 6 | IRQ_N | Interrupt | SE_IRQ |
| 7-32 | NC/RSVD | — | Tie to GND per datasheet |

**I2C Address: 0x48 (7-bit)**

### 2.3 PN7150 NFC Controller (U3) — HVQFN40

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 1 | VDD | Power | VDD_3V3 |
| 2 | VDD_RF | RF Power | VDD_3V3 (filtered) |
| 3 | GND | Ground | GND |
| 4 | ANT1 | Antenna+ | NFC_ANT_P |
| 5 | ANT2 | Antenna- | NFC_ANT_N |
| 6 | SPI_MOSI | SPI Data In | NFC_MOSI |
| 7 | SPI_MISO | SPI Data Out | NFC_MISO |
| 8 | SPI_SCK | SPI Clock | NFC_SCK |
| 9 | SPI_NSS | SPI Select | NFC_NSS |
| 10 | IRQ | Host IRQ | NFC_IRQ |
| 11 | RST_N | Reset | nNFC_RST |
| 12 | VBAT | Backup | VBAT_CR2032 |
| 13-40 | NC/RSVD | — | Per datasheet |

### 2.4 FPC1025A1 Fingerprint Sensor (U4) — LGA57

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 1 | VDD | Digital Power | VDD_3V3 |
| 2 | VDD_IO | IO Power | VDD_1V8 |
| 3 | GND | Ground | GND |
| 4 | TX | UART TX | FP_TX (to MCU RX) |
| 5 | RX | UART RX | FP_RX (to MCU TX) |
| 6 | RST_N | Reset | nFP_RST |
| 7 | WAKE_N | Wake | FP_WAKE (pull-up to VDD_1V8) |
| 8 | IRQ | Interrupt | FP_IRQ (PC10 on MCU) |
| 9-57 | Sensor Array | Capacitive pads | N/A |

### 2.5 nRF52833 (U5) — QFN48

| Pin | Name | Function | Net |
|-----|------|----------|-----|
| 1 | DEC1 | Decoupling | VDD_NRF (filtered) |
| 2 | P0.00 | XL1 | XOSC32_IN |
| 3 | P0.01 | XL2 | XOSC32_OUT |
| 4 | P0.02 | UART CTS | BLE_CTS (unused, pull-up) |
| 5 | P0.03 | UART RTS | BLE_RTS (unused, pull-up) |
| 6 | P0.04 | UART TX | BLE_TX (to MCU USART2_RX) |
| 7 | P0.05 | UART RX | BLE_RX (to MCU USART2_TX) |
| 8 | P0.06 | GPIO | MCU_NSS (unused) |
| 9 | VDD | Power | VDD_3V3 |
| 10 | GND | Ground | GND |
| 11-24 | P0.07-P0.20 | GPIO/NC | — |
| 25 | P0.21 | RESET | nBLE_RST |
| 26 | SWDIO | Debug | SWDIO_NRF |
| 27 | SWCLK | Debug | SWCLK_NRF |
| 28 | ANT | RF | ANT_BLE (via PI network) |
| 29 | VDD_RF | RF Power | VDD_NRF (filtered) |
| 30-48 | P0.22-P1.15 | GPIO/NC | — |

## 3. Netlists

### 3.1 Power Netlist

```
VBUS ─── J1(A4/B9) ─── D1(VBUS) ─── STPMIC1(VIN) ─── STPMIC1(BUCK_OUT) ─── VDD_3V3
      │                                                   │
      │                                                   ├── C5(100nF) ─── GND
      │                                                   ├── C6(10µF) ─── GND
      │                                                   └── R4(10kΩ) ─── GND (VBUS detect divider)
      │
      └── D1(VBUS_protect) ─── FB1 ─── VBUS_FILTERED

VBAT_CR2032 ─── BT1(+) ─── STPMIC1(VBAT) ─── STM32L562(VBAT)
                                      └── C10(100nF) ─── GND

STPMIC1(LDO1_OUT) ─── VDD_1V8 ─── FPC1025(VDD_IO), nRF52833(VDD_IO)
                                      ├── C11(100nF) ─── GND
                                      └── C12(4.7µF) ─── GND

STPMIC1(LDO2_OUT) ─── VDD_1V1 ─── nRF52833(DEC1)
                                      ├── C13(100nF) ─── GND
                                      └── C14(1µF) ─── GND

VDD_3V3 ─── STM32L562(VDD, VDDA)
        ─── A71CH(VDD)
        ─── PN7150(VDD, VDD_RF)
        ─── FPC1025(VDD)
        ─── nRF52833(VDD)
        ─── MX25R1635F(VDD)

GND ─── All IC grounds, USB-C GND pins (A1/B12), battery negative
```

### 3.2 I2C Bus 1 (Secure Element) Netlist

```
STM32L562(PC5/I2C1_SDA) ─── R1(4.7kΩ pull-up) ─── VDD_3V3
                          ─── A71CH(SDA/Pin4)
                          ─── TP1 (test point)

STM32L562(PB0/I2C1_SCL) ─── R2(4.7kΩ pull-up) ─── VDD_3V3
                          ─── A71CH(SCL/Pin3)
                          ─── TP2 (test point)

A71CH(RST_N/Pin5) ─── STM32L562(PC3/nSE_RST)
A71CH(IRQ_N/Pin6) ─── STM32L562(PC4/SE_IRQ) ─── R3(10kΩ pull-up) ─── VDD_3V3
```

### 3.3 SPI Bus 1 (NFC Controller) Netlist

```
STM32L562(PA4/SPI1_NSS) ─── PN7150(SPI_NSS/Pin9)
STM32L562(PA5/SPI1_SCK) ─── PN7150(SPI_SCK/Pin8)
STM32L562(PA6/SPI1_MISO) ─── PN7150(SPI_MISO/Pin7)
STM32L562(PA7/SPI1_MOSI) ─── PN7150(SPI_MOSI/Pin6)
PN7150(IRQ/Pin10) ─── STM32L562(PC2/NFC_IRQ)
PN7150(RST_N/Pin11) ─── STM32L562(PC3/nNFC_RST)
PN7150(ANT1/Pin4) ─── J2(NFC_ANT_P)
PN7150(ANT2/Pin5) ─── J2(NFC_ANT_N)
```

### 3.4 UART1 (Fingerprint Sensor) Netlist

```
STM32L562(PB1/USART1_TX) ─── R5(100Ω) ─── FPC1025(RX/Pin5)
STM32L562(PB2/USART1_RX) ─── R6(100Ω) ─── FPC1025(TX/Pin4)
FPC1025(RST_N/Pin6) ─── STM32L562(PC13/nFP_RST)
FPC1025(IRQ/Pin8) ─── STM32L562(PC10/FP_IRQ)
FPC1025(WAKE_N/Pin7) ─── R7(10kΩ pull-up) ─── VDD_1V8
```

### 3.5 UART2 (BLE Coprocessor) Netlist

```
STM32L562(PA2/USART2_TX) ─── R8(100Ω) ─── nRF52833(P0.05/RX)
STM32L562(PA3/USART2_RX) ─── R9(100Ω) ─── nRF52833(P0.04/TX)
nRF52833(P0.21/RESET) ─── STM32L562(PC0/nBLE_RST)
nRF52833(P0.02/CTS) ─── R10(10kΩ pull-up) ─── VDD_3V3
nRF52833(P0.03/RTS) ─── R11(10kΩ pull-up) ─── VDD_3V3
```

### 3.6 QSPI Flash Netlist

```
STM32L562(PB12/SPI2_NSS) ─── MX25R1635F(CS#/Pin1)
STM32L562(PB13/SPI2_SCK) ─── MX25R1635F(SCLK/Pin6)
STM32L562(PB14/SPI2_MISO) ─── MX25R1635F(SI/IO0/Pin5)
STM32L562(PB15/SPI2_MOSI) ─── MX25R1635F(SO/IO1/Pin2)
STM32L562(PC6) ─── MX25R1635F(IO2/Pin3)
STM32L562(PC7) ─── MX25R1635F(IO3/Pin7)
MX25R1635F(WP#/Pin3) ─── STM32L562(PA15/nFLASH_WP)
MX25R1635F(HOLD#/Pin7) ─── R12(10kΩ pull-up) ─── VDD_3V3
```

### 3.7 USB Netlist

```
J1(A6/DP) ─── R13(27Ω) ─── D1(DP) ─── STM32L562(PA11/USB_DM)
J1(B6/DN) ─── R14(27Ω) ─── D1(DM) ─── STM32L562(PA12/USB_DP)
J1(A4/B9/VBUS) ─── STPMIC1(VIN)
J1(A1/B12/GND) ─── GND
J1(A5/B5/CC) ─── R15(5.1kΩ) ─── GND (5.1k for 1.5A default)
J1(CC2) ─── R16(5.1kΩ) ─── GND
```

## 4. Decoupling Networks

### 4.1 STM32L562QE

| Rail | Caps | Placement |
|------|------|-----------|
| VDD (3.3V) | 100nF (×4, one per VDD pin) + 10µF (×1, bulk) | Adjacent to pins, short loops |
| VDDA (3.3V) | 100nF + 1µF | Adjacent to VDDA pin |
| VBAT | 100nF | Near VBAT pin |

### 4.2 A71CH

| Rail | Caps | Placement |
|------|------|-----------|
| VDD (3.3V) | 100nF + 1µF | Within 2mm of pin |

### 4.3 PN7150

| Rail | Caps | Placement |
|------|------|-----------|
| VDD (3.3V) | 100nF + 4.7µF | Adjacent to VDD pin |
| VDD_RF | 100nF + 10µF (tantalum) | Adjacent to VDD_RF, ESR < 1Ω |

### 4.4 nRF52833

| Rail | Caps | Placement |
|------|------|-----------|
| VDD (3.3V) | 100nF + 4.7µF | Adjacent to VDD |
| DEC1 | 100nF (per Nordic layout guide) | Adjacent to DEC pin |
| VDD_RF | 100nF + 1µF (per Nordic matching network) | Adjacent to ANT pin |

### 4.5 MX25R1635F

| Rail | Caps | Placement |
|------|------|-----------|
| VDD (3.3V) | 100nF | Adjacent to VDD pin |

### 4.6 FPC1025

| Rail | Caps | Placement |
|------|------|-----------|
| VDD (3.3V) | 100nF + 4.7µF | Adjacent to VDD pin |
| VDD_IO (1.8V) | 100nF + 1µF | Adjacent to VDD_IO pin |

## 5. Impedance-Matched Pairs

| Signal Pair | Target Impedance | Max Length Mismatch | Notes |
|-------------|-----------------|---------------------|-------|
| USB_DP / USB_DM | 90Ω differential | 0.5 mm | Per USB 2.0 spec |
| NFC_ANT_P / NFC_ANT_N | 50Ω differential | N/A | Antenna trace matching |
| BLE_ANT (single-ended) | 50Ω single-ended | N/A | PI matching network to antenna |
| SPI1 (SCK/MISO/MOSI) | N/A (short traces) | < 10 mm total | Length-match within 2mm |
| QSPI Flash (SCLK/IO0-3) | N/A (short traces) | < 15 mm total | Length-match within 3mm |

## 6. Pull-Up / Pull-Down Values

| Net | Resistor | Value | Purpose |
|-----|----------|-------|---------|
| I2C1_SDA | R1 | 4.7kΩ | I2C pull-up (400kHz) |
| I2C1_SCL | R2 | 4.7kΩ | I2C pull-up (400kHz) |
| I2C2_SDA | R17 | 4.7kΩ | I2C pull-up (400kHz) |
| I2C2_SCL | R18 | 4.7kΩ | I2C pull-up (400kHz) |
| SE_IRQ | R3 | 10kΩ | Active-low IRQ pull-up |
| nRST | R19 | 10kΩ | MCU reset pull-up |
| BOOT0 | R20 | 10kΩ | Boot mode pull-down |
| FP_WAKE_N | R7 | 10kΩ | Fingerprint wake pull-up |
| BLE_CTS | R10 | 10kΩ | UART CTS pull-up |
| BLE_RTS | R11 | 10kΩ | UART RTS pull-up |
| FLASH_HOLD | R12 | 10kΩ | QSPI flash hold pull-up |
| USB_CC1 | R15 | 5.1kΩ | USB-C 1.5A CC pull-down |
| USB_CC2 | R16 | 5.1kΩ | USB-C 1.5A CC pull-down |
| USB_VBUS | R4 | 10kΩ | VBUS detect divider |

## 7. Power Tree

```
                        ┌──────────────────┐
                        │   USB-C VBUS     │
                        │   (5V, 500mA)    │
                        └────────┬─────────┘
                                 │
                          ┌──────▼──────┐
                          │  STPMIC1    │
                          │  Buck+LDO   │
                          └──┬──┬──┬──┬─┘
                             │  │  │  │
                  ┌──────────┘  │  │  └─────────────┐
                  │             │  │                 │
            ┌─────▼─────┐ ┌───▼──▼───┐      ┌──────▼──────┐
            │  BUCK_OUT  │ │  LDO1     │      │  LDO2       │
            │  3.3V/500mA│ │  1.8V/150mA│     │  1.1V/100mA │
            └─────┬──────┘ └─────┬─────┘      └──────┬──────┘
                  │              │                    │
         ┌────────┼──────────────┼────────────────────┘
         │        │              │
    VDD_3V3  VDD_1V8       VDD_1V1
    (MCU, SE, (FP IO,       (nRF core,
     NFC, FP,  nRF IO)       nRF DEC1)
     nRF, Flash)
         │
    ┌────┴─────────────────────────┐
    │   CR2032 Battery Backup     │
    │   (VBAT domain when USB     │
    │    disconnected)            │
    └──────────────────────────────┘
```

When USB-C is disconnected, STPMIC1 enters low-power mode. The STM32L562 VBAT domain is powered by CR2032 through an ideal diode OR-ing circuit. BLE and NFC are powered down; only the STM32L562 STOP mode with RTC wakes on touch button press or NFC field detection.