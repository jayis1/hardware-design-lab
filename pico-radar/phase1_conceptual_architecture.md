# PicoRadar — Phase 1: Conceptual Architecture

## 1. System Purpose

PicoRadar is a 60GHz FMCW (Frequency-Modulated Continuous Wave) radar development platform designed for gesture recognition, presence detection, people counting, and short-range imaging. It combines Texas Instruments' IWR6843AOP single-chip mmWave radar SoC with an STM32H743 host processor, WiFi/BLE connectivity, Ethernet, and a 1.3" OLED — enabling standalone radar applications or cloud-connected sensing nodes.

**Target Markets:** Smart building / HVAC occupancy, automotive in-cabin monitoring, industrial safety zones, gesture-based HMI, robotics collision avoidance, healthcare vital-signs monitoring.

## 2. Performance Targets

| Parameter | Target | Notes |
|---|---|---|
| Frequency band | 60–64 GHz | ISM band, 4 GHz sweep BW |
| Range resolution | 3.75 cm | BW = 4 GHz → ΔR = c/(2·BW) |
| Max range | 12 m (default), 50 m (extended) | Configurable chirp profile |
| Velocity resolution | 0.3 m/s @ 50 ms frame | 256 chirps, T_frame = 50 ms |
| Angular resolution | 15° (3 TX MIMO) | IWR6843AOP on-chip antennas |
| Point cloud rate | 20 fps (default), up to 30 fps | Configurable |
| Latency (raw→point cloud) | < 5 ms | Hardware accelerator + DSP |
| Host boot time | < 2 s | STM32H7 from QSPI flash |
| Power consumption | < 3.5 W active, < 0.5 W idle | Radar + host + WiFi |
| WiFi throughput | ≥ 10 Mbps sustained | For point-cloud streaming |
| Ethernet | 10/100 Mbps | Optional PoE (802.3af) |
| Ambient temp range | –20 °C to +60 °C | Industrial grade |

## 3. Constraints

- **Size:** 85 mm × 55 mm (credit-card footprint) — must fit behind standard wall plates
- **Power:** USB-C 5 V / 1 A minimum; optional PoE 48 V via Ethernet jack
- **Regulatory:** FCC Part 15.255 (60 GHz), ETSI EN 302 264; radar SoC handles RF compliance internally
- **Cost target:** BOM < $85 at 1K units
- **No external antenna:** IWR6843AOP uses on-package antenna-in-package (AiP)
- **Software:** TI mmWave SDK on radar side; FreeRTOS + custom stack on host side
- **No Linux on host:** STM32H7 runs bare-metal FreeRTOS for deterministic latency

## 4. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        PicoRadar Top-Level                       │
│                                                                  │
│  ┌──────────────┐    UART/CSI     ┌──────────────────────────┐   │
│  │  IWR6843AOP  │◄──────────────►│     STM32H743VIT6         │   │
│  │  60GHz FMCW  │  (Host IF)     │  (Cortex-M7 @ 480 MHz)   │   │
│  │  Radar SoC   │                │                          │   │
│  │  3TX / 4RX   │                │  ┌────────┐ ┌─────────┐  │   │
│  │  C674x DSP   │                │  │QSPI    │ │ 1 MB    │  │   │
│  │  HWA 1.0     │                │  │Flash   │ │ SRAM    │  │   │
│  │  576 KB L3   │                │  │MX25L   │ │         │  │   │
│  └──────┬───────┘                │  └────────┘ └─────────┘  │   │
│         │                        │                          │   │
│   Sync/  │    ┌───────┐          │  ┌──────┐  ┌──────────┐ │   │
│   GPIO   │    │ICM-   │  SPI1   │  │ETH   │  │ESP32-C3  │ │   │
│         │    │42688  │◄────────►│  │PHY   │  │WiFi/BLE │ │   │
│         │    │IMU    │          │  │LAN8720│  │(AT cmd)  │ │   │
│         │    └───────┘          │  └──────┘  └──────────┘ │   │
│         │                       │       │         │         │   │
│         │                       │       │    UART2/SDIO    │   │
│         │                       └───────┼─────────┼─────────┘   │
│         │                               │         │             │
│  ┌──────▼───────┐              ┌─────────▼──┐ ┌───▼──────────┐ │
│  │  PMIC        │              │  RJ45 +     │ │ Chip Ant.   │ │
│  │  TPS65263    │◄── I2C ────│  PoE        │ │ (2.4/5 GHz)  │ │
│  │  (5V→3.3/1.8│              └────────────┘ └──────────────┘ │
│  │   /1.2V)    │                                             │
│  └──────┬──────┘      ┌────────────┐    ┌────────────┐      │
│         │              │ 1.3" OLED  │    │ USB-C      │      │
│         ├─────────────►│ SH1106     │    │ (PWR+CDC)  │      │
│         │              │ 128×64 I2C │    │            │      │
│         │              └────────────┘    └────────────┘      │
│         │                                                     │
│  ┌──────▼───────┐                                          │
│  │ 5V Input     │◄─── USB-C or PoE (TPS2378)             │
│  │ (VBUS/PoE)   │                                          │
│  └──────────────┘                                          │
└─────────────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 Radar Processing Pipeline

```
Antennas (3TX+4RX AiP)
    │
    ▼
RF Front-End (IWR6843 internal)
    │  Chirp generation, mixing, ADC (12-bit, 16 Msps per RX)
    ▼
HWA 1.0 Hardware Accelerator
    │  1D-FFT → 2D-FFT → CFAR detection → AoA estimation
    ▼
C674x DSP
    │  Point-cloud formatting, clustering, classification (optional)
    ▼
UART Host Interface (115200–921600 baud, TLV frames)
    │
    ▼
STM32H743 (Host Processor)
    │  Parse TLV, run application logic, drive peripherals
    ├──► OLED display (I2C) — range/velocity plot, presence indicator
    ├──► ESP32-C3 (UART2) — WiFi point-cloud streaming, BLE config
    ├──► Ethernet (RMII) — MQTT / TCP radar data publish
    ├──► USB-C CDC — serial console, firmware updates
    └──► ICM-42688 (SPI1) — IMU data for sensor fusion
```

### 5.2 Host Communication Paths

| Path | Interface | Protocol | Direction | Bandwidth |
|---|---|---|---|---|
| Radar → Host | UART1 (8-wire) | MMWave SDK TLV | Radar→Host | ~1 Mbps |
| Host → Radar | UART1 | CLI commands | Host→Radar | ~115 kbps |
| Host → WiFi | UART2 | AT commands + binary | Both | ~2 Mbps |
| Host → Ethernet | RMII | TCP/MQTT | Both | ~10 Mbps |
| Host → IMU | SPI1 @ 10 MHz | Raw registers | Both | ~10 Mbps |
| Host → OLED | I2C1 @ 400 kHz | SH1106 cmd/data | Host→Dev | ~400 kbps |
| Host → USB | USB FS CDC | Serial | Both | ~12 Mbps |
| Host → PMIC | I2C1 @ 400 kHz | PMBus | Both | ~100 kbps |

## 6. Bus Topology

```
                        ┌─────────────────────┐
                        │     STM32H743VIT6    │
                        │     (Host CPU)      │
                        └──┬──┬──┬──┬──┬──┬──┘
                           │  │  │  │  │  │
              ┌────────────┘  │  │  │  │  └─────────────┐
              │               │  │  │  │                │
          UART1_TX/RX     SPI1   │  │  I2C1         UART2_TX/RX
              │          (MOSI/  │  │ (SCL/SDA)          │
              │           MISO/ │  │                    │
              │           SCK/  │  │                    │
              │           NSS)  │  │                    │
              ▼               ▼  │  ▼                    ▼
        ┌──────────┐   ┌──────┐ │ ┌──────────┐   ┌──────────┐
        │IWR6843   │   │ICM-  │ │ │SH1106    │   │ESP32-C3  │
        │AOP       │   │42688 │ │ │OLED      │   │WiFi/BLE  │
        │(Radar)   │   │(IMU) │ │ │(Display) │   │          │
        └──────────┘   └──────┘ │ └──────────┘   └──────────┘
                                 │
                             I2C1 (shared)
                                 │
                                 ▼
                          ┌──────────┐
                          │TPS65263  │
                          │(PMIC)    │
                          └──────────┘

          ┌────────────┐                 ┌────────────┐
          │LAN8720A    │◄─── RMII ──────►│ STM32H743  │
          │(ETH PHY)   │                 │ (MAC)      │
          └─────┬──────┘                 └────────────┘
                │
          ┌─────▼──────┐
          │RJ45 w/PoE  │
          │(TPS2378)   │
          └────────────┘
```

## 7. Power Architecture

```
VBUS (USB-C, 5 V) ──┬──► TPS65263 PMIC
                     │      ├── BUCK1 → 3.3 V @ 2 A  (STM32H7, PHY, ESP32, OLED, IMU)
PoE (48 V) ── TPS2378 ┘    ├── BUCK2 → 1.8 V @ 1 A  (IWR6843 digital, STM32H7 VDD)
                            ├── BUCK3 → 1.2 V @ 1 A  (IWR6843 core, STM32H7 core)
                            ├── LDO1  → 1.0 V @ 0.5 A (IWR6843 RF LDO)
                            └── LDO2  → 3.0 V @ 0.1 A (IWR6843 analog)

Total budget: 5 V @ 1.5 A (7.5 W) input → 3.5 W max load
```

## 8. Clock Tree

| Source | Frequency | Consumer | Notes |
|---|---|---|---|
| 40 MHz XTAL | 40 MHz | IWR6843AOP (REFCLK) | ±10 ppm TCXO |
| 8 MHz XTAL | 8 MHz | STM32H743 (HSE) | ±30 ppm |
| 25 MHz XTAL | 25 MHz | LAN8720A (REFCLK) | ±50 ppm |
| STM32H7 PLL1 | 480 MHz | Cortex-M7 SYSCLK | HSE→PLL→480 |
| STM32H7 PLL2 | 200 MHz | FMC / QSPI | HSE→PLL2 |
| IWR6843 internal PLL | 60–64 GHz | RF LO | From 40 MHz ref |
| ESP32-C3 internal PLL | 160 MHz | WiFi/BLE CPU | From external 40 MHz |

## 9. Reset & Boot Strategy

1. **Power-on:** TPS65263 sequences 3.3 V → 1.8 V → 1.2 V with 2 ms stagger
2. **IWR6843 boot:** Internal ROM loads config from SPI flash (W25Q128) on its dedicated SPI0 bus; runs mmWave SDK demo or custom firmware
3. **STM32H7 boot:** Boot from QSPI (MX25L25645G) at address 0x90000000; FreeRTOS starts in < 500 ms
4. **ESP32-C3 boot:** Internal flash; enters AT-command firmware mode by default; host configures WiFi/BLE over UART2
5. **Coordinated start:** STM32H7 releases IWR6843 from reset (GPIO) after own boot completes; then sends `sensorStart` CLI command

## 10. Mechanical & Environmental

- **PCB:** 85 mm × 55 mm, 8-layer, 1.6 mm thickness
- **Mounting:** 4× M2 standoffs at corners
- **Enclosure:** Snap-fit ABS housing with mmWave-transparent front window (polycarbonate, 1 mm)
- **Connectors:** USB-C (power+data), RJ45 (Ethernet+PoE), 10-pin JTAG/SWD debug header
- **Weight:** < 50 g (bare PCB), < 80 g (with enclosure)
- **Thermal:** IWR6843AOP thermal pad to internal copper plane; STM32H743 heat spreader; max 65 °C junction at 60 °C ambient

## 11. Key Differentiators

1. **On-package antennas** — no external antenna design needed, no RF layout expertise required
2. **Dual connectivity** — WiFi/BLE + Ethernet + USB — covers cloud, local, and debug paths
3. **Integrated IMU** — enables sensor fusion (radar + inertial) for robotics
4. **OLED display** — real-time radar visualization without a companion device
5. **PoE option** — single-cable install for ceiling/deployment (power + data)
6. **Companion app** — React Native mobile app for configuration, point-cloud visualization, and OTA updates