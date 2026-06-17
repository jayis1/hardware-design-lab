# Phase 1: Conceptual Architecture — CarbonFlux Soil CO₂ Flux Monitor

**Author: jayis1**  
**Copyright © 2026 jayis1. All rights reserved.**

---

## 1. Purpose

The CarbonFlux device measures soil CO₂ efflux (µmol CO₂·m⁻²·s⁻¹) using the closed dynamic chamber method. It is designed for continuous, unattended deployment in agricultural fields, research plots, and natural ecosystems.

## 2. Performance Targets

| Metric | Target | Rationale |
|--------|--------|-----------|
| CO₂ accuracy | ±(40 ppm + 5%) | <0.1 µmol·m⁻²·s⁻¹ error at typical flux rates |
| Flux detection limit | <0.1 µmol·m⁻²·s⁻¹ | Detection of low-activity soils (deserts, degraded land) |
| Flux range | 0.1–50 µmol·m⁻²·s⁻¹ | Covers global flux range (tundra to tropical) |
| Measurement interval | 15–360 min | Configurable for research vs. monitoring |
| Battery life (solar) | Continuous | Solar recharge during daylight, 7-day autonomy without sun |
| Data retention | 2+ years | 16 MB flash, hourly records, ring buffer |
| Operating temperature | -20°C to +60°C | Field deployment in all climates |
| IP rating | IP65 (console), IP68 (stake) | Rain, dust, frost, UV exposure |

## 3. Constraints

| Constraint | Value | Reason |
|------------|-------|--------|
| BOM cost | <$400 | Open-source alternative to $10k+ commercial systems |
| Power, average | <50 mW | Solar-powered unattended operation |
| PCB size | ≤120 × 80 mm | Standard enclosure form factor |
| Radio regulation | ETSI 868 MHz / FCC 915 MHz | Global LoRaWAN bands |
| Chamber volume | 4.5 L (20 cm dia. × 14.3 cm) | Standard ecological research collar size |

## 4. Block Diagram

See README.md for the full block diagram.

## 5. System Decomposition

### 5.1 Sensing Subsystem

- **SCD41** — CO₂ concentration measurement, 0–40,000 ppm, I²C
- **DPS310** — Barometric pressure, ±0.002 hPa, I²C
- **TMP117** — Air temperature inside chamber, ±0.1°C, I²C
- **DS18B20 × 3** — Soil temperature at 5/15/30 cm, ±0.5°C, 1-Wire
- **TEROS-11** — Volumetric water content, ±3%, SDI-12
- **SQ-420** — PAR (photosynthetically active radiation), 0–2.5V analog

### 5.2 Processing Subsystem

- **STM32U5A9ZG** — Cortex-M33 @ 160 MHz, 2 MB Flash, 1.5 MB SRAM
  - Runs flux engine (linear regression), LoRaWAN stack, power management
  - TrustZone secure enclave for keys and calibration data

### 5.3 Communication Subsystem

- **SX1262** — LoRa modem, +22 dBm, 868/915 MHz, LoRaWAN 1.0.4
- **nRF52840** (optional) — BLE 5.2 for local data fetching and DFU
- **USB-C** — CDC ACM virtual COM port + DFU mode

### 5.4 Power Subsystem

- **CN3791** — MPPT solar charger, 6 W panel
- **LiFePO₄ 12.8V 12Ah** — Primary battery
- **TPS63070** — Buck-boost converter, 3.3V output
- **RV-8803** — RTC with battery backup, ±3 ppm

### 5.5 Actuation Subsystem

- **SG90** — Micro servo for chamber lid open/close, PWM with stall detection

## 6. Bus Topology

| Bus | Masters | Slaves | Arbitration |
|-----|---------|--------|-------------|
| I²C1 | STM32U5 | SCD41, DPS310, TMP117, RV-8803 | Standard multi-address with individual EN |
| SPI1 | STM32U5 | SX1262 | Chip-select per transaction |
| SPI2 | STM32U5 | W25Q128 | Quad-SPI, chip-select |
| 1-Wire | STM32U5 | DS18B20 × 3 | ROM-matched addressing |
| ADC1 | STM32U5 | PAR, Vbat, Spare | Sequencer scan |
| TIM2_CH1 | STM32U5 | SG90 servo | PWM only |

## 7. Power Domains

| Domain | Voltage | Devices | Typical Current |
|--------|---------|---------|----------------|
| VDD_3V3 | 3.3 V | MCU, all sensors, LoRa, flash | 15 mA (active), 2 µA (sleep) |
| VDD_5V | 5.0 V | Servo, optional BLE module | 150 mA (burst), 0 mA (idle) |
| VBAT_12V | 10–14.6 V | LiFePO₄ battery, solar input | 0–500 mA (charging) |
| VDD_1V8 | 1.8 V | Internal MCU core (via LDO) | 30 mA |

## 8. Protocol & Data Flow

1. **Every measurement cycle (15–360 min):**
   - RTC alarm wakes MCU from Stop 2 mode
   - Open chamber lid (servo to 180°) → wait 60 s equilibration
   - Close lid (servo to 0°) → start accumulation timer (120–300 s)
   - Read SCD41 every 2 s, buffer CO₂ values with timestamps
   - Read DPS310, TMP117 at midpoint of accumulation
   - After accumulation: run linear regression on CO₂ vs. time
   - Compute flux using chamber dimensions, temp, pressure
   - Read soil temperature (DS18B20 × 3), VWC, PAR
   - Pack record into 32-byte binary → write to flash ring buffer
   - Queue LoRaWAN uplink (confirmed) → Open lid
   - Return to Stop 2 sleep

2. **LoRaWAN uplink:**
   - Port 1: Flux measurement record (32 bytes)
   - Port 2: Diagnostic/heartbeat (4 bytes: battery SOC, signal strength, errors)
   - ADR enabled, confirmed messages with 3 retries

3. **BLE local fetch (on demand):**
   - Connect via nRF52840
   - Issue command to dump flash records or stream real-time
   - Initiate calibration routine
   - DFU firmware update