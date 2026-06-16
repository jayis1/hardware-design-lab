# Terramesh — Phase 1: Conceptual Architecture

**Author: jayis1**  
**Copyright © 2026 jayis1**

---

## 1. Purpose

Terramesh is a distributed, low-power, subterranean sensor mesh for continuous geotechnical monitoring of soil slopes, embankments, retaining walls, bridge abutments, and building foundations. The system provides real-time early warning of ground failure precursors (pore pressure rise, soil saturation, micro-tilt, seismic vibration) through a self-healing LoRa mesh network with LTE-M cloud uplink.

## 2. Performance Targets

| Metric | Target | Rationale |
|--------|--------|-----------|
| Pore pressure accuracy | ±0.1% FS (0–500 kPa) | Detect 5 kPa changes indicating perched water table |
| Tilt resolution | 0.001° | Detect micro-movement before catastrophic failure |
| Soil moisture accuracy | ±2% VWC | Distinguish saturated vs. unsaturated conditions |
| Mesh range (per hop) | 2 km LOS | Cover typical slope dimensions |
| End-to-end latency | < 60 s (normal), < 10 s (critical) | Timely evacuation warning |
| Battery life | 5–10 years | Maintenance-free deployment |
| Operating temperature | -30°C to +70°C | Global deployment range |
| IP rating | IP68 (20 m, 72 hr) | Subterranean + flood survival |

## 3. Constraints

- **Power**: No external power available at buried nodes. Must operate on primary cells for 5+ years.
- **Communications**: No wired backhaul. LoRa mesh with LTE-M gateway.
- **Size**: Must fit in 120 mm × 80 mm × 50 mm stainless steel tube.
- **Cost**: Per-node BOM target < $150 for dense deployments.
- **Regulatory**: FCC Part 15 / ETSI EN 300.220 for LoRa, PTCRB/GCF for LTE-M.

## 4. Block Diagram

See README.md Section 3.1 for the full block diagram.

## 5. Bus Topology

| Bus | Speed | Devices | Purpose |
|-----|-------|---------|---------|
| SPI1 | 10 MHz | SX1262, MX25R6435F | LoRa + flash |
| SPI2 | 8 MHz | SCL3300, ADXL372 | Tilt + accelerometer |
| I2C1 | 400 kHz | BME688, RV-3028, ST25DV | Environmental + RTC + NFC |
| I2C2 | 100 kHz | ADS1120 (×2) | Pore pressure ADCs |
| LPUART1 | 9600 baud | BG95-M3 (gateway only) | LTE-M AT commands |
| USART2 | 115200 baud | USB CDC-ACM | Debug + configuration |

## 6. Power Domains

| Domain | Voltage | Max Current | Switched | Notes |
|--------|---------|-------------|----------|-------|
| VBAT | 3.0–7.2 V | 3 A | No | Direct from LiSOCl₂ cells |
| VDD_MCU | 1.8 V | 50 mA | No | STM32U5 backup + RTC always on |
| VDD_3V3 | 3.3 V | 200 mA | PMIC switch | Main logic rail |
| VDD_LTE | 3.8 V | 500 mA | PMIC switch | BG95-M3 only (gateway) |
| VREF | 2.5 V | 5 mA | PMIC switch | ADS1120 reference |

## 7. Timing Budget (Normal Cycle, 10 min interval)

| Operation | Time | Current | Energy |
|-----------|------|---------|--------|
| Sleep | 599.5 s | 2.5 µA | 1.5 mJ |
| Wake + sensor read | 200 ms | 15 mA | 3.0 mJ |
| Classification | 50 ms | 15 mA | 0.75 mJ |
| Flash write | 100 ms | 10 mA | 1.0 mJ |
| LoRa TX (SF10) | 1.0 s | 120 mA | 120 mJ |
| LoRa RX window | 150 ms | 10 mA | 1.5 mJ |
| **Total per cycle** | **~601 s** | **avg 45 µA** | **~128 mJ** |
