# Phase 1: Conceptual Architecture — Nebula Matrix
## Professional LED Matrix Display Engine

---

## 1. System Purpose & Vision

Nebula Matrix is a single-board, FPGA-driven LED matrix display engine designed to
drive large-scale RGB LED panel installations — from concert stage backdrops and
architectural media facades to broadcast studio video walls and interactive art
installations. It replaces the traditional stack of sending card, receiving cards,
and hub boards with a single integrated unit capable of driving up to 256×256
pixels (65,536 RGB LEDs) at 120 Hz refresh with 16-bit per-channel color depth.

The device ingests video over Gigabit Ethernet (Art-Net 4, sACN E1.31, or a
custom low-latency binary protocol), HDMI 2.0 (up to 1080p60), or plays back
pre-rendered content from an onboard SD card. The FPGA handles all real-time
pixel processing — gamma correction, color space conversion, dithering, and
HUB75E waveform generation — while an STM32H7 co-processor manages networking,
USB configuration, file I/O, and system health monitoring.

### Target Applications
- **Live Events**: Concert LED walls, festival stage backdrops, DJ booth displays
- **Broadcast**: Virtual production LED volumes, news studio video walls
- **Architectural**: Building facade media displays, digital signage
- **Installation Art**: Interactive LED sculptures, museum exhibits
- **Retail**: High-end digital signage, immersive brand experiences

---

## 2. Performance Targets

| Parameter                 | Target Value                    | Notes                                      |
|---------------------------|---------------------------------|--------------------------------------------|
| Max Pixel Matrix          | 256 × 256 (65,536 pixels)       | 16× HUB75E ports, each 64×64               |
| Refresh Rate              | 120 Hz at full resolution       | Binary-weighted PWM with BCM modulation    |
| Color Depth               | 16 bits per channel (48 bpp)    | 65,536 levels per R, G, B                 |
| Frame Buffer Capacity     | 4 full frames (double-buffered) | 1 GB DDR3L total                           |
| Ethernet Throughput       | 1 Gbps line rate                | UDP streaming, Art-Net/sACN               |
| HDMI Input                | 1080p60, 8-bit RGB              | Scaled/cropped to matrix resolution        |
| USB Configuration         | USB 2.0 Full Speed              | Web-based config interface via RNDIS       |
| SD Card Playback           | Up to 60 fps from UHS-I card    | Pre-rendered raw pixel stream              |
| End-to-End Latency        | < 2 frames (16.7 ms at 60 Hz)  | From Ethernet packet to LED output         |
| Power Consumption         | < 25 W typical, 35 W max        | Excluding LED panel power                  |
| Operating Temperature     | -20°C to +70°C (industrial)     | Fan-assisted cooling                       |

---

## 3. High-Level Block Diagram

```
                          ┌─────────────────────────────────────────────────────────┐
                          │                    NEBULA MATRIX                         │
                          │                                                         │
  ┌──────────┐            │  ┌─────────────┐    ┌──────────────────────────────┐     │
  │ HDMI IN  │──TMDS──────┼──│ ADV7611     │───▶│                              │     │
  │ (1080p60)│            │  │ HDMI Rx     │    │                              │     │
  └──────────┘            │  └─────────────┘    │                              │     │
                          │                    │                              │     │
  ┌──────────┐            │  ┌─────────────┐    │     XILINX ARTIX-7          │     │
  │ 1GbE RJ45│──MDI───────┼──│ KSZ9031RNX  │───▶│     XC7A100T-2FGG484C      │     │
  │ (PoE opt)│            │  │ GbE PHY     │    │                              │     │
  └──────────┘            │  └─────────────┘    │   ┌──────────────────────┐   │     │
                          │                    │   │ Frame Buffer Ctrl    │   │     │
  ┌──────────┐            │  ┌─────────────┐    │   │ (2× DDR3L, 32-bit)   │───┼──┐  │
  │ USB-C    │──DP/DM─────┼──│ USB3320C    │───▶│   └──────────────────────┘   │  │  │
  │ (Config) │            │  │ ULPI PHY    │    │                              │  │  │
  └──────────┘            │  └─────────────┘    │   ┌──────────────────────┐   │  │  │
                          │                    │   │ Pixel Processing     │   │  │  │
  ┌──────────┐            │  ┌─────────────┐    │   │ Gamma / CSC / Dither │   │  │  │
  │ microSD  │──SPI───────┼──│ SD Socket   │───▶│   └──────────────────────┘   │  │  │
  │ (UHS-I)  │            │  └─────────────┘    │                              │  │  │
  └──────────┘            │                    │   ┌──────────────────────┐   │  │  │
                          │                    │   │ HUB75E Waveform Gen  │───┼──┼──┤
                          │  ┌─────────────┐    │   │ (16 ports, 32ch PWM) │   │  │  │
                          │  │ Si5351A     │───▶│   └──────────────────────┘   │  │  │
                          │  │ Clock Gen   │    │                              │  │  │
                          │  └─────────────┘    └──────────┬───────────────────┘  │  │
                          │                                │                      │  │
                          │  ┌────────────────────────────┐│                      │  │
                          │  │ STM32H743VIT6 (Cortex-M7)  ││                      │  │
                          │  │ - System Management        ││                      │  │
                          │  │ - Ethernet Stack (LWIP)    │◀┼── SPI + UART ───────┘  │
                          │  │ - USB RNDIS Config         ││                      │  │
                          │  │ - SD Card Filesystem        ││                      │  │
                          │  │ - Temp/Fan Monitoring      ││                      │  │
                          │  │ - FPGA Config (SPI Flash)  ││                      │  │
                          │  └────────────────────────────┘│                      │  │
                          │                                │                      │  │
                          │  ┌────────────────────────────┐│                      │  │
                          │  │ POWER TREE                ││                      │  │
                          │  │ TPS65218 PMIC             ││                      │  │
                          │  │ +3.3V, +1.8V, +1.35V,    ││                      │  │
                          │  │ +1.0V (FPGA Core), +1.2V  ││                      │  │
                          │  └────────────────────────────┘│                      │  │
                          └────────────────────────────────┼──────────────────────┼──┘
                                                           │                      │
                                          ┌────────────────┼──────────────────────┼────────────┐
                                          │  16× SN74LVCH16T245 Level Shifters    │            │
                                          │  (3.3V FPGA → 5.0V HUB75E)             │            │
                                          └────────────────┼──────────────────────┼────────────┘
                                                           │                      │
                                          ┌────────────────┼──────────────────────┼────────────┐
                                          │          16× HUB75E OUTPUT PORTS                    │
                                          │  R1,G1,B1,R2,G2,B2, A,B,C,D,E, CLK, LAT, OE, GND   │
                                          │  Each port drives up to 64×64 panel                │
                                          └────────────────────────────────────────────────────┘
```

---

## 4. Data Flow Architecture

### 4.1 Video Input Paths

**Path A — Ethernet Streaming (Primary)**
```
Ethernet Frame (UDP) → KSZ9031 PHY → RGMII → STM32H7 MAC
  → LWIP Stack → Art-Net/sACN Parser → Pixel Buffer (SRAM)
  → SPI DMA (50 MHz) → FPGA Pixel FIFO → Frame Buffer (DDR3)
  → Pixel Processing Pipeline → HUB75E Output
```

**Path B — HDMI Capture**
```
HDMI TMDS → ADV7611 → 24-bit Parallel RGB + HSYNC/VSYNC/DE
  → FPGA Capture Block → Scaler/Cropper → Frame Buffer (DDR3)
  → Pixel Processing Pipeline → HUB75E Output
```

**Path C — SD Card Playback**
```
microSD → STM32H7 SDMMC (4-bit, 50 MHz) → FatFS
  → Raw Frame Reader → SPI DMA → FPGA Pixel FIFO
  → Frame Buffer (DDR3) → Pixel Processing Pipeline → HUB75E Output
```

### 4.2 Pixel Processing Pipeline (FPGA)

```
Frame Buffer Read (DDR3, 256-bit AXI)
  → Chroma Resampler (4:4:4 if needed)
  → Color Space Conversion (YUV→RGB or RGB→RGB)
  → Gamma Correction LUT (16-bit in, 16-bit out, 3× 64K-entry BRAM LUTs)
  → Temporal Dithering Engine (Floyd-Steinberg + Blue Noise)
  → Brightness/Contrast Adjust
  → Scan-Rate Converter (input fps → 120 Hz output)
  → HUB75E Waveform Generator
    - Binary-Weighted PWM (BCM) with 16-bit resolution
    - 32 parallel channels per port (16 ports × 32 = 512 total PWM channels)
    - GCLK-driven timing with programmable OE blanking
  → Level Shifters → HUB75E Connectors
```

### 4.3 Control & Configuration Path

```
USB-C → USB3320C ULPI → STM32H7 USB OTG
  → TinyUSB Stack → RNDIS (Ethernet-over-USB)
  → Lightweight HTTP Server (Mongoose/lwIP)
  → Web Configuration Interface
    - Matrix layout (width, height, panel arrangement)
    - Color calibration (per-panel gamma, white point)
    - Network settings (IP, Art-Net universe, sACN priority)
    - Firmware update (FPGA bitstream + STM32 firmware)
    - Diagnostic monitoring (temperature, frame rate, packet stats)
```

---

## 5. Bus Topology & Interconnects

### 5.1 FPGA ↔ STM32H7 Communication

| Interface   | Pins | Speed      | Purpose                              |
|-------------|------|------------|--------------------------------------|
| SPI (MOSI)  | 4    | 50 MHz     | Pixel data streaming (STM32→FPGA)    |
| SPI (MISO)  | 4    | 50 MHz     | Status/telemetry (FPGA→STM32)        |
| UART        | 2    | 3 Mbps     | Command/control channel              |
| GPIO        | 4    | —          | Interrupts, ready flags, reset       |

### 5.2 FPGA ↔ DDR3L Memory

| Interface   | Width | Speed         | Purpose                          |
|-------------|-------|---------------|----------------------------------|
| DDR3L Ch A  | 16-bit| 1066 MT/s     | Frame Buffer 0 (512 MB)          |
| DDR3L Ch B  | 16-bit| 1066 MT/s     | Frame Buffer 1 (512 MB)          |
| Total       | 32-bit| 1066 MT/s     | 1 GB, ~34 Gbps aggregate         |

### 5.3 FPGA ↔ HUB75E Output

| Signal Group | Pins per Port | Total Pins (16 ports) | Direction |
|--------------|---------------|-----------------------|-----------|
| RGB Data     | 6 (R1,G1,B1,R2,G2,B2) | 96            | FPGA→Panel |
| Address      | 5 (A,B,C,D,E) | 80                    | FPGA→Panel |
| Control      | 3 (CLK,LAT,OE)| 48                    | FPGA→Panel |
| **Total**    | **14**        | **224**               |           |

### 5.4 STM32H7 Peripheral Map

| Peripheral  | Pins | Connected To         | Purpose                       |
|-------------|------|----------------------|-------------------------------|
| ETH (RMII)  | 9    | KSZ9031RNX           | Gigabit Ethernet              |
| USB OTG HS  | 12   | USB3320C (ULPI)      | USB 2.0 Configuration          |
| SDMMC1      | 6    | microSD Socket       | SD Card I/O                   |
| SPI2        | 4    | FPGA (Pixel Data)    | High-speed pixel streaming    |
| SPI4        | 4    | FPGA (Status)        | Telemetry readback            |
| USART1      | 2    | FPGA (Command)       | Control channel               |
| SPI1        | 4    | FPGA Config Flash    | FPGA bitstream update         |
| I2C2        | 2    | TMP117 × 4           | Temperature sensors           |
| I2C3        | 2    | Si5351A              | Clock generator config        |
| I2C4        | 2    | TPS65218             | PMIC telemetry                |
| ADC1        | 2    | Current sense        | Power monitoring              |
| TIM1        | 2    | PWM fan control      | 2× 4-pin PWM fans             |
| GPIO        | 8    | Status LEDs, buttons | User interface                |

---

## 6. Clock Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        CLOCK TREE                                │
│                                                                  │
│  Si5351A (I2C-configurable, 25 MHz XTAL reference)              │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │ CLK0: 25.000 MHz  → STM32H7 HSE (Ethernet PTP accuracy)  │   │
│  │ CLK1: 50.000 MHz  → FPGA System Clock (GCLK)             │   │
│  │ CLK2: 27.000 MHz  → ADV7611 HDMI Rx (TMDS pixel clock)   │   │
│  │ CLK3: 25.000 MHz  → KSZ9031RNX (optional, PHY has own)   │   │
│  │ CLK4: 60.000 MHz  → USB3320C ULPI Reference              │   │
│  │ CLK5: 33.333 MHz  → FPGA Pixel Clock (HUB75E timing)     │   │
│  │ CLK6: 1066.667 MHz→ FPGA DDR3 Reference (via MMCM)       │   │
│  │ CLK7: 24.000 MHz  → ADV7611 Audio MCLK (optional)        │   │
│  └──────────────────────────────────────────────────────────┘   │
│                                                                  │
│  FPGA Internal MMCM/PLL:                                        │
│  - DDR3 UI clock: 533.33 MHz (from CLK6)                        │
│  - HUB75E GCLK: 33.333 MHz (from CLK5)                          │
│  - Pixel Proc Clock: 150 MHz (from CLK1 × PLL)                  │
│  - AXI Interconnect: 100 MHz (from CLK1 × PLL)                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 7. Power Architecture

```
12V DC Input (Barrel Jack, 5.5×2.1mm, center-positive)
  │
  ├── 5.0V @ 8A ──── TPS65218 DCDC1 ────┬── HUB75E Level Shifters (VCCA=3.3V, VCCB=5.0V)
  │                                      ├── HUB75E Panel Power Passthrough (optional)
  │                                      └── USB VBUS (500mA limited)
  │
  ├── 3.3V @ 3A ──── TPS65218 DCDC2 ────┬── FPGA I/O Banks (VCCO)
  │                                      ├── STM32H7 VDD
  │                                      ├── KSZ9031RNX DVDDH
  │                                      ├── USB3320C VDDIO
  │                                      ├── ADV7611 DVDDIO
  │                                      ├── Si5351A VDD
  │                                      ├── SPI Flash
  │                                      └── SD Card
  │
  ├── 1.8V @ 2A ──── TPS65218 DCDC3 ────┬── FPGA Aux (VCCAUX)
  │                                      ├── DDR3L VDD (SSTL-135 ref)
  │                                      ├── ADV7611 DVDD
  │                                      └── KSZ9031RNX DVDDL
  │
  ├── 1.35V @ 3A ─── TPS65218 DCDC4 ────┬── DDR3L VDDQ (both chips)
  │                                      └── FPGA VCCO_DDR (DDR3 I/O banks)
  │
  ├── 1.0V @ 6A ──── TPS65218 LDO1 ─────── FPGA VCCINT (Core)
  │
  ├── 1.2V @ 1A ──── TPS65218 LDO2 ─────── FPGA VCCBRAM
  │
  └── 3.3V @ 0.5A ── TPS65218 LDO3 ─────── FPGA MGTAVCC (unused, for future GTP)
```

---

## 8. Mechanical Constraints

| Parameter          | Value                          |
|--------------------|--------------------------------|
| Board Form Factor  | 170 mm × 170 mm (Mini-ITX)     |
| PCB Thickness      | 1.6 mm (standard)              |
| Mounting Holes     | 4× M3 at corners (Mini-ITX pattern) |
| Connector Edge     | All I/O on one edge (rear panel) |
| HUB75E Connectors  | 2 rows of 8 along top edge     |
| Cooling            | 2× 40mm PWM fans (side mount)  |
| Enclosure          | Optional 1U rack-mount or standalone |

---

## 9. Software Architecture Overview

### 9.1 FPGA Bitstream (Verilog/VHDL)
- **Frame Buffer Controller**: AXI4 Memory-Mapped to DDR3 MIG, 256-bit data width
- **Video Input Capture**: HDMI parallel RGB capture with timing detection
- **Scaler Engine**: Bilinear scaling with programmable crop region
- **Pixel Processing Pipeline**: Full pipeline as described in Section 4.2
- **HUB75E Waveform Generator**: 16-port, 32-channel BCM PWM controller
- **SPI Slave**: 50 MHz, 32-bit words, DMA to pixel FIFO
- **Command Interface**: UART-based register read/write

### 9.2 STM32H7 Firmware (C, FreeRTOS)
- **LWIP Stack**: UDP reception, Art-Net 4 / sACN E1.31 parsing
- **TinyUSB Stack**: RNDIS device for web configuration
- **FatFS**: SD card filesystem with raw frame playback
- **Mongoose HTTP Server**: Web configuration interface
- **System Monitor**: Temperature, voltage, fan speed PID control
- **FPGA Manager**: Bitstream loading via SPI, status polling

### 9.3 Companion App (React Native)
- **Matrix Configuration**: Layout editor, panel mapping
- **Live Monitor**: Frame rate, temperature, packet statistics
- **Color Calibration**: Per-panel gamma and white balance
- **Firmware Update**: OTA update for both FPGA and STM32
- **Protocol**: Binary wire protocol over UDP with CRC-16

---

## 10. Design Philosophy & Constraints

1. **Single-Board Integration**: No separate sending/receiving cards. Everything on one PCB.
2. **Industrial Temperature Range**: All components rated -40°C to +85°C where possible.
3. **Field-Upgradeable**: Both FPGA bitstream and MCU firmware updatable via USB or Ethernet.
4. **Open Protocol**: Custom binary protocol documented for third-party controller integration.
5. **Safety**: Polyfuse on input, ESD protection on all external connectors, TVS on HUB75E outputs.
6. **Testability**: JTAG header for FPGA, SWD header for STM32, test points on all power rails.
7. **Manufacturability**: 6-layer PCB, all components on top side, no BGA on bottom.

---

## 11. Risk Analysis

| Risk                          | Probability | Impact | Mitigation                              |
|-------------------------------|-------------|--------|-----------------------------------------|
| DDR3 timing closure at 1066   | Medium      | High   | Length-matched traces, fly-by topology  |
| FPGA resource utilization     | Low         | Medium | XC7A100T has ample LUT/BRAM for design  |
| HUB75E signal integrity        | Medium      | Medium | Series termination, level shifters near FPGA |
| Thermal at 35W                | Low         | Low    | 2× 40mm fans, thermal vias under FPGA   |
| EMI compliance                | Medium      | Medium | 6-layer with solid GND planes, shielding |
| Supply chain (FPGA lead time)  | Medium      | High   | XC7A100T in FGG484 is widely available  |

---

## 12. Development Roadmap

| Phase   | Deliverable                                    | Timeline |
|---------|------------------------------------------------|----------|
| Phase 1 | Architecture & Specification (this document)   | Complete |
| Phase 2 | Component Selection & Schematics               | Next     |
| Phase 3 | PCB Layout & Stackup                           | Next     |
| Phase 4 | Software Stack (FPGA + MCU + App)              | Next     |
| Phase 5 | Prototype Assembly & Bring-Up                  | TBD      |
| Phase 6 | Validation & Certification                     | TBD      |

---

*Document Version: 1.0 | Author: Systems Architect | Date: 2026-06-16*
*Target Device: Nebula Matrix — Professional LED Matrix Display Engine*
