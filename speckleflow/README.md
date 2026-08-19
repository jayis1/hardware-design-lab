# SpeckleFlow — Portable Laser Speckle Contrast Imaging (LSCI) Blood-Flow Imager

![SpeckleFlow](https://img.shields.io/badge/PCB-120×80mm-blue) ![MCU](https://img.shields.io/badge/MCU-STM32H733-orange) ![FPGA](https://img.shields.io/badge/FPGA-iCE40UP5K-green) ![Camera](https://img.shields.io/badge/Camera-OV9281%20120fps%20global%20shutter-purple) ![Laser](https://img.shields.io/badge/Laser-785nm%20VCSEL-red) ![Wireless](https://img.shields.io/badge/Comms-BLE%205.2%20%2B%20USB--C-teal) ![Author](https://img.shields.io/badge/Author-jayis1-orange) ![License](https://img.shields.io/badge/License-CERN--OHL--S%20%2F%20GPL%20%2F%20MIT-yellow)

**Author: jayis1**
**Copyright © 2026 jayis1. All rights reserved.**
**License: CERN-OHL-S v2 (hardware), GPL-2.0 (firmware), MIT (app)**

> A handheld, battery-powered imager that visualizes tissue blood flow in real time
> using Laser Speckle Contrast Imaging (LSCI) — the same technology used in
> neurosurgical cortex perfusion monitors and burn-depth assessment systems, but
> open, portable, and affordable.

---

## 1. Purpose & Overview

SpeckleFlow is a compact, open-hardware medical/research instrument that performs
real-time, non-invasive visualization of blood flow in superficial tissue. It uses
the principle of **Laser Speckle Contrast Imaging (LSCI)**: a coherent near-infrared
laser illuminates the tissue surface, producing a random interference pattern (speckle).
Moving red blood cells blur the speckle pattern on the camera sensor's exposure
timescale, reducing the local speckle contrast. By computing the local contrast
(standard deviation / mean) over a sliding spatial window across every frame,
SpeckleFlow generates a live, color-coded 2D map of relative blood flow — a
**perfusion image** at video frame rates, without any injected contrast agent,
without ionizing radiation, and without contact.

### Why this device?

LSCI is a proven clinical modality used in:

- **Intraoperative neurosurgery** — cortical perfusion mapping during tumor resection,
  aneurysm clipping, and bypass surgery to identify ischemic regions and verify
  revascularization.
- **Burn-depth assessment** — distinguishing superficial (healing) from deep
  (requiring graft) burns within hours of injury, reducing unnecessary surgery.
- **Flap surgery monitoring** — real-time perfusion assessment of free tissue
  transfers (breast reconstruction, limb replantation) to detect vascular
  compromise before irreversible necrosis.
- **Dermatology & wound care** — mapping microcirculation in diabetic ulcers,
  pressure sores, and peripheral arterial disease to guide debridement and
  track healing.
- **Pharmacology research** — measuring drug-induced vasomotor responses,
  cortical spreading depression, and microvascular reactivity in animal models.
- **Sports medicine** — assessing tissue perfusion recovery after injury or
  during rehabilitation.

Despite its utility, commercial LSCI systems (e.g., Moor FLPI, PeriCam PSI)
cost $30,000–$80,000 and are cart-mounted instruments restricted to hospital
and research-lab settings. SpeckleFlow brings this capability into a
handheld, battery-powered, sub-$300 BOM open device — making real-time
perfusion imaging accessible to resource-limited clinics, field medics,
veterinary practitioners, researchers, and educators.

### What makes it novel

No device in this repository or on the open-hardware market combines:

1. **FPGA-accelerated real-time speckle contrast pipeline** — a pipelined
   7×7 sliding-window contrast computation in a Lattice iCE40UP5K delivers
   60 fps perfusion imaging at 640×480 resolution with zero host-CPU load.
2. **Single-mode 785 nm VCSEL illumination** — narrow-linewidth, low-speckle
   decorrelation noise, thermally stabilized with a TEC for wavelength and
   coherence stability across ambient temperature swings.
3. **Global-shutter CMOS sensor** — the OmniVision OV9281 captures
   distortion-free speckle at up to 120 fps, critical for accurate contrast
   measurement (rolling shutter would alias the speckle pattern).
4. **On-device display + BLE streaming** — a 2.8" TFT shows the live
   perfusion map with adjustable colormap, while BLE 5.2 streams quantized
   flow data to a companion app for recording, ROI analysis, and reporting.
5. **Open, end-to-end design** — full schematics, PCB layout, firmware,
   and app source, all under permissive open-source licenses.

---

## 2. System Specifications

| Parameter | Value |
|---|---|
| **Imaging Modality** | Laser Speckle Contrast Imaging (LSCI) |
| **Laser Source** | 785 nm single-mode VCSEL, 30 mW CW, Class 3R |
| **Camera** | OmniVision OV9281 — 1 MP (1280×800) global shutter, up to 120 fps |
| **Processing Resolution** | 640×480 @ 60 fps (hardware-scaled) |
| **Contrast Window** | 7×7 pixels (configurable 5×5 / 7×7 / 9×9) |
| **Flow Map Dynamic Range** | 8-bit (0–255), K ∈ [0, 1] mapped |
| **MCU** | STMicroelectronics STM32H733VIT6 — Cortex-M7 @ 480 MHz, 1 MB Flash, 1 MB SRAM |
| **FPGA** | Lattice iCE40UP5K-SG48I — 5280 LUTs, 128 KB BRAM, 8 DSP cores |
| **Display** | ILI9341 — 2.8" 320×240 TFT, SPI, 16-bit RGB565 |
| **Wireless** | Nordic nRF52840 — BLE 5.2, 2 Mbps PHY |
| **Storage** | microSD (UHS-I, SDIO 4-bit, up to 128 GB) |
| **USB** | USB-C (USB 2.0 Full-Speed, CDC + MSC composite) |
| **IMU** | TDK ICM-42688-P — 6-axis accel + gyro for image registration |
| **Laser Control** | DAC-driven constant current + TEC stabilization (±0.1 °C) |
| **Power** | 3.7 V 2000 mAh LiPo, USB-C charging (MCP73871), TPS63020 buck-boost |
| **Battery Life** | ~2.5 hours continuous imaging |
| **Form Factor** | 120 mm × 80 mm × 28 mm, handheld pistol-grip compatible |
| **Weight** | ~180 g (with battery) |
| **Operating Temp** | +10 °C to +40 °C (clinical) |
| **Regulatory Note** | Research/educational use only; not FDA-cleared for clinical diagnosis |

---

## 3. Architecture & Block Diagram

```
 ┌──────────────────────────────────────────────────────────────────┐
 │                        SPECKLEFLOW                                │
 │                                                                  │
 │   ┌─────────────┐     ┌──────────────┐     ┌─────────────────┐   │
 │   │  785 nm     │     │  OV9281      │     │  iCE40UP5K      │   │
 │   │  VCSEL      │────▶│  Global-Shut │────▶│  FPGA           │   │
 │   │  + TEC      │     │  1280×800    │ DVP │  Contrast       │   │
 │   │  + DAC drv  │     │  120 fps     │ 8b  │  Pipeline       │   │
 │   └──────┬──────┘     └──────┬───────┘     │  7×7 window     │   │
 │          │                   │  Sync/Trig  │  60 fps @640×480│   │
 │          │                   │             └────────┬────────┘   │
 │          │                   │                      │ DMA SPI     │
 │          ▼                   ▼                      ▼            │
 │   ┌──────────────────────────────────────────────────────────┐   │
 │   │              STM32H733VIT6  (Cortex-M7 @ 480 MHz)         │   │
 │   │                                                          │   │
 │   │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────────┐  │   │
 │   │  │ UI/RTOS │ │ Image   │ │ BLE     │ │ SD Log        │  │   │
 │   │  │ Task    │ │ Process │ │ UART    │ │ SDIO          │  │   │
 │   │  │         │ │ Colormap│ │ Bridge  │ │               │  │   │
 │   │  └─────────┘ └─────────┘ └─────────┘ └───────────────┘  │   │
 │   │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌───────────────┐  │   │
 │   │  │ Laser   │ │ IMU     │ │ USB-C   │ │ Power Mgmt    │  │   │
 │   │  │ Ctrl    │ │ I2C     │ │ CDC/MSC │ │ Battery Mon   │  │   │
 │   │  └─────────┘ └─────────┘ └─────────┘ └───────────────┘  │   │
 │   └──────────────────────┬──────────────────────────────────┘   │
 │                          │ SPI                                    │
 │   ┌──────────────────────▼──────────────────────────────────┐   │
 │   │  ILI9341 2.8" TFT 320×240 — Live Perfusion Map Display  │   │
 │   └─────────────────────────────────────────────────────────┘   │
 │                                                                  │
 │   ┌─────────────┐                  ┌─────────────────────────┐  │
 │   │ nRF52840    │ ◀── UART (3 Mbps)│ USB-C (CDC + MSC)        │  │
 │   │ BLE 5.2     │                  │ Charging + Data + DFU    │  │
 │   └─────────────┘                  └─────────────────────────┘  │
 │                                                                  │
 │   ┌─────────────┐     ┌─────────────┐     ┌─────────────────┐   │
 │   │ ICM-42688-P │     │ MCP73871    │     │ TPS63020        │   │
 │   │ IMU (I²C)   │     │ Charger     │     │ Buck-Boost 3.3V│   │
 │   └─────────────┘     └─────────────┘     └─────────────────┘   │
 └──────────────────────────────────────────────────────────────────┘
```

### Data flow

1. The **785 nm VCSEL** illuminates the tissue with expanded (~30 mm diameter)
   coherent light. The VCSEL is TEC-stabilized to maintain coherence length
   and wavelength across ambient temperature.
2. The **OV9281 global-shutter sensor** captures the back-scattered speckle
   pattern. Global shutter is essential — a rolling shutter would alias the
   random speckle pattern and corrupt the contrast measurement.
3. The **iCE40UP5K FPGA** ingests the raw pixel stream over an 8-bit parallel
   DVP interface. A pipelined image-scaling block downsamples 1280×800 →
   640×480. A **streaming contrast engine** computes a 7×7 sliding-window
   local contrast `K = σ / μ` for every pixel using an integral-image-free
   pipelined approach (running sums of pixel and pixel²). The resulting
   8-bit flow map is output over SPI to the STM32.
4. The **STM32H733** applies a colormap (jet, thermal, or grayscale),
   renders the perfusion image + HUD overlay (ROI box, flow value, battery,
   laser status) to the **ILI9341 TFT**, streams a downsampled flow map
   over **BLE** to the companion app, and optionally logs full-resolution
   frames to **microSD**.
5. The **nRF52840** handles BLE 5.2 protocol independently, receiving
   quantized flow-map tiles over a 3 Mbps UART link from the STM32.
6. The **ICM-42688-P IMU** logs device orientation for each frame, enabling
   image registration and stabilization in post-processing.

---

## 4. Hardware Specifications

### 4.1 MCU — STM32H733VIT6

| Feature | Value |
|---|---|
| Core | ARM Cortex-M7 @ 480 MHz, double-precision FPU |
| Flash | 1 MB (dual-bank, live OTA update) |
| SRAM | 1 MB (192 KB DTCM + 864 KB SRAM) |
| Cache | 16 KB I-cache, 16 KB D-cache |
| Peripherals | 3× SPI (up to 100 MHz), 4× I²C, 4× UART, SDMMC, USB OTG-HS, 2× ADC, 2× DAC, 17× TIM |
| Package | LQFP100 |
| Role | System controller, UI, colormap, BLE bridge, SD logging, USB |

### 4.2 FPGA — Lattice iCE40UP5K-SG48I

| Feature | Value |
|---|---|
| LUTs | 5280 |
| BRAM | 128 KB (30 × 4 KB blocks) |
| DSP | 8 × 16×16 multiply-accumulate |
| PLL | 1 |
| Package | SG48 (5×5 mm) |
| Configuration | SPI flash (W25Q128) |
| Role | Camera DVP capture, downscaler, speckle contrast pipeline, SPI output |

### 4.3 Camera — OmniVision OV9281

| Feature | Value |
|---|---|
| Resolution | 1280 × 800 (1 MP) |
| Shutter | Global (essential for LSCI) |
| Max Frame Rate | 120 fps (full res), 240 fps (VGA) |
| Pixel Size | 3.0 µm × 3.0 µm |
| Output | 8-bit parallel DVP (Y8 monochrome) |
| Sensitivity | 6800 e⁻/(lx·s) @ 785 nm (NIR-enhanced) |
| Interface | DVP 8-bit + sync + PCLK |
| Control | SCCB (I²C-compatible) via STM32 |

### 4.4 Laser — 785 nm VCSEL

| Feature | Value |
|---|---|
| Wavelength | 785 nm (near-IR, low melanin absorption) |
| Type | Single-mode VCSEL (Vixar V800-0000-X00 class) |
| Power | 30 mW CW (Class 3R) |
| Linewidth | < 0.1 nm (high coherence for strong speckle) |
| Beam | Circular, 5° divergence → 30 mm spot at 20 cm |
| Driver | Constant-current DAC (MCP4921) + soft-start |
| Stabilization | TEC (MP-3021T) + thermistor feedback loop, ±0.1 °C |
| Safety | Interlock, key switch, LED indicator, 5 s ramp-up per IEC 60825-1 |

### 4.5 Display — ILI9341

| Feature | Value |
|---|---|
| Size | 2.8" diagonal |
| Resolution | 320 × 240 (QVGA) |
| Interface | SPI (4-wire, up to 40 MHz) |
| Color | 16-bit RGB565 |
| Touch | Resistive (optional capacitive) |
| Role | Live perfusion map + HUD overlay |

### 4.6 Wireless — nRF52840

| Feature | Value |
|---|---|
| Core | ARM Cortex-M4F @ 64 MHz |
| Radio | BLE 5.2, 2 Mbps PHY, Long Range |
| Range | ~10 m line-of-sight |
| UART Bridge | 3 Mbps to STM32 |
| Role | BLE protocol stack, app pairing, flow-map streaming |

### 4.7 Power

| Component | Specification |
|---|---|
| Battery | 3.7 V 2000 mAh LiPo (ICR18650-22P or pouch) |
| Charger | MCP73871 — USB-C, 1 A charge, 500 mA / 1 A input current limit |
| Regulator | TPS63020 — buck-boost, 3.3 V / 2 A, >90% efficiency |
| Laser Supply | TPS7A4700 LDO — 3.3 V ultra-low-noise for VCSEL |
| TEC Driver | MAX1968 — bidirectional H-bridge, ±1.5 A |
| Fuel Gauge | MAX17048 — I²C, 1% accuracy |
| Battery Life | ~2.5 h continuous imaging, ~8 h standby |

### 4.8 Mechanical / Form Factor

| Parameter | Value |
|---|---|
| PCB | 120 mm × 80 mm, 6-layer, 1.6 mm FR4 |
| Enclosure | 3D-printed PETG, 120 × 80 × 28 mm |
| Lens | Fixed-focus 16 mm f/1.4 C-mount, working distance 15–25 cm |
| Grip | Pistol-grip with trigger (capture) + mode buttons |
| Weight | ~180 g with battery |
| Mount | 1/4"-20 tripod thread + DIN-rail clip |

---

## 5. Firmware Design

### 5.1 Architecture

The firmware runs on the STM32H733 using a cooperative, interrupt-driven
architecture (no RTOS overhead — the application is a single-threaded
super-loop with prioritized ISRs). The FPGA runs independently, producing
a steady stream of processed flow-map frames over SPI.

```
 ┌─────────────────────────────────────────────────────┐
 │                  STM32H733 Firmware                  │
 │                                                      │
 │  ┌────────────┐  ┌──────────────┐  ┌──────────────┐ │
 │  │  main()    │  │  DMA ISRs    │  │  Timer ISRs  │ │
 │  │  super-loop │  │  SPI/SDIO/  │  │  1 ms tick   │ │
 │  │            │  │  UART        │  │  debounce    │ │
 │  └─────┬──────┘  └──────┬───────┘  └──────┬───────┘ │
 │        │                │                  │         │
 │        ▼                ▼                  ▼         │
 │  ┌──────────────────────────────────────────────┐  │
 │  │            Frame Pipeline                      │  │
 │  │  SPI RX (FPGA) → Ring Buffer → Colormap →      │  │
 │  │  TFT DMA + BLE Tile Encoder + SD Logger        │  │
 │  └──────────────────────────────────────────────┘  │
 │                                                      │
 │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐│
 │  │ camera   │ │ laser    │ │ display  │ │ ble    ││
 │  │ driver   │ │ driver   │ │ driver   │ │ driver ││
 │  ├──────────┤ ├──────────┤ ├──────────┤ ├────────┤│
 │  │ fpga     │ │ sdcard   │ │ imu      │ │ power  ││
 │  │ driver   │ │ driver   │ │ driver   │ │ driver ││
 │  └──────────┘ └──────────┘ └──────────┘ └────────┘│
 └─────────────────────────────────────────────────────┘
```

### 5.2 Key design decisions

1. **STM32H733 over STM32H743** — the H733 has the same Cortex-M7 core
   and 1 MB SRAM but in a smaller LQFP100 package with dual-bank flash
   for live OTA updates. 480 MHz is sufficient for colormap + HUD overlay
   since the FPGA offloads the heavy contrast computation.

2. **iCE40UP5K FPGA for real-time contrast** — the 7×7 sliding-window
   contrast computation requires 49 multiplications and 49 additions per
   pixel per frame. At 640×480 × 60 fps = 18.4 Mpx/s, this is 900 million
   multiply-accumulates per second — well within the FPGA's 8 DSP cores
   using a pipelined systolic approach. The STM32 alone could not sustain
   this at 60 fps while also driving the display and BLE.

3. **Global-shutter OV9281** — rolling-shutter sensors (e.g., OV5640)
   would produce spatially-varying exposure times across the speckle
   pattern, corrupting the contrast measurement. The OV9281's global
   shutter ensures all pixels integrate simultaneously.

4. **785 nm VCSEL over laser diode** — a VCSEL (Vertical-Cavity
   Surface-Emitting Laser) produces a circular, low-divergence beam with
   narrow linewidth, yielding higher-contrast speckle than edge-emitting
   laser diodes. 785 nm is chosen over 633 nm (red) for deeper tissue
   penetration (melanin absorption is ~10× lower) and over 808 nm to
   reduce water absorption. The TEC stabilizes the junction temperature
   to ±0.1 °C, keeping the wavelength and coherence length constant.

5. **DVP parallel over MIPI-CSI** — the iCE40UP5K does not have
   hardened MIPI-CSI lanes. The OV9281's 8-bit DVP output at 120 fps
   is easily captured by the FPGA's GPIO. The FPGA then serializes the
   processed result over SPI to the STM32.

6. **No RTOS** — the application is a single super-loop with DMA-driven
   SPI/SDIO/UART transfers and prioritized interrupts. This eliminates
   RTOS scheduling jitter in the frame pipeline, which is critical for
   consistent frame timing in perfusion measurement.

7. **Dual-bank OTA** — the STM32H733's dual-bank flash allows firmware
   updates over USB-C without a bootloader, enabling field updates.

### 5.3 Speckle contrast algorithm

The FPGA computes the local speckle contrast:

```
K(x, y) = σ(x, y) / μ(x, y)
```

where `μ` is the mean and `σ` is the standard deviation of pixel intensity
over a 7×7 window centered at `(x, y)`. This is computed using running
sums:

```
S  = Σ I(x, y)          (sum of intensities)
S2 = Σ I(x, y)²          (sum of squared intensities)
μ  = S / N              (N = 49 for 7×7)
σ² = (S2 / N) - μ²       (variance)
K  = sqrt(σ²) / μ
```

The inverse `1/K` is proportional to the correlation time `τ_c` of the
speckle, which is inversely proportional to blood flow velocity. The
firmware maps `K` to a colormap: low K (high flow) → red/warm, high K
(low flow) → blue/cool.

The flow index is computed as:

```
Flow = (K_static - K_measured) / K_static × 255
```

where `K_static` is a calibration reference (diffuser plate or static
paper) measured at startup.

### 5.4 Laser safety

The firmware implements IEC 60825-1 Class 3R safety:

- **5-second ramp-up** — laser power increases from 0 to 30 mW over 5 s
  with a visible red LED indicator.
- **Interlock** — a hardware interlock cuts laser power if the enclosure
  is opened or the trigger is released.
- **Key switch** — a physical key enables laser operation.
- **Auto-shutoff** — laser disables after 30 s of no trigger activity.
- **Power limit** — DAC output is hardware-clamped to 30 mW via a
  current-sense resistor and comparator.

---

## 6. Application / Software Interface

### 6.1 Companion app (React Native)

The SpeckleFlow companion app runs on iOS and Android and provides:

- **Device connection** — BLE 5.2 scanning, pairing, and connection
  management.
- **Live perfusion view** — real-time 160×120 flow-map display with
  selectable colormaps (jet, thermal, grayscale, viridis).
- **ROI analysis** — draw rectangular or freehand regions of interest;
  the app computes mean flow, min/max, and time-series within the ROI.
- **Recording** — capture frame sequences to device storage; replay
  with timeline scrubber.
- **Export** — export frames as PNG, time-series as CSV, sessions as
  HDF5.
- **Settings** — contrast window size (5/7/9), exposure time, laser
  power, frame rate, colormap, BLE data rate.
- **Calibration** — guided static-reference calibration using the
  included diffuser plate.

### 6.2 BLE protocol

The BLE link uses a custom GATT service with three characteristics:

| Characteristic | UUID | Direction | Payload |
|---|---|---|---|
| Flow Map Tile | `0000FF01-...` | Device → App | 128-byte flow-map tile (16×8 pixels, 8-bit) |
| Status | `0000FF02-...` | Device → App | 8-byte status: battery, laser, fps, temp |
| Command | `0000FF03-...` | App → Device | 4-byte command: start/stop/calibrate/set-param |

The 640×480 flow map is divided into 240 tiles of 16×8 pixels (20×30
grid). Each tile is sent as a 128-byte notification. At 60 fps, the
app receives 14,400 tiles/s = 1.84 Mbps, within BLE 5.2's 2 Mbps PHY
capacity (the app also supports a reduced 160×120 @ 15 fps mode for
older phones).

### 6.3 USB-C interface

USB-C presents a composite CDC + MSC device:

- **CDC (virtual serial)** — text-based command interface for scripting
  and integration with lab software (MATLAB, Python, LabVIEW).
- **MSC (mass storage)** — the microSD card appears as a USB drive for
  direct file access without removing the card.

---

## 7. Use Cases & Target Audience

### 7.1 Clinical research

- **Neurosurgery research labs** — cortical perfusion studies in animal
  models (rodent cranial windows). Current commercial systems are too
  expensive for individual labs; SpeckleFlow enables multi-animal,
  multi-site studies at a fraction of the cost.
- **Burn units** — burn-depth assessment within 2–72 hours of injury.
  SpeckleFlow can distinguish superficial partial-thickness (healing
  in 2 weeks) from deep partial-thickness (requiring excision and graft)
  burns, reducing unnecessary surgery by up to 30%.
- **Reconstructive surgery** — free-flap monitoring in the first 72
  post-operative hours, when venous/arterial thrombosis is most common.
  SpeckleFlow provides continuous, non-contact perfusion monitoring
  vs. the current standard of periodic clinical observation.

### 7.2 Field & resource-limited settings

- **Disaster medicine** — triage of crush injuries and burns to assess
  tissue viability before evacuation. Battery-powered and handheld,
  SpeckleFlow works in field hospitals without infrastructure.
- **Veterinary medicine** — perfusion assessment in equine and
  companion-animal surgery, where commercial LSCI systems are rarely
  available due to cost.
- **Global health** — diabetic foot ulcer screening in low-resource
  clinics. Peripheral perfusion mapping identifies at-risk tissue before
  ulceration, enabling preventive care.

### 7.3 Education & outreach

- **Biomedical engineering education** — students learn LSCI principles,
  FPGA image processing, and embedded system design from fully open
  schematics, firmware, and app source.
- **Science communication** — museums and outreach events can demonstrate
  real-time blood-flow imaging (e.g., showing perfusion changes during
  cold pressor tests or cognitive tasks).

### 7.4 Industrial R&D

- **Cosmetics & skincare** — evaluating the effect of topical products
  on skin microcirculation.
- **Pharmaceuticals** — preclinical vasomotor drug screening.
- **Wearable sensor validation** — calibrating PPG/PPiG wearable
  perfusion sensors against a ground-truth LSCI reference.

---

## 8. Bill of Materials (Key Components)

| Ref | Part | Description | Qty | Est. Cost |
|---|---|---|---|---|
| U1 | STM32H733VIT6 | MCU, Cortex-M7 @ 480 MHz | 1 | $12.50 |
| U2 | iCE40UP5K-SG48I | FPGA, 5280 LUTs | 1 | $5.20 |
| U3 | OV9281 | Camera, 1 MP global shutter | 1 | $8.00 |
| U4 | nRF52840 | BLE 5.2 module | 1 | $4.80 |
| U5 | ILI9341 | 2.8" TFT display | 1 | $6.50 |
| U6 | Vixar V800 VCSEL | 785 nm, 30 mW | 1 | $22.00 |
| U7 | MAX1968 | TEC driver | 1 | $7.50 |
| U8 | MCP73871 | USB-C charger | 1 | $2.30 |
| U9 | TPS63020 | Buck-boost 3.3 V | 1 | $3.80 |
| U10 | ICM-42688-P | 6-axis IMU | 1 | $3.50 |
| U11 | MAX17048 | Battery fuel gauge | 1 | $2.10 |
| U12 | W25Q128 | SPI flash (FPGA config) | 1 | $1.20 |
| U13 | MCP4921 | 12-bit DAC (laser current) | 1 | $1.50 |
| D1 | TEC MP-3021T | Thermoelectric cooler | 1 | $4.00 |
| — | microSD socket | UHS-I, push-push | 1 | $0.80 |
| — | USB-C receptacle | 24-pin | 1 | $0.90 |
| — | LiPo 2000 mAh | 3.7 V pouch | 1 | $6.00 |
| — | PCB, 6-layer | 120 × 80 mm | 1 | $8.00 |
| — | Enclosure (3D-printed) | PETG | 1 | $3.00 |
| — | Lens, 16 mm f/1.4 | C-mount | 1 | $12.00 |
| | **Total (est.)** | | | **~$115** |

---

## 9. File Structure

```
speckleflow/
├── README.md                  ← This file
├── firmware/
│   ├── main.c                 ← System init, super-loop, UI, frame pipeline
│   ├── registers.h            ← STM32H7 register definitions
│   ├── board.h                ← Pin assignments, constants, config
│   ├── Makefile               ← ARM GCC build system
│   ├── linker.ld              ← Memory layout (dual-bank)
│   └── drivers/
│       ├── camera.c/.h        ← OV9281 SCCB config + trigger
│       ├── fpga.c/.h          ← iCE40 SPI config + contrast pipeline
│       ├── laser.c/.h         ← 785 nm VCSEL DAC + TEC PID
│       ├── display.c/.h       ← ILI9341 SPI + DMA framebuffer
│       ├── ble.c/.h           ← nRF52840 UART bridge + GATT
│       ├── sdcard.c/.h        ← SDIO 4-bit FAT32 logging
│       ├── imu.c/.h           ← ICM-42688-P 6-axis
│       └── power.c/.h         ← Battery monitor + power management
├── kicad/
│   ├── device.kicad_pro       ← KiCad project
│   ├── device.kicad_sch       ← Schematic (all components + netlist)
│   └── device.kicad_pcb       ← PCB layout (6-layer, 120×80 mm)
└── app/
    ├── App.js                 ← React Native entry + navigation
    ├── package.json           ← Dependencies
    ├── screens/
    │   ├── ConnectScreen.js   ← BLE scan + connect
    │   ├── LiveFlowScreen.js  ← Real-time perfusion map
    │   ├── RecordScreen.js    ← Session recording + replay
    │   ├── AnalysisScreen.js  ← ROI statistics + export
    │   └── SettingsScreen.js  ← Device configuration
    ├── components/
    │   ├── FlowMap.js         ← Canvas-based flow-map renderer
    │   └── DeviceList.js      ← BLE device list
    └── utils/
        └── protocol.js        ← BLE GATT protocol + tile decoder
```

---

## 10. License

| Component | License |
|---|---|
| Hardware design (KiCad) | CERN-OHL-S v2 |
| C firmware & drivers | GPL-2.0 |
| React Native companion app | MIT |

---

## 11. Disclaimer

SpeckleFlow is a research and educational instrument. It is **not**
FDA-cleared, CE-marked, or intended for clinical diagnosis. The 785 nm
Class 3R laser requires appropriate laser safety training and protective
eyewear. Use in clinical settings requires institutional review board
(IRB) approval and compliance with local medical device regulations.

---

**Author: jayis1** · **Copyright © 2026 jayis1** · **All rights reserved.**