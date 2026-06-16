# Phase 1: Conceptual Architecture — Chronos Pulser

## 1. Device Identity & Purpose

**Chronos Pulser** is a portable, USB-powered precision Time-Domain Reflectometer (TDR) and sub-nanosecond pulse generator. It is designed for field engineers, RF technicians, and hardware validation labs to characterize transmission line impedance, locate cable faults, measure dielectric constant, and validate high-speed digital interconnects — all from a handheld instrument that connects to a laptop or mobile device via USB 3.0.

Traditional TDR instruments (e.g., Tektronix 1502/1503, Keysight 86100D) cost $5,000–$50,000 and weigh 5–15 kg. Chronos Pulser delivers comparable measurement fidelity in a 120×80×30 mm enclosure weighing under 200 g, at a BOM cost under $300.

### Core Measurement Capabilities
| Parameter | Specification |
|-----------|--------------|
| Pulse rise time (10%–90%) | < 100 ps (typ. 65 ps) |
| Pulse amplitude | 250 mV into 50 Ω (programmable 100–500 mV) |
| Pulse width (FWHM) | 300 ps (fixed by SRD) |
| TDC timing resolution | 10 ps (RMS jitter < 5 ps) |
| ADC sample rate | 250 MSPS, 12-bit |
| Effective bandwidth | > 3 GHz (analog frontend) |
| Maximum cable length | ~100 m (RG-58), ~500 m (low-loss) |
| Impedance measurement range | 10 Ω – 200 Ω |
| Impedance accuracy | ±2% (after calibration) |
| Distance accuracy | ±2 mm (short range), ±0.1% (long range) |
| Repetition rate | 1 kHz – 1 MHz (programmable) |
| Averaging depth | 1 – 65,536 acquisitions |

## 2. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        CHRONOS PULSER                              │
│                                                                     │
│  ┌──────────┐   ┌──────────────┐   ┌─────────────┐   ┌──────────┐  │
│  │ USB 3.0  │◄──┤ FT601Q       │◄──┤ Lattice     │──►│ TDC Core │  │
│  │ Type-C   │   │ USB-FIFO     │   │ ECP5-45F    │   │ (FPGA)   │  │
│  │ Conn.    │   │ Bridge       │   │ FPGA        │   │ 10 ps    │  │
│  └──────────┘   └──────────────┘   └──────┬──────┘   └─────┬─────┘  │
│                                           │                 │       │
│                    ┌──────────────────────┼─────────────────┘       │
│                    │                      │                         │
│  ┌─────────────────▼──┐  ┌───────────────▼──────────┐              │
│  │  Pulse Generator   │  │  Analog Frontend (AFE)   │              │
│  │  ┌──────────────┐  │  │  ┌────────────────────┐  │              │
│  │  │ Step Recovery│  │  │  │ AD9230-250        │  │              │
│  │  │ Diode (SRD)  │  │  │  │ 12-bit 250MSPS    │  │              │
│  │  │ MPN: MA44769 │  │  │  │ ADC               │  │              │
│  │  └──────┬───────┘  │  │  └────────┬───────────┘  │              │
│  │         │          │  │           │              │              │
│  │  ┌──────▼───────┐  │  │  ┌────────▼───────────┐  │              │
│  │  │ Avalanche    │  │  │  │ Variable Gain      │  │              │
│  │  │ Transistor   │  │  │  │ Amplifier (VGA)    │  │              │
│  │  │ BFR92A       │  │  │  │ LMH6517            │  │              │
│  │  └──────┬───────┘  │  │  └────────┬───────────┘  │              │
│  │         │          │  │           │              │              │
│  │  ┌──────▼───────┐  │  │  ┌────────▼───────────┐  │              │
│  │  │ Pulse Shaping│  │  │  │ Directional        │  │              │
│  │  │ Network      │  │  │  │ Coupler / Bridge   │  │              │
│  │  └──────┬───────┘  │  │  │ Mini-Circuits      │  │              │
│  │         │          │  │  │ TCD-10-1X+         │  │              │
│  └─────────┼──────────┘  │  └────────┬───────────┘  │              │
│            │              │           │              │              │
│            └──────────────┼───────────┼──────────────┘              │
│                           │           │                             │
│                    ┌──────▼───────────▼──────┐                      │
│                    │   SMA Output Connector  │                      │
│                    │   (50 Ω, edge-launch)   │                      │
│                    └─────────────────────────┘                      │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Power Management                                            │  │
│  │  USB VBUS (5V) ──► TPS63070 Buck-Boost ──► 3.3V (FPGA I/O)  │  │
│  │                 ──► TPS65131 Boost ──► +5V, -5V (analog)     │  │
│  │                 ──► LP5907 LDO ──► 1.2V (FPGA core)          │  │
│  │                 ──► ADP7118 LDO ──► 1.8V (ADC, AFE)          │  │
│  └──────────────────────────────────────────────────────────────┘  │
│                                                                     │
│  ┌──────────────────────────────────────────────────────────────┐  │
│  │  Clock Distribution                                          │  │
│  │  SiT9365 250 MHz LVDS ──► FPGA PLL ──► 250 MHz (ADC)        │  │
│  │                        ──► FPGA PLL ──► 500 MHz (TDC tap)   │  │
│  │                        ──► FPGA PLL ──► 100 MHz (FT601)      │  │
│  └──────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## 3. Data Flow Architecture

### Acquisition Pipeline
```
[Pulse Gen Trigger] → [SRD fires] → [Pulse travels to DUT]
    → [Reflection returns] → [Directional coupler separates incident/reflected]
    → [VGA conditions signal] → [ADC digitizes at 250 MSPS]
    → [FPGA FIFO buffers samples] → [TDC timestamps reflection edge]
    → [USB 3.0 DMA to host] → [App processes & displays]
```

### TDC Measurement Chain
The FPGA implements a tapped delay line TDC using the carry-chain primitives of the ECP5 architecture. A coarse counter runs at 250 MHz (4 ns bins), and the fine interpolator uses 256 carry-chain taps with ~15 ps per tap, achieving 10 ps effective resolution after calibration.

```
Trigger ──► Coarse Counter (250 MHz, 32-bit) ──► 4 ns LSB
         ──► Fine Interpolator (256 taps) ──► ~15 ps LSB
         ──► Calibration LUT (bin-by-bin) ──► 10 ps effective
         ──► Timestamp Packet (48-bit: 32 coarse + 16 fine)
```

### USB 3.0 Data Framing
- FT601Q provides 32-bit parallel FIFO interface at 100 MHz
- Bulk transfer mode, 32 KB packet size
- Theoretical throughput: 400 MB/s (limited to ~300 MB/s by ADC data rate)
- ADC data rate: 250 MSPS × 12-bit = 375 MB/s raw → decimated/averaged to ~50 MB/s
- Command/control channel: USB control endpoint (EP0)
- Data channel: USB bulk endpoint (EP1 IN)

## 4. Bus Topology & Interconnects

### FPGA Internal Bus Architecture
```
                    ┌──────────────────────────────┐
                    │        Wishbone Crossbar      │
                    │   (shared bus, 32-bit data)   │
                    └──────┬───────┬───────┬────────┘
                           │       │       │
              ┌────────────▼┐ ┌────▼──┐ ┌──▼──────────┐
              │ ADC Capture │ │ TDC   │ │ USB Bridge   │
              │ (DMA Master)│ │ Core  │ │ (DMA Master) │
              └──────┬──────┘ └───┬───┘ └──────┬───────┘
                     │            │             │
              ┌──────▼────────────▼─────────────▼───────┐
              │         DDR3 SDRAM Controller           │
              │    (MT41K128M16JT-125, 256 MB)          │
              │    Acquisition Buffer: 64M samples      │
              └────────────────────────────────────────┘
```

### External Bus Connections
| Bus | Pins | Speed | Connected Devices |
|-----|------|-------|-------------------|
| USB 3.0 SS | 4 diff pairs | 5 Gbps | FT601Q → Host |
| ADC LVDS | 6 diff pairs + 1 clk | 250 MHz DDR | AD9230 → FPGA |
| SPI | 4 (CS,CLK,MOSI,MISO) | 50 MHz | FPGA → VGA (LMH6517) |
| I²C | 2 (SDA,SCL) | 400 kHz | FPGA → Temp sensor (TMP117) |
| GPIO | 8 | DC | FPGA → Status LEDs, trigger sync |
| DDR3 | 48 (DQ[15:0], DQS, ADDR, CTRL) | 400 MHz (800 MT/s) | FPGA → SDRAM |

## 5. FPGA Resource Allocation

| Resource | Used | Available | Utilization |
|----------|------|-----------|-------------|
| LUT4 | 18,500 | 43,200 | 43% |
| EBR (block RAM) | 92 | 108 | 85% |
| DSP blocks | 12 | 72 | 17% |
| PLLs | 3 | 4 | 75% |
| SERDES channels | 4 | 8 | 50% |
| I/O pins | 142 | 203 | 70% |

### FPGA Functional Partitioning
1. **TDC Core** (~3,000 LUTs): Carry-chain tapped delay line, calibration engine, timestamp FIFO
2. **ADC Capture** (~2,500 LUTs): LVDS deserializer, data alignment, decimation filter, DMA engine
3. **USB Bridge** (~1,800 LUTs): FT601 FIFO controller, packet framing, CRC generation
4. **DDR3 Controller** (~4,000 LUTs): Open-source LiteDRAM controller, 16-bit wide, 800 MT/s
5. **Pulse Gen Control** (~800 LUTs): Trigger timing, repetition rate generator, avalanche bias DAC
6. **Soft CPU (optional)** (~3,000 LUTs): RISC-V VexRiscv for command processing and calibration
7. **Infrastructure** (~3,400 LUTs): PLLs, reset controller, JTAG, I²C/SPI masters, GPIO

## 6. Performance Targets & Constraints

### Timing Budget
| Stage | Delay | Notes |
|-------|-------|-------|
| FPGA trigger → SRD bias switch | 2.5 ns | LVCMOS output + transistor switching |
| SRD snap transition | 65 ps | MA44769 step recovery time |
| Pulse propagation to DUT connector | 150 ps | ~30 mm microstrip on FR-4 |
| Reflection return (1 m cable) | ~10 ns | RG-58 velocity factor 0.66 |
| Directional coupler transit | 200 ps | TCD-10-1X+ |
| VGA propagation delay | 1.2 ns | LMH6517 |
| ADC pipeline delay | 8 cycles = 32 ns | AD9230-250 |
| FPGA deserializer latency | 4 cycles = 16 ns | Fixed known offset |
| **Total system latency (excl. cable)** | **~52 ns** | Calibrated out |

### Jitter Budget
| Source | Contribution |
|--------|-------------|
| SiT9365 reference clock | 0.5 ps RMS (12 kHz–20 MHz) |
| FPGA PLL jitter | 3 ps RMS |
| TDC carry-chain mismatch | 2 ps RMS (after calibration) |
| ADC aperture jitter | 0.5 ps RMS |
| **Total RMS jitter** | **~3.8 ps RMS** |

### Power Budget
| Rail | Voltage | Max Current | Power | Source |
|------|---------|-------------|-------|--------|
| VCCIO (FPGA I/O) | 3.3V | 400 mA | 1.32 W | TPS63070 |
| VCCAUX (FPGA aux) | 3.3V | 150 mA | 0.50 W | TPS63070 |
| VCCINT (FPGA core) | 1.2V | 800 mA | 0.96 W | LP5907 |
| VCCADC (ADC analog) | 1.8V | 350 mA | 0.63 W | ADP7118 |
| VCCDR (ADC digital) | 1.8V | 120 mA | 0.22 W | ADP7118 |
| VCC_VGA (VGA) | +5V | 80 mA | 0.40 W | TPS65131 |
| VEE_VGA (VGA neg) | -5V | 40 mA | 0.20 W | TPS65131 |
| VCC_SRD (avalanche bias) | +25V | 5 mA | 0.13 W | Boost converter |
| VCC_DDR (DDR3) | 1.35V | 300 mA | 0.41 W | TPS51200 |
| VTT_DDR (DDR3 term) | 0.675V | 200 mA | 0.14 W | TPS51200 |
| **Total** | | | **~4.91 W** | |

USB 3.0 provides up to 4.5W (900 mA @ 5V) standard, and up to 7.5W with USB-PD negotiation. The device targets 5W nominal, within standard USB 3.0 budget.

## 7. Calibration Strategy

### Factory Calibration (stored in SPI flash)
1. **TDC bin-by-bin calibration**: Measure each carry-chain tap delay using statistical code density test with uncorrelated random pulses. Store 256-entry LUT.
2. **ADC offset/gain**: Short input, measure DC offset. Apply known reference, measure gain error.
3. **System propagation delay**: Connect precision short (known length), measure round-trip. Subtract from all measurements.
4. **Temperature compensation**: TMP117 sensor provides die temperature; apply polynomial correction to TDC bins.

### Field Calibration (user-initiated)
1. **Open calibration**: Leave DUT port open, measure 100% positive reflection. Normalize amplitude.
2. **Short calibration**: Connect precision short, measure 100% negative reflection. Verify timing.
3. **Load calibration**: Connect precision 50 Ω terminator, verify zero reflection.

## 8. Companion Application Architecture

The React Native companion app communicates over USB via a custom binary protocol:

```
┌─────────────────────────────────────────────────────┐
│                  React Native App                    │
│                                                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────────────┐  │
│  │ Dashboard│  │ Settings │  │ Waveform Viewer  │  │
│  │ Screen   │  │ Screen   │  │ Screen           │  │
│  └────┬─────┘  └────┬─────┘  └────────┬─────────┘  │
│       │             │                 │             │
│  ┌────▼─────────────▼─────────────────▼─────────┐  │
│  │           Protocol Engine (utils/protocol.js) │  │
│  │  Frame builder, CRC-16, command dispatch      │  │
│  └──────────────────────┬───────────────────────┘  │
│                         │                          │
│  ┌──────────────────────▼───────────────────────┐  │
│  │         USB Serial / WebUSB Bridge           │  │
│  │  (react-native-usb or WebUSB polyfill)       │  │
│  └──────────────────────┬───────────────────────┘  │
│                         │                          │
│                    USB 3.0 Cable                   │
└─────────────────────────┼─────────────────────────┘
                          │
                   ┌──────▼──────┐
                   │ Chronos     │
                   │ Pulser      │
                   └─────────────┘
```

## 9. Safety & Protection

- **ESD Protection**: TVS diodes (TPD4E05U06) on USB data lines, SMA connector protected by ultra-low-capacitance TVS (PESD0402-140, 0.25 pF)
- **Overvoltage**: SMA input rated to ±15V DC with series AC coupling capacitor and clamping diodes
- **Reverse polarity**: USB Type-C connector inherently prevents reverse insertion
- **Thermal shutdown**: TMP117 monitored by FPGA; throttle repetition rate above 70°C, shutdown at 85°C
- **Short-circuit**: Polyfuse (1206L050) on USB VBUS input

## 10. Mechanical Outline

- **PCB dimensions**: 100 mm × 70 mm, 4-layer
- **Enclosure**: Custom CNC aluminum, 120 × 80 × 30 mm
- **Connectors**: 1× USB Type-C (edge), 1× SMA female (edge-launch), 1× Trigger Sync SMA (optional)
- **Indicators**: 3× RGB LED (power, activity, fault) behind light pipe
- **Cooling**: Passive (aluminum enclosure as heatsink); no fan required at 5W

## 11. Development Toolchain

| Tool | Purpose |
|------|---------|
| Yosys + nextpnr-ecp5 | FPGA synthesis & place-and-route |
| Project Trellis | ECP5 bitstream generation |
| LiteX / LiteDRAM | DDR3 controller and SoC infrastructure |
| GCC RISC-V (riscv64-unknown-elf) | Soft CPU firmware |
| KiCad 8.0 | PCB design |
| OpenEMS | Signal integrity simulation |
| React Native 0.76+ | Companion app |
| WebUSB API | Browser-based USB communication |

## 12. Competitive Analysis

| Feature | Chronos Pulser | Tektronix 1503C | Keysight 86100D | Picoscope 9404 |
|---------|---------------|-----------------|-----------------|----------------|
| Rise time | 65 ps | 300 ps | 7 ps (module) | 70 ps |
| TDC resolution | 10 ps | N/A (sampling only) | 0.1 ps | 0.25 ps |
| Weight | 200 g | 9.5 kg | 15 kg | 1.5 kg |
| Power | USB (5W) | AC mains / battery | AC mains | USB (7.5W) |
| Cost (BOM) | $280 | $8,000 | $35,000 | $5,500 |
| Open source | Yes | No | No | No |
| Companion app | Yes (React Native) | No | Windows only | Windows only |
| Portability | Pocket | Backpack | Bench | Laptop-bag |

---

*End of Phase 1 — Conceptual Architecture*
