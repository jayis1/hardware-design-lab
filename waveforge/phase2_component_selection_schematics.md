# WaveForge — Phase 2: Component Selection & Schematics

## 1. Bill of Materials (BOM)

| Ref | Part Number | Description | Package | Qty | Unit Price (1K) | Total |
|---|---|---|---|---|---|---|
| U1 | STM32H743VIT6 | MCU, Cortex-M7, 480 MHz, 2 MB Flash, 1 MB RAM | LQFP-100 | 1 | $12.50 | $12.50 |
| U2 | WM8778SEDS/RV | Audio codec, 24-bit, 96 kHz, 110 dB SNR, 2in/2out | QFN-28 | 1 | $3.80 | $3.80 |
| U3 | W25Q256JVEIQ | QSPI flash, 32 MB, 133 MHz | WSON-8 | 1 | $1.20 | $1.20 |
| U4 | TPA6130A2RTJR | Headphone amp, 128 dB SNR, 138 mW | QFN-16 | 1 | $0.90 | $0.90 |
| U5 | 24C256-WMN | EEPROM, 256 Kbit, I2C | SOIC-8 | 1 | $0.30 | $0.30 |
| U6 | TLV6256933PDDCR | Buck converter, 5V→1.2V, 3 A | SOT-563 | 1 | $0.45 | $0.45 |
| U7 | LD39050PUR | LDO, 5V→3.3V, 500 mA | DFN-6 | 1 | $0.35 | $0.35 |
| U8 | LD39050PUR | LDO, 5V→1.8V, 500 mA | DFN-6 | 1 | $0.35 | $0.35 |
| U9 | USB3300-EZK | USB 2.0 FS PHY (internal in H743, but ULPI header) | N/A (internal) | 0 | — | — |
| U10 | TPD4E05U06DQAR | ESD protection, USB | USON-10 | 1 | $0.25 | $0.25 |
| U11 | 6N138 | Optocoupler, MIDI IN | DIP-8 | 1 | $0.40 | $0.40 |
| U12 | ADM3202ARNZ | RS232 transceiver (MIDI OUT UART) | TSSOP-16 | 0 | — | — |
| Y1 | 8.000 MHz | HSE crystal, 20 ppm, 12 pF | HC49 | 1 | $0.25 | $0.25 |
| Y2 | 32.768 kHz | LSE crystal, 20 ppm, 6 pF | FC-135 | 1 | $0.12 | $0.12 |
| J1 | USB-C-SMD-16P | USB-C receptacle, 16-pin, SMD | SMD | 1 | $0.50 | $0.50 |
| J2 | DIN-5-MIDI | 5-pin DIN, MIDI IN | PCB mount | 1 | $0.60 | $0.60 |
| J3 | DIN-5-MIDI | 5-pin DIN, MIDI THRU | PCB mount | 1 | $0.60 | $0.60 |
| J4 | Barrel-5.5-2.1 | DC barrel jack, 7–12 V input | SMD | 1 | $0.30 | $0.30 |
| J5 | TRS-3.5MM | 3.5 mm line out L | SMD | 1 | $0.25 | $0.25 |
| J6 | TRS-3.5MM | 3.5 mm line out R | SMD | 1 | $0.25 | $0.25 |
| J7 | TRS-3.5MM | 3.5 mm headphone out | SMD | 1 | $0.35 | $0.35 |
| J8 | TRS-3.5MM | 3.5 mm line in L | SMD | 1 | $0.25 | $0.25 |
| J9 | TRS-3.5MM | 3.5 mm line in R | SMD | 1 | $0.25 | $0.25 |
| J10 | Pin header 2×5 | CV input header (CV1–CV4, GND) | 2.54 mm | 1 | $0.15 | $0.15 |
| J11 | SWD 2×5 | Debug header, ARM 10-pin | 2.54 mm | 1 | $0.15 | $0.15 |
| C1–C20 | 100 nF, 25 V, X7R | Decoupling caps, 0402 | 0402 | 20 | $0.005 | $0.10 |
| C21–C24 | 10 µF, 10 V, X5R | Bulk decoupling, 0805 | 0805 | 4 | $0.03 | $0.12 |
| C25 | 22 µF, 6.3 V, X5R | LDO output, 0805 | 0805 | 1 | $0.05 | $0.05 |
| C26 | 22 µF, 6.3 V, X5R | LDO output, 0805 | 0805 | 1 | $0.05 | $0.05 |
| C27 | 47 µF, 6.3 V, X5R | Buck output, 0805 | 0805 | 1 | $0.07 | $0.07 |
| C28–C31 | 27 pF, 50 V, C0G | HSE load caps | 0402 | 4 | $0.005 | $0.02 |
| C32 | 1 µF, 25 V, X7R | WM8778 decoupling | 0402 | 1 | $0.01 | $0.01 |
| R1 | 220 Ω, 1/16 W | MIDI IN resistor | 0402 | 1 | $0.005 | $0.005 |
| R2 | 1 kΩ, 1/16 W | MIDI IN opto LED | 0402 | 1 | $0.005 | $0.005 |
| R3 | 220 Ω, 1/16 W | MIDI THRU out | 0402 | 1 | $0.005 | $0.005 |
| R4–R7 | 100 kΩ, 1/16 W | CV input dividers | 0402 | 4 | $0.005 | $0.02 |
| R8–R11 | 10 kΩ, 1/16 W | CV input series protection | 0402 | 4 | $0.005 | $0.02 |
| R12, R13 | 3.3 kΩ, 1/16 W | I2S MCLK divider/filter | 0402 | 2 | $0.005 | $0.01 |
| R14 | 470 Ω, 1/16 W | USB CC pull-up | 0402 | 1 | $0.005 | $0.005 |
| R15, R16 | 5.1 kΩ, 1/16 W | USB CC pull-down | 0402 | 2 | $0.005 | $0.01 |
| R17 | 10 kΩ, 1/16 W | NRST pull-up | 0402 | 1 | $0.005 | $0.005 |
| R18 | 10 kΩ, 1/16 W | BOOT0 pull-down | 0402 | 1 | $0.005 | $0.005 |
| L1 | 2.2 µH, 3 A | Buck inductor, TLV62569 | 3×3 mm | 1 | $0.15 | $0.15 |
| D1 | LED green 0402 | Power indicator | 0402 | 1 | $0.01 | $0.01 |
| D2 | LED amber 0402 | MIDI activity | 0402 | 1 | $0.01 | $0.01 |
| FB1, FB2 | 600 Ω @ 100 MHz | Ferrite bead, power rail filter | 0402 | 2 | $0.02 | $0.04 |

**Total BOM (approx):** ~$25.00 at 1000 units

## 2. Pinout Tables

### 2.1 STM32H743VIT6 (U1) — LQFP-100 Pin Assignment

| Pin | Name | Function | Connection | Notes |
|---|---|---|---|---|
| 1 | VBAT | Power | 3.3 V via VBAT rail | Backup domain |
| 2 | PC13 | GPIO | CV_IN4 (ADC1_IN13) | Analog CV input 4 |
| 3 | PC14 | GPIO | HSE osc out | 8 MHz crystal |
| 4 | PC15 | GPIO | HSE osc in | 8 MHz crystal |
| 5 | PF0 | OSC_IN | LSE osc in | 32.768 kHz |
| 6 | PF1 | OSC_OUT | LSE osc out | 32.768 kHz |
| 7 | NRST | Reset | NRST pull-up + SWD header | Active low |
| 8 | PC0 | ADC | CV_IN1 (ADC1_IN10) | Analog CV input 1 |
| 9 | PC1 | ADC | CV_IN2 (ADC1_IN11) | Analog CV input 2 |
| 10 | PC2 | ADC | CV_IN3 (ADC1_IN12) | Analog CV input 3 |
| 11 | PC3 | GPIO | MIDI_IN (via opto) | UART4_RX, 31250 baud |
| 12 | PA0 | GPIO | BOOT0 pull-down | Boot configuration |
| 13 | PA1 | I2S | I2S1_SDO (data out to codec) | Audio TX |
| 14 | PA2 | I2S | I2S1_SDI (data in from codec) | Audio RX (full-duplex) |
| 15 | PA3 | I2S | I2S1_BCLK | Audio bit clock |
| 16 | PA4 | I2S | I2S1_LRCK | Audio LR clock |
| 17 | PA5 | SPI | SPI1_SCK | QSPI flash clock |
| 18 | PA6 | SPI | SPI1_MISO | QSPI flash data in |
| 19 | PA7 | SPI | SPI1_MOSI | QSPI flash data out |
| 20 | PC4 | I2C | I2C1_SDA | Codec/EEPROM/HP amp control |
| 21 | PC5 | I2C | I2C1_SCL | Codec/EEPROM/HP amp clock |
| 22 | PB0 | GPIO | LED_POWER (active low) | Power indicator |
| 23 | PB1 | GPIO | LED_MIDI (active low) | MIDI activity |
| 24 | PB2 | SPI | SPI1_NCS (flash chip select) | W25Q256 CS |
| 25 | PE7 | GPIO | MIDI_THRU_OUT | UART4_TX, 31250 baud |
| 26 | PE8 | GPIO | CODEC_RESET (active low) | WM8778 reset |
| 27 | PE9 | GPIO | HPAMP_EN | TPA6130 enable |
| 28 | PE10 | I2S | I2S1_MCLK | Master clock (256×fs) |
| 29 | PE11 | USB | USB_DP | USB FS data+ |
| 30 | PE12 | USB | USB_DM | USB FS data- |
| 31 | PE13 | GPIO | SWDIO | Debug |
| 32 | PE14 | GPIO | SWCLK | Debug |
| 33 | PE15 | GPIO | SWO | Debug trace |
| 34 | PB3 | GPIO | BUTTON (active low) | User button |
| 35 | PB4 | GPIO | SPI1_NCS_ALT | Future expansion |
| 36–42 | VDD/VSS | Power | 3.3 V / GND | Decouple each pair |
| 43 | PB5 | I2S | I2S2_SDI | Line-in ADC data |
| 44 | PB6 | I2S | I2S2_BCLK | Line-in bit clock |
| 45 | PB7 | I2S | I2S2_LRCK | Line-in LR clock |
| 46 | PB8 | GPIO | FLASH_WP | W25Q256 write protect |
| 47 | PB9 | GPIO | FLASH_HOLD | W25Q256 hold |
| 48 | VDD | Power | 1.2 V core | VDD_CORE |
| 49 | VDDA | Power | 3.3 V analog | VDDA (filtered) |
| 50 | VREF+ | Reference | 3.3 V (filtered) | ADC reference |

### 2.2 WM8778 (U2) — QFN-28 Pin Assignment

| Pin | Name | Function | Connection | Notes |
|---|---|---|---|---|
| 1 | AGND | Ground | Analog GND | |
| 2 | VMID | Mid-rail | 10 µF to AGND | Virtual ground |
| 3 | AINL | Analog in left | J8 (line in L) via 10 kΩ + 1 µF AC-coupled | |
| 4 | AINR | Analog in right | J9 (line in R) via 10 kΩ + 1 µF AC-coupled | |
| 5 | DGND | Ground | Digital GND | |
| 6 | DVDD | Digital power | 3.3 V (filtered) | |
| 6 | MCLK | Master clock | PE10 (I2S1_MCLK) | 256×fs |
| 7 | BCLK | Bit clock | PA3 (I2S1_BCLK) | |
| 8 | LRCLK | LR clock | PA4 (I2S1_LRCK) | |
| 9 | SDIN | Data in | PA1 (I2S1_SDO) | Audio data from MCU |
| 10 | SDOUT | Data out | PA2 (I2S1_SDI) | Audio data to MCU |
| 11 | SCLK | I2C clock | PC5 (I2C1_SCL) | Control interface |
| 12 | SDATA | I2C data | PC4 (I2C1_SDA) | Control interface |
| 13 | CSB | Chip select | PE8 (CODEC_RESET, inverted) | Active low reset |
| 14 | AOUTL | Analog out left | J5 (line out L) via 100 Ω + 10 µF AC-coupled | |
| 15 | AOUTR | Analog out right | J6 (line out R) via 100 Ω + 10 µF AC-coupled | |
| 16 | HPVDD | HP power | 3.3 V (separate filtered) | |
| 17 | HPGND | HP ground | Analog GND | |
| 18 | HPOUTL | HP out left | J7 (HP out L) via 22 Ω | |
| 19 | HPOUTR | HP out right | J7 (HP out R) via 22 Ω | |
| 20 | AVDD | Analog power | 3.3 V (heavily filtered) | |

### 2.3 W25Q256 (U3) — WSON-8 Pin Assignment

| Pin | Name | Function | Connection | Notes |
|---|---|---|---|---|
| 1 | /CS | Chip select | PB2 (SPI1_NCS) | |
| 2 | DO (IO1) | Data out | PA6 (SPI1_MISO) | QSPI data 1 |
| 3 | /WP | Write protect | PB8 (FLASH_WP) | 10 kΩ pull-up to 3.3 V |
| 4 | GND | Ground | GND | |
| 5 | DI (IO0) | Data in | PA7 (SPI1_MOSI) | QSPI data 0 |
| 6 | CLK | Clock | PA5 (SPI1_SCK) | Up to 133 MHz in QSPI |
| 7 | /HOLD | Hold | PB9 (FLASH_HOLD) | 10 kΩ pull-up to 3.3 V |
| 8 | VCC | Power | 3.3 V | 100 nF decoupling |

## 3. Netlists (Source Pin → Component → Dest Pin)

### 3.1 Power Rails

```
VBARREL (J4.1) → D1_Schottky (protection) → VBUS_NET
VUSB (J1.VBUS) → D1_Schottky (protection) → VBUS_NET
VBUS_NET → U6 (TLV62569) VIN → [Buck] → VDD_CORE_1.2V
VBUS_NET → U7 (LD39050) VIN → [LDO] → VDD_3.3V
VBUS_NET → U8 (LD39050) VIN → [LDO] → VDD_CODEC_1.8V

VDD_CORE_1.2V → U1 (STM32) VDD_CORE pins [21, 48]
VDD_3.3V → U1 (STM32) VDD pins [36-42], VDDA [49], VREF+ [50], VBAT [1]
VDD_3.3V → U2 (WM8778) DVDD [6], AVDD [20], HPVDD [16]
VDD_3.3V → U3 (W25Q256) VCC [8]
VDD_3.3V → U5 (24C256) VCC [8]
VDD_3.3V → U4 (TPA6130) VDD
VDD_CODEC_1.8V → Reserved (codec internal PLL, unused — WM8778 is 3.3V native)
```

### 3.2 I2S Audio Bus (Stereo, 48 kHz, 24-bit, I2S format)

```
U1.PE10 (I2S1_MCLK) → R13 (33 Ω series) → U2.Pin6 (MCLK)
U1.PA3  (I2S1_BCLK)  → R12 (33 Ω series) → U2.Pin7 (BCLK)
U1.PA4  (I2S1_LRCK)                      → U2.Pin8 (LRCLK)
U1.PA1  (I2S1_SDO)                       → U2.Pin9 (SDIN)
U2.Pin10 (SDOUT)                          → U1.PA2 (I2S1_SDI)
```

### 3.3 I2C Control Bus

```
U1.PC5 (I2C1_SCL) → [SDA] → U2.Pin11 (SCLK), U5.Pin6 (SCL), U4.Pin6 (SCL)
U1.PC4 (I2C1_SDA) → [SCL] → U2.Pin12 (SDATA), U5.Pin5 (SDA), U4.Pin5 (SDA)
```
Note: I2C address assignments:
- WM8778: 0x1A (7-bit) — pin-configurable
- 24C256: 0x50 (7-bit) — A0=A1=A2=0
- TPA6130A2: 0x1C (7-bit)

### 3.4 SPI Flash Bus (Mode 0, MSB-first)

```
U1.PA5  (SPI1_SCK)  → U3.Pin6 (CLK)
U1.PA6  (SPI1_MISO) ← U3.Pin2 (DO/IO1)
U1.PA7  (SPI1_MOSI) → U3.Pin5 (DI/IO0)
U1.PB2  (SPI1_NCS)  → U3.Pin1 (/CS)
U1.PB8  (GPIO)      → U3.Pin3 (/WP)
U1.PB9  (GPIO)      → U3.Pin7 (/HOLD)
```

### 3.5 MIDI Input (Opto-Isolated)

```
J2.Pin4 (MIDI_IN current) → R1 (220 Ω) → U11.Pin2 (6N138 LED anode)
U11.Pin3 (6N138 LED cathode) → J2.Pin5 (MIDI_IN GND)
U11.Pin6 (6N138 collector) → U1.PC3 (UART4_RX) with 10 kΩ pull-up to 3.3 V
U11.Pin5 (6N138 base) → R2 (1 kΩ) → GND (speed-up resistor)
U11.Pin8 (6N138 VCC) → 3.3 V via 220 Ω + 100 nF filter
```

### 3.6 MIDI THRU Output

```
U1.PE7 (UART4_TX) → R3 (220 Ω) → J3.Pin5 (MIDI_THRU pin 5)
J3.Pin4 (MIDI_THRU pin 4) → GND via 220 Ω (current loop)
```

### 3.7 USB Interface

```
J1.DP → R14 (470 Ω) → U1.PE11 (USB_DP)
J1.DN → R14 (470 Ω) → U1.PE12 (USB_DM)
J1.CC1 → R15 (5.1 kΩ) → GND  (USB-C sink default)
J1.CC2 → R16 (5.1 kΩ) → GND
J1.VBUS → fuse (500 mA) → VBUS_NET
J1.GND → GND
J1.SHIELD → GND via 1 MΩ || 4.7 nF (ESD)
```

### 3.8 CV Inputs (0–5 V, divided to 0–3.3 V)

```
J10.Pin1 (CV1) → R10 (10 kΩ series) → R4 (100 kΩ to GND) → U1.PC0 (ADC1_IN10)
  (voltage divider: 5V × 100/(100+10) ≈ 4.5V — need adjustment)
  CORRECTED: CV1 → R10 (33 kΩ) → node → R4 (68 kΩ to GND) → ADC
  Division: 5V × 68/(33+68) = 3.37 V, safe for 3.3 V ADC
  Clamp: 3.3 V Zener at ADC pin
  
J10.Pin2 (CV2) → [same divider] → U1.PC1 (ADC1_IN11)
J10.Pin3 (CV3) → [same divider] → U1.PC2 (ADC1_IN12)
J10.Pin4 (CV4) → [same divider] → U1.PC13 (ADC1_IN13)
```

## 4. Decoupling Networks

### 4.1 STM32H743 (U1) — 7 VDD/VSS pairs

| Pair | Capacitors | Notes |
|---|---|---|
| VDD (pin 36–42) | 4 × 100 nF X7R 0402 + 1 × 10 µF X5R 0805 | Place one 100 nF per pair, bulk near corner |
| VDDA (pin 49) | 100 nF X7R 0402 + 1 µF X5R 0402 | Low-ESR, close to pin |
| VBAT (pin 1) | 100 nF X7R 0402 | Backup domain |
| VDD_CORE (pin 48) | 2 × 100 nF X7R 0402 + 1 × 10 µF X5R 0805 | Core supply decoupling |

### 4.2 WM8778 (U2)

| Rail | Capacitors | Notes |
|---|---|---|
| DVDD (pin 6) | 100 nF + 1 µF | Close to pin |
| AVDD (pin 20) | 100 nF + 1 µF | Separate ferrite bead from digital rail |
| HPVDD (pin 16) | 100 nF + 10 µF | Low-ESR, analog rail |
| VMID (pin 2) | 10 µF to AGND | Mid-rail reference |

### 4.3 W25Q256 (U3)

| Pin | Capacitors | Notes |
|---|---|---|
| VCC (pin 8) | 100 nF X7R 0402 | Close to pin |

### 4.4 TLV62569 Buck (U6)

| Component | Value | Notes |
|---|---|---|
| C_IN | 10 µF X5R 0805 | Input bypass |
| C_OUT | 47 µF X5R 0805 + 100 nF | Output filter |
| L1 | 2.2 µH, 3 A, low DCR | 3×3 mm shielded inductor |
| C_BST | 100 nF | Bootstrap cap (integrated in TLV62569) |

## 5. Impedance-Matched Pairs

| Net Pair | Impedance | Routing | Notes |
|---|---|---|---|
| USB_DP / USB_DM | 90 Ω differential | Length-matched within 0.5 mm | USB 2.0 FS, no strict requirement but good practice |
| I2S_BCLK / I2S_LRCK | — | Length-matched within 5 mm | Clock pair, less critical |
| SPI_SCK / SPI_MISO/MOSI | — | Length-matched within 10 mm | Up to 80 MHz, moderate matching |

## 6. Pull-Up / Pull-Down Values

| Net | Resistor | Value | Purpose |
|---|---|---|---|
| I2C1_SCL | R20 | 4.7 kΩ to 3.3 V | I2C pull-up |
| I2C1_SDA | R21 | 4.7 kΩ to 3.3 V | I2C pull-up |
| NRST | R17 | 10 kΩ to 3.3 V | Reset pull-up |
| BOOT0 | R18 | 10 kΩ to GND | Boot from flash |
| USB_CC1 | R15 | 5.1 kΩ to GND | USB-C sink detect |
| USB_CC2 | R16 | 5.1 kΩ to GND | USB-C sink detect |
| FLASH_WP | R22 | 10 kΩ to 3.3 V | Write-protect default off |
| FLASH_HOLD | R23 | 10 kΩ to 3.3 V | Hold default off |
| MIDI_RX | R24 | 10 kΩ to 3.3 V | Opto collector pull-up |