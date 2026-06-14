# NeuroLink — Phase 1: Conceptual Architecture

## 1. System Purpose

NeuroLink is a 32-channel biosignal acquisition platform designed for electromyography (EMG), electroencephalography (EEG), and electrocardiography (ECG) applications. It targets researchers and developers building brain-computer interfaces (BCI), prosthetic control systems, and real-time biofeedback applications.

The device simultaneously samples up to 32 differential analog channels at 24-bit resolution, applies configurable digital filtering in a dedicated DSP coprocessor, and streams processed data over USB-C (high-speed bulk) or Bluetooth 5.0 LE to a host. An on-board accelerometer provides motion-artifact tagging.

### Key Differentiators
- 32 true-differential channels with per-channel programmable gain (1×–24×)
- 24-bit ΔΣ ADCs with simultaneous sampling (no multiplexing skew)
- Real-time DSP pipeline: 4th-order IIR filters, feature extraction (RMS, MAV, WL, SSC)
- Sub-1 µV rms input-referred noise
- < 3 ms end-to-end latency from electrode to host
- Battery-powered operation: 8+ hours on a single charge
- IP54-rated enclosure for clinical and field use

## 2. Performance Targets

| Parameter                     | Target                              |
|-------------------------------|-------------------------------------|
| Number of channels            | 32 differential                      |
| ADC resolution                | 24 bits                              |
| Sampling rate (per channel)   | 250 SPS – 16 kSPS, configurable     |
| Input-referred noise         | < 1 µV rms (BW 0.5–500 Hz)         |
| CMRR                          | > 110 dB at 50/60 Hz                |
| Input impedance               | > 10 GΩ                             |
| Programmable gain             | 1, 2, 4, 6, 8, 12, 24              |
| Bandwidth                     | 0.5 Hz – 5 kHz (configurable)       |
| DSP latency (single block)    | < 1 ms                              |
| End-to-end latency            | < 3 ms                              |
| USB throughput                | 480 Mbps (HS)                       |
| BLE throughput                | 2 Mbps PHY, ~1.3 Mbps effective     |
| Battery life                   | ≥ 8 hours continuous               |
| Power consumption              | ≤ 1.5 W total                       |
| Board dimensions               | 85 mm × 54 mm (credit-card form)   |

## 3. Design Constraints

1. **Size**: Must fit within 85 × 54 mm — credit-card footprint for wearable mounting
2. **Power**: Single-cell LiPo (3.7 V nominal); whole-system budget ≤ 1.5 W
3. **Thermal**: Passive cooling only; max 45°C case temperature at 25°C ambient
4. **Regulatory**: Designed to meet IEC 60601-1 (medical electrical equipment) and IEC 60601-1-2 (EMC)
5. **EMC**: Mixed-signal layout with strict analog/digital partitioning
6. **Cost target**: BOM ≤ $85 at 1K units

## 4. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            NeuroLink Top-Level                              │
│                                                                             │
│  ┌─────────┐   ┌──────────────┐   ┌──────────────┐   ┌────────────────┐   │
│  │ Electrode│──▶│ ESD/EMI     │──▶│ 8× ADS1299   │──▶│ DSP Coprocessor│   │
│  │ Connector│   │ Protection  │   │ (24-bit ΔΣ)  │   │ (iCE40UP5K)    │   │
│  └─────────┘   └──────────────┘   └──────┬───────┘   └───────┬────────┘   │
│                                         │                   │             │
│                                         │  SPI (32 MHz)     │ SPI (30MHz) │
│                                         ▼                   ▼             │
│  ┌──────────────────────────────────────────────────────────────────┐      │
│  │                    STM32H743ZIT6 (Main MCU)                      │      │
│  │  ┌──────────┐ ┌───────────┐ ┌──────────┐ ┌──────────────────┐  │      │
│  │  │ USB HS   │ │ BLE 5.0   │ │ QSPI     │ │ DMA Controllers  │  │      │
│  │  │ (480Mb)  │ │ (nRF52840│ │ Flash    │ │ (2× MDMA + BDMA) │  │      │
│  │  │          │ │  via UART)│ │ (16MB)   │ │                  │  │      │
│  │  └────┬─────┘ └─────┬────┘ └────┬─────┘ └────────┬─────────┘  │      │
│  └───────┼─────────────┼───────────┼────────────────┼────────────┘      │
│          │             │           │                │                     │
│          ▼             ▼           ▼                ▼                     │
│  ┌───────────┐  ┌──────────┐ ┌──────────┐  ┌──────────────┐            │
│  │ USB-C     │  │ nRF52840 │ │ W25Q128  │  │ 512MB DDR3L  │            │
│  │ Connector │  │ Module   │ │ (Boot FW)│  │ (IS43TR16256)│            │
│  └───────────┘  └──────────┘ └──────────┘  └──────────────┘            │
│                                                                         │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │                     Power Management                             │    │
│  │  LiPo (3.7V) ──▶ BQ25895 (Charger) ──▶ TPS62821 (3V3) ──▶ Rails│    │
│  │                                ──▶ TPS62822 (1V8) ──▶ Rails     │    │
│  │                                ──▶ LP5907 (1V2 MCU) ──▶ Rails   │    │
│  └──────────────────────────────────────────────────────────────────┘    │
│                                                                         │
│  ┌──────────────┐   ┌────────────┐   ┌─────────────┐                    │
│  │ LSM6DSOX    │   │ MicroSD    │   │ RGB LED ×4  │                    │
│  │ (IMU)       │   │ Slot       │   │ (Status)    │                    │
│  └──────────────┘   └────────────┘   └─────────────┘                    │
└─────────────────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 Acquisition Pipeline

```
Electrodes → ESD Clamps → ADS1299 (×8, 4ch each)
    ↓ 24-bit ΔΣ @ configurable SPS
    SPI1 (daisy-chain, 32 MHz SCLK)
    ↓
STM32H743 (SPI DMA → MDMA → SRAM buffer)
    ↓
iCE40UP5K FPGA (DSP pipeline via SPI2)
    ├─ DC offset removal
    ├─ 4th-order IIR bandpass/notch
    ├─ Feature extraction (RMS, MAV, WL, SSC)
    └─ Trigger detection
    ↓
STM32H743 (processed samples in SRAM)
    ↓
    ├─ USB HS Bulk IN → Host PC (12 Mbps sustained)
    └─ UART → nRF52840 → BLE 5.0 → Mobile App (1.3 Mbps)
```

### 5.2 Control Path

```
Host/App → USB/BLE → STM32H743 → SPI commands → ADS1299 (gain, SPS, channel config)
                                   → iCE40UP5K (filter coefficients, feature mask)
                                   → Power rails (channel power-down)
```

### 5.3 Impedance Measurement

```
STM32H743 generates AC current (via DAC) → injection MUX → electrode pair
← ADS1299 measures voltage response → STM32H743 calculates |Z| and θ
```

## 6. Bus Topology

| Bus          | Master        | Slaves                    | Speed    | Protocol |
|--------------|---------------|---------------------------|----------|----------|
| SPI1         | STM32H743     | 8× ADS1299 (daisy-chained) | 32 Mbps | SPI Mode 1 |
| SPI2         | STM32H743     | iCE40UP5K (DSP)            | 30 Mbps | SPI Mode 0 |
| SPI3         | STM32H743     | W25Q128 (boot flash)       | 80 Mbps | QSPI     |
| I2C1         | STM32H743     | LSM6DSOX (IMU)             | 3.4 Mbps| I2C HS  |
| I2C2         | STM32H743     | BQ25895 (charger), TMP117 (temp) | 400 kbps | I2C FM+ |
| UART1        | STM32H743     | nRF52840 (BLE module)      | 3 Mbps  | 8N1      |
| USB HS       | STM32H743     | USB-C connector (host)      | 480 Mbps| USB 2.0  |
| SDIO         | STM32H743     | MicroSD card slot           | 50 MHz  | SD 3.0   |
| FMC          | STM32H743     | IS43TR16256 (DDR3L)        | 800 Mbps | DDR3L-1600 |

### Address Map

| Peripheral   | Base Address    | Size    |
|--------------|-----------------|---------|
| SPI1         | 0x40013000      | 1 KB    |
| SPI2         | 0x40003800      | 1 KB    |
| SPI3 (QSPI)  | 0x40006600      | 1 KB    |
| I2C1         | 0x40005400      | 1 KB    |
| I2C2         | 0x40005800      | 1 KB    |
| USART1       | 0x40011000      | 1 KB    |
| USB_OTG_HS   | 0x40040000      | 4 KB    |
| FMC          | 0xA0000000      | 256 MB  |
| MDMA         | 0x50080000      | 1 KB    |
| DMA1         | 0x40020000      | 1 KB    |

## 7. Memory Architecture

```
┌─────────────────────────────────────────────────┐
│ STM32H743 Memory Map                            │
│                                                  │
│  ┌─────────────────┐  0x08000000                 │
│  │ Flash (2 MB)    │  Application firmware       │
│  └─────────────────┘                             │
│  ┌─────────────────┐  0x24000000                 │
│  │ AXI SRAM (512KB)│  DMA buffers, sample FIFOs  │
│  └─────────────────┘                             │
│  ┌─────────────────┐  0x30000000                 │
│  │ SRAM1 (128KB)   │  Stack, heap                │
│  └─────────────────┘                             │
│  ┌─────────────────┐  0x30020000                 │
│  │ SRAM2 (128KB)   │  DSP coefficient tables     │
│  └─────────────────┘                             │
│  ┌─────────────────┐  0x38000000                 │
│  │ SRAM4 (64KB)    │  USB packet buffers         │
│  └─────────────────┘                             │
│  ┌─────────────────┐  0xC0000000                 │
│  │ DDR3L (512MB)   │  Long recordings, waveforms │
│  └─────────────────┘                             │
│  ┌─────────────────┐  0x90000000                 │
│  │ QSPI Flash (16MB)│  Config, FPGA bitstream    │
│  └─────────────────┘                             │
└─────────────────────────────────────────────────┘
```

## 8. Power Architecture

```
┌──────────────┐     ┌─────────────────┐     ┌────────────┐
│ USB-C 5V     │────▶│ BQ25895         │────▶│ LiPo 3.7V  │
│ (VBUS)       │     │ (Charger/Power  │     │ (2000mAh)  │
│              │     │  Path Mgmt)     │     │            │
└──────────────┘     └────────┬────────┘     └─────┬──────┘
                              │                     │
                              ▼                     ▼
                    ┌─────────────────────────────────┐
                    │ VSYS (3.0–4.4V)                 │
                    └──┬──────────┬──────────┬────────┘
                       │          │          │
                       ▼          ▼          ▼
              ┌────────────┐ ┌──────────┐ ┌──────────┐
              │ TPS62821   │ │ TPS62822 │ │ LP5907   │
              │ (3.3V/2A) │ │ (1.8V/2A)│ │ (1.2V/0.3A)
              └─────┬──────┘ └────┬─────┘ └────┬─────┘
                    │             │            │
                    ▼             ▼            ▼
              ┌──────────┐ ┌───────────┐ ┌──────────┐
              │ VDD_3V3  │ │ VDD_1V8   │ │ VDD_1V2  │
              │ (Analog, │ │ (I/O,     │ │ (MCU Core│
              │  ADS1299,│ │  DDR3L,   │ │  FPGA)   │
              │  Periph) │ │  nRF)     │ │          │
              └──────────┘ └───────────┘ └──────────┘
```

## 9. Safety & Isolation

- Patient isolation per IEC 60601-1: USB port uses ADuM3160 digital isolator (5 kV isolation)
- ESD protection: TPD4E05U06 on all electrode inputs (>8 kV contact discharge)
- Battery charging isolated from measurement circuits
- Hardware watchdog (STM32 IWDG) with 4-second timeout
- Over-temperature shutdown: TMP117 monitors board temp, triggers interrupt at 60°C

## 10. Mechanical Concept

- PCB: 85 × 54 mm, 1.6 mm thickness
- Connector: 2× 20-pin Samtec FLE-120-02-G-DV (board-edge, for electrode harness)
- USB-C: Amphenol 12401832E422A (right-angle, through-hole)
- Enclosure: IP54-rated polycarbonate snap-fit shell (custom 3D-printable)
- Battery compartment: Integrated LiPo pouch pocket with strain relief
- LED window: 4 RGB LEDs visible through light pipe