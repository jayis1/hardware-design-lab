# HexaScope — Phase 1: Conceptual Architecture

## 1. System Purpose

HexaScope is a **6-channel mixed-signal oscilloscope / logic analyzer** designed for embedded developers, hobbyists, and field technicians. It provides:

- **4 analog channels** (2 differential pairs, individually switchable to single-ended)
- **2 digital logic channels** (threshold-adjustable, up to 200 MS/s)
- **USB-C PD data + power delivery** to a host PC or mobile device
- **On-board FPGA signal processing** for real-time triggering, decimation, and protocol decode
- **256 MB DDR3L memory** for deep waveform capture (up to 128 Mpts/channel)
- **Wi-Fi 6 (802.11ax)** for wireless data streaming to the companion app

The device targets a price point under $299 in volume, targeting makers, students, and IoT developers who need a portable, mixed-signal debug instrument.

---

## 2. Performance Targets

| Parameter | Target |
|---|---|
| Analog bandwidth | 100 MHz per channel |
| Sample rate (analog) | 250 MS/s (8-bit ADC) |
| Sample rate (digital) | 200 MS/s |
| Vertical resolution | 8-bit (6-bit effective at highest sweep) |
| Input range | ±20 mV to ±40 V (1-2-5 sequence, 9 ranges) |
| Input impedance | 1 MΩ ∥ 13 pF (BNC), 50 Ω switchable |
| Memory depth | 128 Mpts per channel (shared 256 MB DDR3L) |
| Trigger types | Edge, pulse width, timeout, runt, window, logic, serial pattern |
| Protocol decode | UART, SPI, I²C, CAN, LIN (in FPGA) |
| Latency (trigger-to-capture) | < 50 ns |
| USB transfer rate | ≥ 320 Mbps sustained |
| Wi-Fi throughput | ≥ 80 Mbps sustained |
| Power consumption | ≤ 12 W total (USB-C PD 15 V @ 0.8 A) |
| Warm-up time | < 5 s from power-on to first capture |
| Form factor | 170 mm × 110 mm × 25 mm (handheld) |

---

## 3. Constraints

1. **USB-C PD powered** — no external supply; must operate from 5–20 V PD contract.
2. **Passive cooling only** — no fan; max PCB temp rise 40 °C above ambient.
3. **EMC** — Must pass FCC Part 15 Class B and CE EN 55032.
4. **Safety** — Input channels must withstand ±400 V transient (CAT II); isolation to host via digital isolators on USB data lines.
5. **Cost** — BOM ≤ $120 at 1 K units.
6. **Software** — FPGA gateware is closed-source blob; companion app is open-source (MIT).
7. **Single-board design** — no mezzanine; all components on one 6-layer PCB.

---

## 4. High-Level Block Diagram

```
                         ┌──────────────────────────────────────────────────────┐
                         │                    HexaScope                         │
                         │                                                      │
  CH1 ──BNC──┐           │  ┌──────────┐    ┌──────────────┐    ┌───────────┐   │
  CH2 ──BNC──┤           │  │ 4× ADC   │    │              │    │ USB-C     │   │
  CH3 ──BNC──┼─ Att/ACL ─┤  │ ADS61B49 │────│              │    │ PD PHY    │   │
  CH4 ──BNC──┤           │  │ 250MS/s  │    │   Lattice   │    │ FUSB302   │──┼── Host
             │           │  └──────────┘    │   NX-17     │    │ + STM32  │   │  PC /
  D1 ──MCX───┤           │                  │  ECP5-5G    │    │ USB3340  │   │  Phone
  D2 ──MCX───┘           │  ┌──────────┐    │              │    └─────┬─────┘   │
                         │  │ 2× CMP   │    │              │          │         │
                         │  │ LT1715   │────│   FPGA      │    ┌─────┴─────┐   │
                         │  │ 200MS/s  │    │              │    │ Digital   │   │
                         │  └──────────┘    │  ┌────────┐  │    │ Isolator  │   │
                         │                  │  │DDR3L   │  │    │ ISO7740  │   │
                         │  ┌──────────┐    │  │MT41K   │  │    └───────────┘   │
                         │  │ PMIC     │    │  │256MB   │  │                    │
                         │  │ DA9063   │    │  └────────┘  │    ┌───────────┐   │
                         │  └────┬─────┘    │              │    │ Wi-Fi 6   │   │
                         │       │          │              │    │ ESP32-C6  │   │
                         │  ┌────┴─────┐    │              │    └───────────┘   │
                         │  │ Power     │    └──────────────┘                    │
                         │  │ Tree      │         │                             │
                         │  │ 5V/3.3V/  │         │                             │
                         │  │ 1.8V/1.1V │    ┌────┴─────┐                       │
                         │  └───────────┘    │ SPI Flash│                       │
                         │                   │ W25Q256  │                       │
                         │                   └──────────┘                       │
                         └──────────────────────────────────────────────────────┘
```

---

## 5. Data Flow

### 5.1 Acquisition Path

```
Analog Input → Attenuator/ACL (1×/10×/100×) → Anti-Alias Filter (50 MHz LP)
    → ADC (ADS61B49, 250 MS/s, LVDS output)
    → FPGA LVDS RX → Trigger Engine → DDR3L Write FIFO → Capture Buffer

Digital Input → Level Comparator (LT1715, threshold DAC)
    → FPGA LVCMOS RX → Trigger Engine → DDR3L Write FIFO → Capture Buffer
```

### 5.2 Processing & Display Path

```
Capture Buffer (DDR3L) → FPGA Decimation Engine → Protocol Decoder (UART/SPI/I²C/CAN)
    → FIFO → USB3340 USB 3.0 PHY → Digital Isolator (ISO7740) → USB-C → Host

Capture Buffer (DDR3L) → FPGA → UART → ESP32-C6 Wi-Fi 6 → 802.11ax → Mobile App
```

### 5.3 Configuration Path

```
Host USB → ISO7740 → STM32G4 USB control endpoint → STM32G4 SPI → FPGA config registers
Host USB → ISO7740 → STM32G4 I²C → PMIC (DA9063) rail control / measurement
```

---

## 6. Bus Topology

### 6.1 Internal Buses

| Bus | Master | Slaves | Width | Clock | Protocol |
|---|---|---|---|---|---|
| DDR3L | FPGA (L2S) | MT41K256M8 | 16-bit | 400 MHz | DDR3-800 |
| FPGA Config SPI | STM32G4 | W25Q256, FPGA | 1/4-bit | 50 MHz | SPI Mode 0 |
| PMIC I²C | STM32G4 | DA9063 | — | 400 kHz | I²C Fast+ |
| ADC LVDS | 4× ADS61B49 | FPGA | 4× 8-bit diff | 250 MHz | LVDS |
| CMP CMOS | 2× LT1715 | FPGA | 2× 1-bit | — | LVCMOS 1.8 V |
| USB 3.0 | USB3340 | FPGA (ULPI) | 8-bit | 60 MHz | ULPI |
| Wi-Fi UART | FPGA | ESP32-C6 | — | 4 MHz | UART w/ HW flow |
| Threshold DAC SPI | STM32G4 | DAC60508 | — | 20 MHz | SPI Mode 1 |
| JTAG | STM32G4 | FPGA | 4-bit | — | JTAG (TAP) |

### 6.2 Address Map (FPGA Register Space)

| Offset | Size | Peripheral | Description |
|---|---|---|---|
| 0x0000_0000 | 64 KB | Trigger Engine | Trigger type, threshold, hysteresis |
| 0x0001_0000 | 64 KB | Decimator | Sample rate divider, averaging mode |
| 0x0002_0000 | 64 KB | Protocol Decoder | Protocol select, baud rate, config |
| 0x0003_0000 | 4 KB | Channel MUX | Analog/digital channel routing |
| 0x0004_0000 | 4 KB | DAC Control | Threshold voltage per digital channel |
| 0x0005_0000 | 4 KB | Calibration | Gain/offset per analog channel |
| 0x0006_0000 | 256 B | Status | Armed/triggered/done flags, FIFO level |
| 0x0007_0000 | 64 KB | USB DMA | Transfer length, endpoint config |
| 0x0008_0000 | 64 KB | Wi-Fi DMA | Transfer length, UART config |

---

## 7. Power Architecture

```
USB-C PD (5–20 V) ──→ FUSB302 PD Negotiation ──→ VBUS (negotiated, typically 15 V)
                                                       │
                                            ┌──────────┴──────────┐
                                            │   DA9063 PMIC       │
                                            │                     │
                                            ├─→ VDD_CORE  1.1V @ 3A  (FPGA core)
                                            ├─→ VDD_DDR   1.35V @ 1A  (DDR3L VREF)
                                            ├─→ VDD_IO18  1.8V @ 1A  (FPGA IO, ADC, CMP)
                                            ├─→ VDD_IO33  3.3V @ 2A  (STM32, USB, Wi-Fi)
                                            └─→ VDD_ANA   5.0V @ 0.5A (ADC analog, refs)
```

Total budget: ~7.5 W typical, 12 W peak during burst captures.

---

## 8. Trigger Architecture

The FPGA Trigger Engine is a pipelined state machine with < 50 ns latency:

1. **Edge Trigger** — Programmable threshold per channel, rising/falling/both
2. **Pulse Width** — Qualify on pulse duration (<, >, == within tolerance)
3. **Timeout** — Trigger on absence of edge within time window
4. **Runt** — Pulse that crosses first threshold but not second
5. **Window** — Signal enters or exits a voltage window
6. **Logic** — Boolean combination of channel conditions (AND, OR, NAND, NOR)
7. **Serial Pattern** — Protocol-specific start-of-frame pattern (UART start bit, I²C START, etc.)

All trigger conditions are evaluated in parallel across all 6 channels.

---

## 9. Mechanical & Form Factor

- **Enclosure**: Anodized aluminum extrusion, 170 × 110 × 25 mm
- **PCB**: 160 × 100 mm, 6-layer, ENIG finish
- **Connectors**:
  - 4× BNC (analog inputs, front)
  - 2× MCX (digital inputs, front)
  - 1× USB-C (data + power, rear)
  - 1× SMA (external trigger in/out, rear)
  - 1× microSD (firmware update + offline storage)
- **User Interface**: 4 tactical switches (Run/Stop, Single, Auto, Force) + 4 LEDs (Power, Run, Trigger, USB)

---

## 10. Key Design Decisions

| Decision | Choice | Rationale |
|---|---|---|
| FPGA vendor | Lattice ECP5 | Low cost, adequate LUT count (5G = 45K LUTs), free tools (Trellis) |
| MCU | STM32G474 | USB 2.0 HS, rich analog for calibration DACs, Cortex-M4F |
| ADC | ADS61B49 | 250 MS/s 8-bit, LVDS, low jitter, well-characterized |
| DDR3L | MT41K256M8TW-107 | 256 MB, 1.35 V, commodity, reliable supply |
| USB | USB3340 + STM32G4 | ULPI PHY for 480 Mbps, STM32 handles control plane |
| Isolation | ISO7740 | 4-channel digital isolator, 150 Mbps, 5 kV isolation |
| Wi-Fi | ESP32-C6 | 802.11ax, low cost, integrated stack, UART interface |
| PMIC | DA9063 | 6 configurable LDO/Buck outputs, I²C control, proven |