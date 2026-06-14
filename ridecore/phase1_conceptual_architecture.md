# RideCore — Phase 1: Conceptual Architecture

## 1. System Purpose

RideCore is a **48 V / 200 A three-phase PMSM motor controller** designed for light electric vehicles — e-bikes, e-scooters, mopeds, and small EV platforms. It implements sensorless Field-Oriented Control (FOC) with space-vector PWM (SVPWM), regenerative braking, and real-time telemetry over CAN FD and BLE 5.0. The controller is intended as an open-hardware platform for EV builders, makers, and OEMs who need a reliable, tunable, and affordable motor drive.

## 2. Key Performance Targets

| Parameter | Target | Notes |
|---|---|---|
| DC bus voltage | 24–60 V nominal (abs max 72 V) | Lead-acid or Li-ion packs |
| Continuous phase current | 120 A RMS | With heatsink |
| Peak phase current | 200 A RMS (10 s) | Thermal-limited |
| PWM frequency | 10–30 kHz | Configurable |
| Control loop rate | 20 kHz | Current loop |
| Switching device | SiC MOSFET or Trench MOSFET | Low RDS(on) |
| Efficiency | > 98% at rated load | Motor-side |
| Communication | CAN FD (5 Mbps) + BLE 5.0 | Dual bus |
| Position sensing | Hall sensors (3×) or sensorless observer | Auto-detect |
| On-board storage | 16 Mb SPI flash | Config logs, firmware |
| Boot time | < 200 ms | From power-on to FOC ready |
| Protections | OVP, UVP, OCP, OTP, DESAT, stall | Full protection suite |

## 3. Constraints

- **Thermal**: 120 A continuous in 40 °C ambient with passive heatsink + PCB copper spreading. Junction ≤ 150 °C.
- **EMC**: Must pass CISPR 25 Class 5 (automotive). Common-mode chokes on phase outputs, Y-caps on HV bus.
- **Safety**: Galvanic isolation between HV bus and logic (digital isolators on gate-driver inputs). IEC 61851-1 compliance for charger pass-through.
- **Cost**: BOM target ≤ $45 USD at 1K volume.
- **Form factor**: 80 mm × 60 mm × 18 mm (fits standard e-bike controller housing).
- **Software**: Apache 2.0 licensed firmware; MIT licensed companion app.

## 4. High-Level Block Diagram

```
                    ┌──────────────────────────────────────────────────┐
                    │                    RideCore                       │
                    │                                                  │
  BAT+ ─────────┐  │  ┌──────────┐   ┌──────────────┐   ┌─────────┐  │  ┌───── PH_A
  (48V)         │  │  │          │   │              │   │ 6×      │  │  │
  BAT- ─────┐   │  │  │  STM32   │   │  Gate       │   │ MOSFET │  │  ├───── PH_B
            │   │  │  │  G474    │   │  Driver     │   │ Half   │  │  │
            │   ├──┤  │  (Cortex │──▶│  3× IRS2186 │──▶│ Bridge  │  │  └───── PH_C
            │   │  │  │  M4 @    │   │  + ISO)     │   │ x3     │  │       │
            │   │  │  │  170 MHz)│   │              │   │        │  │       │
            │   │  │  │          │   └──────────────┘   └────────┘  │       │
            │   │  │  │          │                                  │       │
            │   │  │  │   ┌─────┴──────┐  ┌─────────┐  ┌────────┐ │       │
            │   │  │  │   │ ADC       │  │ CAN FD  │  │ BLE    │ │       │
            │   │  │  │   │ 6× shunt  │  │ MCP2518 │  │ nRF528│ │       │
            │   │  │  │   │ + temp    │  │ + TJA14 │  │ 32     │ │       │
            │   │  │  │   └──────────┘  └─────────┘  └────────┘ │       │
            │   │  │  └──────────────────────────────────────────┘       │
            │   │  │                                                  │   │
            │   │  │  ┌────────────┐  ┌──────────┐  ┌──────────────┐  │   │
            │   │  │  │ PMIC      │  │ SPI      │  │ USB-C       │  │   │
            │   │  │  │ TPS65218 │  │ W25Q128 │  │ Console     │  │   │
            │   │  │  └────────────┘  └──────────┘  └──────────────┘  │   │
            │   │  └──────────────────────────────────────────────────┘   │
            │   │                                                      │
            └───┘  HV GND                                               │
                   ─────────────────────────────────────────────────────┘
```

## 5. Data Flow

### 5.1 Power Flow
```
Battery (48V) → Reverse-polarity protection (MOSFET ideal diode) → 
  Bulk capacitors (470 µF × 4, 63 V aluminium polymer) → 
    3× Half-bridge → Motor phases A/B/C
```

### 5.2 Gate Drive Signal Flow
```
STM32G474 PWM (TIM1 CH1-4) → Digital isolator (ADuM3223) → 
  IRS2186 gate driver (bootstrap) → MOSFET gates (6×)
```

### 5.3 Current Sensing Flow
```
Phase shunt (0.5 mΩ) → INA241A current-sense amp (50 V/V gain) → 
  STM32 ADC1/2 (12-bit, 3.75 MSPS interleaved) → FOC ISR
```

### 5.4 Telemetry Flow
```
FOC ISR (20 kHz) → Packaged status frame (100 Hz) → 
  ├─ CAN FD bus (MCP2518FD via SPI1)
  └─ BLE 5.0 (nRF52832 via UART2 @ 1 Mbps)
```

### 5.5 User Configuration Flow
```
Companion App (BLE) → RideCore BLE → UART2 → Command parser → 
  NVM params (SPI flash W25Q128) / runtime FOC tuning
```

## 6. Bus Topology

| Bus | Protocol | Master | Slaves | Speed | Purpose |
|---|---|---|---|---|---|
| IBUS1 | SPI1 | STM32G474 | MCP2518FD (CAN), W25Q128 (flash) | 20 MHz | High-speed comms + storage |
| IBUS2 | I2C1 | STM32G474 | TPS6521801 (PMIC), AT30TS74 (temp) | 400 kHz | Power management, temp |
| IBUS3 | UART2 | STM32G474 | nRF52832 (BLE module) | 1 Mbps | Wireless telemetry |
| IBUS4 | CAN FD | MCP2518FD | External vehicle bus | 5 Mbps | Vehicle network |
| IBUS5 | USB 2.0 FS | STM32G474 | Host PC | 12 Mbps | Debug/console/firmware update |
| IBUS6 | ADC | STM32G474 | 6× shunt amps, 2× temp, VBAT | — | Current/voltage/thermal |
| IBUS7 | PWM (TIM1) | STM32G474 | 3× gate drivers (via isolators) | 20 kHz | Motor drive |

## 7. Memory Map Overview

| Region | Start | Size | Device |
|---|---|---|---|
| Flash (MCU) | 0x0800_0000 | 512 KB | STM32G474 firmware |
| SRAM (MCU) | 0x2000_0000 | 128 KB | Stack, heap, buffers |
| SPI Flash | — | 16 Mb | Config, logs, firmware backup |
| CAN FD Ctrl | SPI-mapped | 8 KB (reg space) | MCP2518FD registers |

## 8. Boot Sequence

1. POR → STM32G474 resets, reads option bytes
2. Boot from internal Flash (0x0800_0000)
3. System clock config: HSE 8 MHz → PLL → 170 MHz SYSCLK
4. PMIC init via I2C (TPS6521801: enable 3.3V, 5V rails with sequenced ramp)
5. SPI flash init (W25Q128: verify JEDEC ID, load saved config)
6. CAN FD init (MCP2518FD: set bit timing, enable FIFO, interrupts)
7. BLE module init (nRF52832: UART handshake, set advertising name)
8. ADC calibration (offset trim for each shunt channel)
9. Gate driver enable (de-assert SD pin, verify DESAT fault clear)
10. FOC ready — blink status LED, send CAN "READY" frame