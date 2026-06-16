# PhotonWeave — Phase 1: Conceptual Architecture

## 1. System Identity & Purpose

**PhotonWeave** is a real-time Computer-Generated Holography (CGH) accelerator engine. It ingests 3D depth-map video streams from multi-camera arrays or depth sensors, computes complex-amplitude holograms via FPGA-accelerated Fresnel diffraction transforms, and drives a 4K (3840×2160) phase-only Spatial Light Modulator (SLM) at 60 frames per second. The system enables true holographic display without offline precomputation — every frame is generated from live depth data in under 16.67 ms.

### Target Applications
- **Volumetric Telepresence**: Real-time 3D video conferencing with holographic display
- **Medical Visualization**: Live holographic rendering of CT/MRI volumetric data during surgery
- **Automotive HUD**: Next-generation holographic heads-up displays with true depth
- **Digital Signage**: Glasses-free 3D advertising displays
- **Scientific Visualization**: Molecular dynamics, fluid simulation, astronomical data
- **Art & Entertainment**: Live holographic performances and installations

## 2. Performance Targets

| Parameter | Target | Notes |
|-----------|--------|-------|
| Input Resolution | 2048×2048 depth map | Per color channel, 16-bit depth |
| Output Resolution | 3840×2160 (4K UHD) | SLM native resolution |
| Frame Rate | 60 fps sustained | 16.67 ms per frame budget |
| Hologram Computation | <12 ms per frame | Leaves 4.67 ms for I/O and SLM update |
| Depth Layers | 256 planes | Multi-plane Fresnel propagation |
| Color Channels | 3 (RGB) time-sequential | 180 Hz total SLM refresh |
| FFT Size | 4096×4096 complex | Zero-padded from 2048×2048 |
| FFT Throughput | 3× (4096² log2 4096) ops/frame | ~1.5 billion complex ops per frame |
| PCIe Bandwidth | 64 Gbps (Gen3 x8) | Depth map ingest |
| Memory Bandwidth | 76.8 GB/s (4× DDR4-2400) | Frame buffer read/write |
| SLM Interface | HDMI 2.0, 18 Gbps | 4K60 8-bit grayscale |
| Power Budget | <75W (PCIe slot + aux) | Passive cooling capable |
| Form Factor | Full-height, half-length PCIe | FHHL, single slot |

## 3. High-Level Block Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        PHOTONWEAVE CGH ACCELERATOR                       │
│                                                                          │
│  ┌──────────┐   PCIe Gen3 x8   ┌──────────────────────────────────┐      │
│  │ HOST PC  │◄═══════════════►│         XILINX KINTEX-7           │      │
│  │ (Depth   │  64 Gbps         │       XC7K325T-2FFG900C          │      │
│  │  Source) │                  │                                  │      │
│  └──────────┘                  │  ┌────────────────────────────┐  │      │
│                                │  │  CGH COMPUTE PIPELINE      │  │      │
│  ┌──────────┐   QSFP+ 40G     │  │                            │  │      │
│  │ CAMERA   │◄══════════════►│  │  Depth → Phase Map          │  │      │
│  │ ARRAY    │  Direct ingest  │  │  ↓                         │  │      │
│  └──────────┘                  │  │  Fresnel Propagator        │  │      │
│                                │  │  ↓                         │  │      │
│                                │  │  4096² Complex FFT (×256)  │  │      │
│                                │  │  ↓                         │  │      │
│                                │  │  Accumulate & Normalize    │  │      │
│                                │  │  ↓                         │  │      │
│                                │  │  Phase Quantize (8-bit)    │  │      │
│                                │  └────────────────────────────┘  │      │
│                                │                                  │      │
│  ┌────────────────┐           │  ┌────────────────────────────┐  │      │
│  │ DDR4-2400 ×4   │◄════════►│  │  MEMORY CONTROLLER (MIG)   │  │      │
│  │ 4Gb each (16Gb)│  76.8 GB/s│  │  4× 16-bit interfaces      │  │      │
│  └────────────────┘           │  └────────────────────────────┘  │      │
│                                │                                  │      │
│  ┌────────────────┐           │  ┌────────────────────────────┐  │      │
│  │ Si5345 CLOCK   │──────────►│  │  CLOCKING & RESET          │  │      │
│  │ GENERATOR      │  Multi-ref │  │  200 MHz (FFT core)        │  │      │
│  └────────────────┘           │  │  125 MHz (PCIe)            │  │      │
│                                │  │  300 MHz (DDR4)            │  │      │
│  ┌────────────────┐           │  │  148.5 MHz (HDMI pixel)    │  │      │
│  │ STM32H743VI    │──I2C/SPI─►│  └────────────────────────────┘  │      │
│  │ SYSTEM CTRL    │           │                                  │      │
│  │ Power seq, USB │           │  ┌────────────────────────────┐  │      │
│  │ config, temp   │           │  │  HDMI 2.0 TX (GTP XCVR)   │  │      │
│  └────────────────┘           │  │  4K60 8-bit grayscale      │  │      │
│                                │  │  → SLM Panel               │  │      │
│  ┌────────────────┐           │  └────────────────────────────┘  │      │
│  │ TPS65218 PMIC  │──────────►│                                  │      │
│  │ Multi-rail PWR │           │  ┌────────────────────────────┐  │      │
│  └────────────────┘           │  │  FT601 USB 3.0 FIFO        │  │      │
│                                │  │  Config/Debug Interface    │  │      │
│                                │  └────────────────────────────┘  │      │
│                                └──────────────────────────────────┘      │
│                                                                          │
│  ┌──────────────────────────────────────────────────────────────────┐    │
│  │                    POWER DISTRIBUTION                             │    │
│  │  PCIe 12V → TPS65218 → VCCINT (1.0V @ 25A), VCCAUX (1.8V @ 3A) │    │
│  │                       → VCCBRAM (1.0V @ 2A), VCCO (3.3V @ 5A)   │    │
│  │                       → MGTAVCC (1.0V @ 8A), MGTAVTT (1.2V @4A)│    │
│  │                       → VDD_DDR (1.2V @ 12A), VTT_DDR (0.6V @6A)│    │
│  │                       → 3.3V_STM32 (3.3V @ 0.5A)                │    │
│  └──────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────┘
```

## 4. Data Flow Architecture

### Frame Processing Pipeline (per 16.67 ms frame)

```
T=0.00 ms:  PCIe DMA ingest depth map (2048×2048×16-bit × 3 channels)
            → DDR4 Frame Buffer A (write)
            QSFP+ alternate path: direct camera array ingest

T=1.50 ms:  Depth Map → Complex Amplitude conversion
            For each pixel (x,y) at depth z:
              amplitude = depth_value / 65535.0
              phase = 2π × z / λ  (λ = 532 nm green reference)
            → DDR4 Working Buffer

T=2.00 ms:  Begin Fresnel Propagation Loop (256 depth planes)
            For each plane k (0..255):
              z_k = z_min + k × Δz
              Multiply by Fresnel kernel: H(fx,fy) = exp(j·2π·z_k/λ · sqrt(1-(λ·fx)²-(λ·fy)²))
              Execute 4096×4096 complex 2D FFT
              Accumulate result into Hologram Buffer
            → DDR4 Hologram Buffer (double-buffered)

T=11.00 ms: Fresnel loop complete (256 planes × ~35 µs each)
            Normalize hologram: find max amplitude, scale to [0,255]
            Phase quantize: arg(hologram) → 8-bit grayscale
            → DDR4 Output Buffer

T=12.00 ms: HDMI TX: read Output Buffer, format HDMI 2.0 stream
            TMDS encoding, 4K60 timing generation
            → SLM Panel (physical display)

T=16.67 ms: VSYNC — next frame begins
```

### Memory Map (DDR4 16Gb total)

| Region | Size | Purpose |
|--------|------|---------|
| Frame Buffer A | 256 MB | Incoming depth map (double-buffer A) |
| Frame Buffer B | 256 MB | Incoming depth map (double-buffer B) |
| Working Buffer | 512 MB | Complex amplitude arrays (float32) |
| FFT Scratch | 1024 MB | FFT twiddle factors, bit-reverse tables |
| Hologram Buffer 0 | 256 MB | Accumulated hologram (double-buffer 0) |
| Hologram Buffer 1 | 256 MB | Accumulated hologram (double-buffer 1) |
| Output Buffer | 64 MB | Phase-quantized 8-bit frame for HDMI |
| Descriptor Tables | 16 MB | DMA descriptor rings, command queues |
| Reserved | 1.5 GB | Future expansion |

## 5. Bus Topology & Interconnects

### Primary Data Buses

```
PCIe Gen3 x8 (8 lanes @ 8 GT/s)
  ├── Endpoint: Xilinx PCIe Gen3 Integrated Block (7 Series)
  ├── Max Payload: 256 bytes
  ├── Max Read Request: 512 bytes
  ├── DMA Engine: AXI Memory-Mapped to AXI4-Stream Bridge
  └── BAR0: 64 KB (control registers), BAR1: 16 MB (DMA aperture)

AXI4-Stream Interconnect (FPGA Internal, 256-bit @ 200 MHz = 51.2 Gbps)
  ├── PCIe DMA → Memory Controller (write channel)
  ├── Memory Controller → FFT Engine (read channel)
  ├── FFT Engine → Memory Controller (write channel)
  ├── Memory Controller → HDMI TX (read channel)
  └── QSFP+ Ingest → Memory Controller (write channel)

DDR4 Memory Bus (4 independent 16-bit interfaces)
  ├── Interface 0: Frame Buffers A/B (lower 2Gb)
  ├── Interface 1: Working Buffer + FFT Scratch (lower 2Gb)
  ├── Interface 2: Hologram Buffers + Output (lower 2Gb)
  └── Interface 3: Descriptor Tables + Reserved (lower 2Gb)

I2C Control Bus (100 kHz Standard-mode)
  ├── STM32H743 (Master) → Si5345 Clock Gen (Addr 0x68)
  ├── STM32H743 (Master) → TPS65218 PMIC (Addr 0x24)
  ├── STM32H743 (Master) → FPGA (Addr 0x30, GPIO expander emulation)
  ├── STM32H743 (Master) → Temperature Sensors (Addr 0x48, 0x49, 0x4A, 0x4B)
  └── STM32H743 (Master) → EEPROM (Addr 0x50, board info)

SPI Control Bus (50 MHz, Quad-SPI mode)
  ├── STM32H743 (Master) → FPGA Configuration Flash (MT25QU512ABB)
  └── STM32H743 (Master) → FT601 Configuration EEPROM

USB 3.0 SuperSpeed (FT601 FIFO Bridge)
  ├── Host PC → FT601 → FPGA 32-bit parallel FIFO
  └── Used for: firmware updates, debug telemetry, register access
```

## 6. Clock Architecture

```
Si5345A (Any-Frequency Clock Generator)
  │
  ├── OUT0: 200.000 MHz → FPGA FFT Core Clock (GCLK0)
  ├── OUT1: 125.000 MHz → FPGA PCIe User Clock (GCLK1)
  ├── OUT2: 300.000 MHz → FPGA DDR4 Reference (GCLK2)
  ├── OUT3: 148.500 MHz → FPGA HDMI Pixel Clock (GCLK3)
  ├── OUT4: 100.000 MHz → FPGA AXI Interconnect (GCLK4)
  ├── OUT5:  25.000 MHz → STM32H743 HSE (bypass)
  ├── OUT6:  50.000 MHz → FPGA Config Clock (EMCCLK)
  ├── OUT7:  60.000 MHz → FT601 CLK (via FPGA PLL)
  └── OUT8: 156.250 MHz → QSFP+ Reference (GCLK5)
  └── OUT9:  26.000 MHz → HDMI TMDS Reference

Reference Input: 25.000 MHz TCXO (SiT5356, ±0.1 ppm stability)
```

## 7. FFT Engine Architecture

The core compute engine is a streaming 4096×4096 complex 2D FFT implemented as:

```
Row FFT (4096-point, 1D):
  ┌─────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
  │ DDR4 RD │──►│ Bit-Reverse│──►│ Radix-4  │──►│ Twiddle  │
  │ (burst) │   │ Reorder   │   │ Butterfly│   │ Multiply │
  └─────────┘   └──────────┘   └──────────┘   └──────────┘
                                                     │
  ┌─────────┐   ┌──────────┐   ┌──────────┐   ┌────▼─────┐
  │ DDR4 WR │◄──│ Normalize │◄──│ Radix-4  │◄──│ Transpose│
  │ (burst) │   │ (1/4096)  │   │ Butterfly│   │ Buffer   │
  └─────────┘   └──────────┘   └──────────┘   └──────────┘

Pipeline depth: 12 stages (log4 4096 = 6, ×2 for row+col)
Throughput: 1 sample/clock @ 200 MHz = 200 Msps complex
4096² = 16.78M samples → 83.9 ms for full 2D FFT (single engine)
→ Need 8 parallel FFT engines to achieve <12 ms total
→ 8 engines × 200 Msps = 1.6 Gsps → 16.78M / 1.6G = 10.5 ms for 2D FFT
→ 256 planes × 10.5 ms = 2688 ms (too slow!)

OPTIMIZATION: Use Angular Spectrum Method (ASM) instead of per-plane FFT
  - Single 2D FFT of source field
  - Multiply by transfer function H(fx,fy,z) for each plane
  - Single 2D IFFT to reconstruct
  - 256 planes × transfer function multiply (O(N²) but no FFT per plane)
  - Total: 2 FFTs + 256 × O(N²) multiply-accumulate
  - 2 × 10.5 ms + 256 × 0.14 ms = 21 + 35.8 = 56.8 ms (still borderline)

FINAL ARCHITECTURE: Hybrid ASM with 16 parallel FFT engines
  - 2 FFTs @ 16 engines: 2 × (16.78M / 3.2G) = 2 × 5.25 ms = 10.5 ms
  - 256 transfer multiplies @ 16 parallel MAC arrays: 256 × 0.009 ms = 2.3 ms
  - Total compute: ~12.8 ms ✓ (within 16.67 ms budget)

## 8. FPGA Resource Budget (XC7K325T)

| Resource | Available | Used | Utilization |
|----------|-----------|------|-------------|
| Logic Cells | 326,080 | ~245,000 | 75% |
| DSP48E1 Slices | 840 | 720 | 86% |
| Block RAM (36Kb) | 445 | 380 | 85% |
| GTX Transceivers | 16 | 12 | 75% |
| PCIe Gen3 Block | 1 | 1 | 100% |
| MMCM/PLL | 10 | 8 | 80% |
| I/O Pins | 500 | 380 | 76% |

## 9. Thermal Design

- **Total Power**: ~65W (FPGA: 45W, DDR4: 8W, STM32: 0.5W, PMIC losses: 5W, other: 6.5W)
- **Cooling Strategy**: Passive heatsink with PCIe slot airflow
- **Heatsink**: Aluminum finned, 80×120×35mm, thermal resistance <0.4°C/W
- **Thermal Interface**: Honeywell PCM45F phase-change pad
- **Temperature Monitoring**: 4× TMP117 sensors (FPGA die edge, DDR4, PMIC, PCIe edge)
- **Thermal Throttling**: FPGA dynamic frequency scaling at 85°C junction

## 10. Host Software Interface

### Driver Model
- Linux kernel PCIe driver (pci_driver)
- Character device: /dev/photonweave0
- IOCTL interface for:
  - PHOTONWEAVE_IOC_SET_DEPTH_MAP: Submit depth frame
  - PHOTONWEAVE_IOC_GET_HOLOGRAM: Retrieve computed hologram
  - PHOTONWEAVE_IOC_SET_PARAMS: Configure wavelength, depth range, planes
  - PHOTONWEAVE_IOC_GET_STATUS: Temperature, FPS, error flags
  - PHOTONWEAVE_IOC_STREAM_START: Begin continuous streaming mode
  - PHOTONWEAVE_IOC_STREAM_STOP: Stop streaming

### Userspace SDK
- libphotonweave.so: C library wrapping IOCTLs
- Python bindings via ctypes
- OpenCV integration for depth map preprocessing
- ROS2 node for robotics integration

## 11. Safety & Reliability

- **Watchdog**: STM32H743 independent watchdog (IWDG), 2-second timeout
- **Power Sequencing**: TPS65218 enforces VCCINT → VCCAUX → VCCO order
- **Overcurrent Protection**: Per-rail current sensing with TPS65218
- **ESD Protection**: TVS diodes on all external connectors (PCIe, HDMI, QSFP+, USB)
- **Brownout Detection**: STM32H743 BOR at 2.7V
- **CRC Protection**: All DMA transfers CRC32-checked
- **ECC Memory**: DDR4 with SECDED ECC (single-error correct, double-error detect)

## 12. Compliance Targets

- **PCIe Card Electromechanical**: PCIe CEM 3.0, FHHL form factor
- **EMI/EMC**: FCC Part 15 Class A, EN 55032 Class A
- **Safety**: IEC 62368-1 (AV/IT equipment)
- **RoHS**: EU Directive 2011/65/EU (lead-free)
- **HDMI**: HDMI 2.0 compliance (TMDS signaling)
- **USB**: USB 3.0 SuperSpeed compliance (FT601 certified)

## 13. Development Roadmap

| Phase | Milestone | Duration |
|-------|-----------|----------|
| RTL Design | FFT engine, DMA, memory controller | 16 weeks |
| PCB Design | Schematic, layout, fab | 8 weeks |
| Board Bringup | Power, clocks, JTAG | 4 weeks |
| Firmware | STM32 system controller | 4 weeks |
| Linux Driver | PCIe driver + IOCTL | 6 weeks |
| SDK | libphotonweave + Python | 4 weeks |
| App | React Native companion | 4 weeks |
| Integration | End-to-end hologram display | 4 weeks |
| **Total** | | **50 weeks** |
