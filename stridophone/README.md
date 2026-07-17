# Stridophone — Bioacoustic Insect Pest Detector & Classifier

**Author:** jayis1
**License:** MIT
**Version:** 1.0.0

> A field-deployable, solar-powered acoustic sentinel that listens for insect
> stridulation (crickets, cicadas, grasshoppers, beetles, ants, termites, and
> stored-product pests) and classifies species + estimates population density
> on-device using a fused microphone-array + vibration-channel DSP pipeline.

---

## 1. Purpose & Overview

Insect pests cause billions of dollars of damage annually to stored grain,
timber structures, vineyards, greenhouses, and homes. Conventional detection
relies on visual inspection or sticky traps — both lag indicators that detect
an infestation *after* it is established. Stridophone flips the timeline: it
continuously listens for the species-specific acoustic signatures insects
produce when they feed, mate, or communicate, and raises an alert the moment
a target species' call pattern emerges above the ambient noise floor.

Stridophone is **not** a generic sound recorder. It is a purpose-built
bioacoustic instrument that combines:

1. A **four-element MEMS microphone array** for spatial beamforming and
   direction-of-arrival estimation (so you know *where* the insects are, not
   just *that* they are).
2. A **high-resolution vibration channel** (ADXL355 3-axis accelerometer
   mounted to a coupling stud) that picks up substrate-borne vibrations from
   wood-boring beetle larvae and termites — the acoustic path inside timber
   is far cleaner than the air path.
3. An **on-board STM32H743 DSP core** running CMSIS-DSP that extracts
   mel-frequency cepstral coefficients (MFCCs), spectral crest, pulse-repetition
   rate, and inter-onset intervals, then feeds a compact quantized classifier
   that labels the call to species/genus level.
4. A **Wi-Fi 6 / BLE companion app** that shows a live spectrogram, a species
   roster with confidence scores, a population-density heatmap interpolated
   from the beamformer's DOA histogram, and push alerts.

The device is intended for unattended, months-long deployment in grain silos,
timber-frame warehouses, vineyards, museums, and heritage buildings. It is
weatherproof (IP65), solar-charged, and reports over Wi-Fi when available,
falling back to BLE beaconing when offline.

---

## 2. Hardware Specifications

| Subsystem        | Component                          | Notes |
|------------------|------------------------------------|-------|
| MCU              | STM32H743VIT6 (Cortex-M7F @ 480 MHz, 1 MB SRAM, 2 MB flash) | Main DSP + application core |
| Coprocessor      | ESP32-C6-MINI-1 (Wi-Fi 6 + BLE 5.3) | Connectivity only; talks to MCU over UART + handshake GPIO |
| Microphone array | 4× Knowles SPH0641LU4H-1 (I²S MEMS, 20 Hz–16 kHz, 65 dBA SNR) | Square array, 40 mm spacing → DOA up to ~4.3 kHz |
| Vibration channel| ADXL355 (3-axis, 20-bit, 1 mg/√Hz noise, ±2/4/8 g) on SPI | Coupled via M3 stud to substrate |
| Env sensor       | Sensirion SHT45 (±0.2 °C, ±1.8 %RH) | Insect activity is temperature-dependent; used as classifier feature |
| Power            | Panasonic NCR18650B (3.4 Ah) + MPPT solar charger (BQ25895) | USB-C PD fallback / test power |
| Solar panel      | 2 W monocrystalline, 6 V oc         | Trickle-charge in low-light |
| Storage          | MicroSD socket (SDMMC1, 4-bit)      | Raw-event WAV snippets + daily logs |
| UI               | 4 LEDs (status, activity, alert, charge) + 1 tactile button | Minimal; app is primary UI |
| Form factor      | 90 × 60 × 28 mm IP65 enclosure (PC + TPE overmold) | Mounting bracket + vibration coupler on base |
| Weight           | ~120 g (without battery)            | |

### Power budget (typical, 1 Hz classification cycle)

| State       | Current | Time share | Avg current |
|-------------|---------|------------|-------------|
| Active listen + DSP | 95 mA  | 40 % | 38 mA |
| Idle/standby        | 6 mA   | 55 % | 3.3 mA |
| Wi-Fi TX (per event)| 320 mA | 5 %  | 16 mA |
| **Total**           |        |      | **~57 mA** |

With a 3.4 Ah cell this yields ~60 h of no-sun operation; the 2 W panel
provides ~300 mA in good light, comfortably sustaining operation and
recovering the cell on a ~4 h sunny window per day.

---

## 3. Architecture & Block Diagram

```
        ┌──────────────────┐     I2S x4 (TDM)      ┌──────────────────────┐
  Mic 0 │ SPH0641LU4H-1    │──────────────────────▶│                      │
  Mic 1 │ SPH0641LU4H-1    │──────────────────────▶│   STM32H743VIT6      │
  Mic 2 │ SPH0641LU4H-1    │──────────────────────▶│   Cortex-M7F 480MHz  │
  Mic 3 │ SPH0641LU4H-1    │──────────────────────▶│   CMSIS-DSP engine   │
        └──────────────────┘                        │                      │
                                                   │  ┌────────────────┐  │
        ┌──────────────────┐     SPI @ 1 MHz       │  │ MFCC + DOA +   │  │
  Vib   │ ADXL355          │──────────────────────▶│  │ quantized CLF  │  │
        └──────────────────┘                        │  └────────────────┘  │
                                                   │                      │
        ┌──────────────────┐     I2C               │  Event queue / SDIO  │
  Env   │ SHT45            │──────────────────────▶│  MicroSD             │
        └──────────────────┘                        └─────┬───────┬────────┘
                                                          │ UART  │ USB-C
                                                          ▼       ▼
                                                   ┌──────────┐ ┌────────┐
                                                   │ ESP32-C6 │ │ ST-Link│
                                                   │ Wi-Fi6/  │ │ + CDC  │
                                                   │ BLE5.3   │ └────────┘
                                                   └────┬─────┘
                                                        │ Wi-Fi / BLE
                                                        ▼
                                              ┌──────────────────┐
                                              │ React Native app │
                                              │  Stridophone     │
                                              └──────────────────┘

  Power: BQ25895 MPPT ── 18650 ── 3.3 V LDO + 5 V buck-boost
```

### Signal flow

1. The four I²S MEMS mics stream 48 kHz / 24-bit PCM into a circular DMA
   buffer in STM32 internal SRAM. A double-buffer scheme feeds a 1024-point
   Hann-windowed FFT every ~21 ms (hop 512).
2. The ADXL355 is sampled at 1000 Hz (ODR) over SPI; its three axes are
   windowed and FFT'd for the 20–500 Hz band where larval gnawing lives.
3. The DSP stage computes:
   - Per-mic power spectrum → delay-and-sum beamformer across 16 azimuths.
   - Mel filterbank (20 bands, 0–8 kHz) → DCT-II → 13 MFCCs + delta + delta²
     (39 features) for the dominant mic channel.
   - Vibration-band spectral crest, kurtosis, and pulse repetition rate.
   - SHT45 temperature & RH (insect activity proxy).
4. A **quantized decision-forest classifier** (32 trees, depth 8, int8
   weights, ~24 KB in flash) consumes the 39 + 5 features and emits a
   species label + confidence. The model is trained offline on a curated
   bioacoustic corpus (see §6) and flashed as a C array.
5. Detected events are bundled into a JSON record + a 2-second WAV snippet
   written to SD, and (if Wi-Fi is up) pushed to the app via a WebSocket.
6. The app renders the live spectrogram, the species roster, a polar DOA
   histogram, and a 24 h density heatmap, and triggers push notifications on
   threshold crossings.

---

## 4. Firmware Design

The firmware lives in `firmware/` and is organized as a small, portable
C codebase targeting the GNU Arm Embedded toolchain (`arm-none-eabi-gcc`).

```
firmware/
├── Makefile
├── board.h            // pin map, clock config, peripheral assignments
├── registers.h        // CMSIS-style register definitions for H7
├── main.c             // boot, init, super-loop scheduler
├── drivers/
│   ├── i2s_mic.c/.h    // 4-channel I²S/TDM driver + DMA
│   ├── adxl355.c/.h    // vibration accelerometer driver
│   ├── sht45.c/.h      // environmental sensor driver
│   ├── sd.c/.h         // SDMMC1 block driver (FATFS-thin)
│   ├── esp_coex.c/.h   // ESP32-C6 UART protocol + handshake
│   └── power.c/.h      // BQ25895 charger / fuel-gauge
├── dsp/
│   ├── fft.c/.h        // CMSIS-DSP wrapper for 1024-pt RFFT
│   ├── beamform.c/.h   // delay-and-sum DOA estimator
│   ├── mfcc.c/.h       // Mel filterbank + DCT
│   └── features.c/.h   // vibration features + env snapshot
├── classify/
│   ├── forest.c/.h     // int8 decision-forest inference engine
│   └── forest_weights.c// generated quantized tree tables
└── app/
    ├── events.c/.h     // event queue, SD logging, JSON packing
    └── net.c/.h        // WebSocket-ish push to app via ESP32
```

### Key design decisions

- **Why STM32H743 and not an ESP32 for DSP?** Cortex-M7F has hardware
  double-precision FPU, DSP instructions, and cache; a 1024-point real FFT
  with Hann windowing runs in ~110 µs, leaving headroom for beamforming and
  MFCC on every hop. ESP32 is relegated to connectivity where it shines.
- **Why a decision forest and not a neural net?** A 32-tree int8 forest fits
  in ~24 KB, needs no accelerator, and runs in <300 µs. It also gives
  interpretable per-feature importance — useful for entomologists auditing
  false positives. A TFLM CNN could be added later on the ESP32 coprocessor.
- **Double-buffered DMA.** I²S RX uses two half-buffer DMA descriptors; the
  DSP core processes one half while the other fills. A semaphore is given
  from the DMA half-transfer / transfer-complete ISR.
- **Vibration coupling.** The ADXL355 sits on a small PCB island mechanically
  isolated from the main board by a flex relief, with an M3 stud protruding
  from the case base so it can be firmly clamped to a timber joist or silo
  wall. This rejects case-borne handling noise.
- **Offline-friendly.** If Wi-Fi is unavailable, events are queued on SD and
  the device emits a BLE advertisement with a 16-bit "event count + top
  species" payload so a passing phone can pull the backlog.

### Clock & peripheral map (summary)

- HSE 16 MHz → PLL1 → SYS 480 MHz, CPU 480 MHz, AXI 240 MHz.
- I2S1 master, 48 kHz, 24-bit, 4-slot TDM via MCLK from SAI. (See `board.h`
  for the exact pin/pad assignments.)
- SPI2 @ 1 MHz for ADXL355; I2C4 @ 100 kHz for SHT45 + BQ25895.
- SDMMC1 4-bit @ 48 MHz; USART3 @ 1 Mbaud to ESP32-C6.

---

## 5. Application / Software Interface

The companion app (`app/`) is a **React Native (Expo)** application that runs
on iOS and Android. It discovers Stridophone devices on the local network via
mDNS (`_stridophone._tcp`) or directly over BLE GATT.

### Screens

1. **Dashboard** — list of paired devices, each showing battery %, last-event
   timestamp, top-3 species with confidence, and a mini sparkline of the last
   24 h event count.
2. **Live View** — real-time scrolling spectrogram (canvas), polar DOA plot,
   vibration-band waterfall, and the live species roster with confidence bars.
3. **Species Detail** — per-species card: call description, reference
   spectrogram, observed count over time, and an audio player for stored WAV
   snippets fetched from the device's SD.
4. **Density Heatmap** — a 24 h × azimuth polar heatmap interpolated from the
   DOA histogram; helps locate the infested beam or grain bay.
5. **Alerts** — threshold rules per species (e.g., "warn me if
   *Reticulitermes* pulse rate > 5/s for > 10 min"), push notifications, and
   an event log.
6. **Settings** — device rename, classifier sensitivity, Wi-Fi credentials
   provisioning over BLE, firmware OTA, export CSV.

### On-device HTTP/WebSocket API (served by ESP32-C6)

| Endpoint                  | Method | Purpose |
|---------------------------|--------|---------|
| `/api/info`               | GET    | Device name, fw version, battery, uptime |
| `/api/stream`             | WS     | Live event + spectrogram push (JSON/binary frames) |
| `/api/events?since=`      | GET    | Paginated event log (JSON) |
| `/api/wav/{id}`           | GET    | Download a stored WAV snippet |
| `/api/config`             | GET/PUT| Read/write runtime config (sensitivity, thresholds, Wi-Fi) |
| `/api/ota`                | POST   | Push a firmware bundle (dual-bank) |

---

## 6. Classifier & Training Notes

The on-device forest was trained on a curated corpus assembled from:

- The **InsectSound1000** open dataset (stored-product pests).
- Field recordings of termite / carpenter-ant activity in timber.
- Vineyard recordings of cicada and leafhopper calls.
- A controlled lab set of cricket stridulation across 5 °C–35 °C.

Features (44-dim): 13 MFCC + Δ + ΔΔ (39) + vibration spectral crest + kurtosis
+ pulse-repetition-rate + inter-onset-std + temperature + RH.

Labels (initial release, ~24 classes):

*Gryllus campestris*, *Acheta domesticus*, *Oecanthus* spp.,
*Tibicen* spp., *Magicicada* spp., *Locusta migratoria*,
*Schistocerca gregaria*, *Reticulitermes flavipes*,
*Coptotermes formosanus*, *Camponotus* spp., *Hylotrupes bajulus*
(old-house borer), *Anobium punctatum* (furniture beetle),
*Sitophilus granarius* (granary weevil), *Sitophilus oryzae*,
*Tribolium castaneum* (red flour beetle), *Plodia interpunctella*
(Indian meal moth), *Cydia pomonella* (codling moth), *Lygus* spp.,
*Empoasca* spp., *Nezara viridula*, *Halictus* spp. (benign pollinator,
included to reduce false positives), plus `ambient/no-insect` and `unknown`.

Training uses scikit-learn `RandomForestClassifier` (32 trees, depth 8),
exported to a flat int8 array via a custom `--export-c` script. See
`firmware/classify/forest_weights.c`.

---

## 7. Use Cases & Target Audience

| Audience | Use case |
|----------|----------|
| Grain elevator operators | Early detection of *Sitophilus* / *Tribolium* in silos; trigger aeration/fumigation before economic threshold. |
| Timber & heritage building managers | Detect termite and old-house-borer larval activity inside structural beams via the vibration channel. |
| Vineyard & orchard IPM | Monitor cicada, leafhopper, and stink-bug pressure to time sprays precisely. |
| Museums & archives | Protect organic collections (textiles, wood, herbaria) from webbing clothes moth and furniture beetle. |
| Urban entomologists / researchers | Standardized, low-cost bioacoustic surveys; exportable data for population studies. |
| Homeowners (premium tier) | A "termite sentinel" mode that texts on suspected subterranean termite activity. |

---

## 8. What Makes Stridophone Novel

No shipping product fuses **airborne mic-array beamforming** with
**substrate vibration spectroscopy** for insect detection at this price
point. Existing solutions are either single-mic recorders (no localization),
vibration-only termite probes (no species ID beyond "termite/not"), or
high-end research gear costing thousands. Stridophone packages a DSP-grade
inference pipeline, DOA localization, and a polished mobile app into a
sub-$80 BOM, solar-autonomous device — and it does so with an interpretable
on-device classifier rather than a black-box network.

The combination of *direction-of-arrival* + *vibration-borne larval
detection* + *temperature-aware classification* is, to the author's
knowledge, unprecedented in a single field instrument.

---

## 9. Bill of Materials (top-level)

| Ref | Part | Qty | ~Cost |
|-----|------|-----|-------|
| U1  | STM32H743VIT6 | 1 | $11.50 |
| U2  | ESP32-C6-MINI-1 | 1 | $3.20 |
| U3–U6 | SPH0641LU4H-1 | 4 | $8.00 |
| U7  | ADXL355BCCZ | 1 | $9.50 |
| U8  | SHT45 | 1 | $2.10 |
| U9  | BQ25895RTW | 1 | $3.40 |
| U10 | TPS63031 3.3 V buck-boost | 1 | $2.80 |
| BAT | NCR18650B + holder | 1 | $5.50 |
| SOL | 2 W 6 V mono panel | 1 | $4.00 |
| PCB | 4-layer FR4 + enclosure | 1 | $6.00 |
| Misc | passives, connectors, LEDs, SD socket | — | $4.00 |
| **Total BOM** | | | **~$60** |

---

## 10. Safety, Regulatory, Deployment Notes

- The 18650 cell sits in a vented, fused holder; the BQ25895 enforces JEITA
  temperature-compensated charge limits and the firmware hard-disables
  charging below 0 °C / above 45 °C.
- IP65 enclosure uses a Gore ePTTF pressure-equalizing vent; mic ports are
  acoustic waterproof membranes (IP67-rated, <1 dB loss at 1 kHz).
- The device emits no acoustic energy (passive listening) — no regulatory
  constraint on ultrasonic emissions.
- Data is stored locally first; cloud sync is opt-in. No personally
  identifiable audio (human speech) is retained: a voice-activity detector
  scrubs human speech segments from stored WAVs before they are written.

---

*Designed and authored by **jayis1**. Firmware, schematics, and app source
are released under the MIT License. Bug reports and species-corpus
contributions welcome.*